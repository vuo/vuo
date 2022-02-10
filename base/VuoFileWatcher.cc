/**
 * @file
 * VuoFileWatcher implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <dirent.h>
#include "VuoFileWatcher.hh"

const double VuoFileWatcher::checkIntervalSeconds = 2;  ///< How frequently to recheck for the existence of a file/folder.
const double VuoFileWatcher::changeNotificationDelaySeconds = 0.25;  ///< How long to wait (during a rapid series of events) before notifying the delegate.

/**
 * Starts watching a file or folder.
 *
 * If @a persistent is true:
 * If the file/folder doesn't exist, or exists and is later deleted or moved/renamed,
 * it is periodically checked for existence, then watching begins (again).
 *
 * If @a persistent is false:
 * If the file/folder doesn't exist, this class instance does nothing.
 * If the file/folder exists and is later deleted or moved/renamed,
 * this class instance stops watching it
 * (even if another file/folder with the same name later reappears,
 * or if the same file/folder is renamed back into place).
 */
VuoFileWatcher::VuoFileWatcher(VuoFileWatcherDelegate *delegate, const string &fileOrFolderToWatch, bool persistent)
{
	queue = dispatch_queue_create("org.vuo.filewatcher", NULL);
	dispatch_sync(queue, ^{
		initialize(delegate, fileOrFolderToWatch, persistent, NULL);
	});
}

/**
 * Private constructor for child watchers.
 */
VuoFileWatcher::VuoFileWatcher(VuoFileWatcherDelegate *delegate, const string &fileOrFolderToWatch, bool persistent, VuoFileWatcher *parent)
{
	queue = parent->queue;
	initialize(delegate, fileOrFolderToWatch, persistent, parent);
}

/**
 * Helper function for constructors.
 *
 * @threadQueue{queue}
 */
void VuoFileWatcher::initialize(VuoFileWatcherDelegate *delegate, const string &fileOrFolderToWatch, bool persistent, VuoFileWatcher *parent)
{
	this->parent = parent;
	this->fileToWatch = fileOrFolderToWatch;
	this->delegate = delegate;
	this->persistent = persistent;
	this->fileToWatchSource = NULL;
	this->fileToWatchSourceRunning = dispatch_group_create();
	this->reopen = false;
	this->periodicCheckForExistence = NULL;
	this->periodicCheckForExistenceRunning = dispatch_group_create();
	this->childWatchersQueue = dispatch_queue_create("org.vuo.filewatcher.child", NULL);
	this->changeNotificationDelay = NULL;
	this->changeNotificationDelayRunning = dispatch_group_create();

	DIR *d = opendir(fileOrFolderToWatch.c_str());
	if (d)
	{
		closedir(d);
		startWatching();
	}
	else if (errno == ENOTDIR)
	{
		startWatching();
	}
	else if (errno == ENOENT)
	{
		if (persistent)
			periodicallyCheckForExistence();
	}
	else
	{
		VUserLog("Error: Couldn't access '%s': %s", fileOrFolderToWatch.c_str(), strerror(errno));
	}
}

/**
 * Starts watching @ref fileToWatch and, if it's a directory, the files it contains.
 *
 * @threadQueue{queue}
 */
void VuoFileWatcher::startWatching()
{
	fileToWatchFD = open(fileToWatch.c_str(), O_EVTONLY);
	if (fileToWatchFD < 0)
	{
		VUserLog("Error: %s", strerror(errno));
		return;
	}

	dispatch_group_enter(fileToWatchSourceRunning);

	unsigned long mask = DISPATCH_VNODE_DELETE | DISPATCH_VNODE_RENAME | DISPATCH_VNODE_WRITE;
	fileToWatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE, fileToWatchFD, mask, queue);
	dispatch_source_set_event_handler(fileToWatchSource, ^{
		unsigned long l = dispatch_source_get_data(fileToWatchSource);

		if (l & DISPATCH_VNODE_DELETE
		 || l & DISPATCH_VNODE_RENAME)
		{
			DIR *d = opendir(fileToWatch.c_str());
			if (d)
			{
				// got a delete event, but it still exists and is a folder — not sure how this could happen
				closedir(d);
			}
			else if (errno == ENOTDIR)
			{
				changeObserved();
				reopen = true;
				dispatch_source_cancel(fileToWatchSource);
			}
			else if (errno == ENOENT)
			{
				changeObserved();
				dispatch_source_cancel(fileToWatchSource);
				if (persistent)
					periodicallyCheckForExistence();
			}
		}
		else if (l & DISPATCH_VNODE_WRITE)
		{
			changeObserved();
			updateChildWatchers();
		}
	});
	dispatch_source_set_cancel_handler(fileToWatchSource, ^{
		close(fileToWatchFD);

		dispatch_release(fileToWatchSource);
		fileToWatchSource = NULL;
		dispatch_group_leave(fileToWatchSourceRunning);

		if (reopen)
		{
			reopen = false;
			startWatching();
		}
	});
	dispatch_resume(fileToWatchSource);

	updateChildWatchers();
}

/**
 * Starts a timer that keeps checking if the file exists and, if it does, stops checking
 * and starts watching the file.
 *
 * @threadQueue{queue}
 */
void VuoFileWatcher::periodicallyCheckForExistence()
{
	dispatch_group_enter(periodicCheckForExistenceRunning);

	periodicCheckForExistence = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
	dispatch_source_set_timer(periodicCheckForExistence, dispatch_time(DISPATCH_TIME_NOW, checkIntervalSeconds*NSEC_PER_SEC), checkIntervalSeconds*NSEC_PER_SEC, checkIntervalSeconds*NSEC_PER_SEC/10);
	dispatch_source_set_event_handler(periodicCheckForExistence, ^{
		DIR *d = opendir(fileToWatch.c_str());
		if (d)
		{
			closedir(d);
			changeObserved();
			startWatching();
			dispatch_source_cancel(periodicCheckForExistence);
		}
	});
	dispatch_source_set_cancel_handler(periodicCheckForExistence, ^{
		dispatch_release(periodicCheckForExistence);
		periodicCheckForExistence = NULL;
		dispatch_group_leave(periodicCheckForExistenceRunning);
	});
	dispatch_resume(periodicCheckForExistence);
}

/**
 * If @ref fileToWatch is a directory, starts watching any of the files in it that are not already
 * being watched, and stops watching (if not persistent) or starts checking for the recreation of
 * (if persistent) any files that were being watched but have disappeared.
 *
 * @threadQueue{queue}
 */
void VuoFileWatcher::updateChildWatchers()
{
	DIR *d = opendir(fileToWatch.c_str());
	if (!d)
		return;

	__block vector<VuoFileWatcher *> childWatchersToDestroy;
	dispatch_sync(childWatchersQueue, ^{

	// Add new child watchers for new files/folders.
	struct dirent *de;
	while ((de = readdir(d)) != NULL)
	{
		if (strcmp(de->d_name, ".") == 0
		 || strcmp(de->d_name, "..") == 0)
			continue;

		string compositePath = fileToWatch + '/' + de->d_name;

		bool haveExistingWatcher = false;
		for (vector<VuoFileWatcher *>::iterator i = childWatchers.begin(); i != childWatchers.end(); ++i)
			if ((*i)->fileToWatch == compositePath)
			{
				haveExistingWatcher = true;
				break;
			}

		if (haveExistingWatcher)
			continue;

		childWatchers.push_back(new VuoFileWatcher(delegate, compositePath, false, this));
	}

	// Remove obsolete child watchers.
	for (vector<VuoFileWatcher *>::iterator i = childWatchers.begin(); i != childWatchers.end(); )
	{
		if (!(*i)->fileToWatchSource
		 && !(*i)->periodicCheckForExistence)
		{
			childWatchersToDestroy.push_back(*i);
			i = childWatchers.erase(i);
		}
		else
			++i;
	}

	});

	for (VuoFileWatcher *childWatcher : childWatchersToDestroy)
		delete childWatcher;

	closedir(d);
}

/**
 * Coalesces multiple rapid changes into a single delegate notification.
 *
 * @threadQueue{queue}
 */
void VuoFileWatcher::changeObserved()
{
	if (parent)
	{
		parent->changeObserved();
		return;
	}

	if (changeNotificationDelay)
		// Push the timer back.
		dispatch_source_set_timer(changeNotificationDelay, dispatch_time(DISPATCH_TIME_NOW, changeNotificationDelaySeconds*NSEC_PER_SEC), DISPATCH_TIME_FOREVER, changeNotificationDelaySeconds*NSEC_PER_SEC/10);
	else
	{
		dispatch_group_enter(changeNotificationDelayRunning);

		changeNotificationDelay = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
		dispatch_source_set_timer(changeNotificationDelay, dispatch_time(DISPATCH_TIME_NOW, changeNotificationDelaySeconds*NSEC_PER_SEC), DISPATCH_TIME_FOREVER, changeNotificationDelaySeconds*NSEC_PER_SEC/10);
		dispatch_source_set_event_handler(changeNotificationDelay, ^{
			delegate->fileChanged(fileToWatch);
			dispatch_source_cancel(changeNotificationDelay);
		});
		dispatch_source_set_cancel_handler(changeNotificationDelay, ^{
			dispatch_release(changeNotificationDelay);
			changeNotificationDelay = NULL;
			dispatch_group_leave(changeNotificationDelayRunning);
		});
		dispatch_resume(changeNotificationDelay);
	}
}

/**
 * Stops watching the folder.
 */
VuoFileWatcher::~VuoFileWatcher()
{
	__block vector<VuoFileWatcher *> childWatchersCopy;
	dispatch_sync(childWatchersQueue, ^{
		childWatchersCopy = childWatchers;
		childWatchers.clear();
	});

	for (VuoFileWatcher *childWatcher : childWatchersCopy)
		delete childWatcher;

	if (fileToWatchSource)
	{
		dispatch_source_cancel(fileToWatchSource);
		dispatch_group_wait(fileToWatchSourceRunning, DISPATCH_TIME_FOREVER);
	}
	dispatch_release(fileToWatchSourceRunning);

	if (periodicCheckForExistence)
	{
		dispatch_source_cancel(periodicCheckForExistence);
		dispatch_group_wait(periodicCheckForExistenceRunning, DISPATCH_TIME_FOREVER);
	}
	dispatch_release(periodicCheckForExistenceRunning);

	if (changeNotificationDelay)
	{
		dispatch_source_cancel(changeNotificationDelay);
		dispatch_group_wait(changeNotificationDelayRunning, DISPATCH_TIME_FOREVER);
	}
	dispatch_release(changeNotificationDelayRunning);

	if (! parent)
		dispatch_release(queue);

	dispatch_release(childWatchersQueue);
}
