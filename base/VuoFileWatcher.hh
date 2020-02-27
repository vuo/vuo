/**
 * @file
 * VuoFileWatcher interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <dispatch/dispatch.h>

/**
 * An abstract class to be implemented by a client that will receive notifications from @ref VuoFileWatcher.
 */
class VuoFileWatcherDelegate
{
public:
	/**
	 * This delegate method is invoked each time the file watcher detects a change in the folder
	 * being watched (or one of its subfolders).
	 *
	 *    - a file/folder is added to or removed from the folder (or its subfolders)
	 *    - the contents of a file (the file itself, or any file in the folder or its subfolders) is modified
	 *
	 * Multiple rapid changes are coalesced into a single notification
	 * (such as when multiple files are copied into a folder,
	 * or when a file is edited by a tempfile-using editor like `vim`).
	 *
	 * @param fileBeingWatched The file path that was passed to @ref VuoFileWatcher::VuoFileWatcher.
	 */
	virtual void fileChanged(const string &fileBeingWatched) = 0;
};

/**
 * Notifies a delegate about changes to a file or folder.
 */
class VuoFileWatcher
{
public:
	VuoFileWatcher(VuoFileWatcherDelegate *delegate, const string &fileToWatch, bool persistent=true);
	~VuoFileWatcher();

private:
	VuoFileWatcher *parent;
	VuoFileWatcherDelegate *delegate;
	string fileToWatch;
	bool persistent;
	dispatch_queue_t queue;

	VuoFileWatcher(VuoFileWatcherDelegate *delegate, const string &fileToWatch, bool persistent, VuoFileWatcher *parent);
	void initialize(VuoFileWatcherDelegate *delegate, const string &fileToWatch, bool persistent, VuoFileWatcher *parent);

	void startWatching();
	int fileToWatchFD;
	dispatch_source_t fileToWatchSource;
	dispatch_group_t fileToWatchSourceRunning;
	bool reopen;

	void periodicallyCheckForExistence();
	dispatch_source_t periodicCheckForExistence;
	dispatch_group_t periodicCheckForExistenceRunning;
	static const double checkIntervalSeconds;

	void updateChildWatchers();
	vector<VuoFileWatcher *> childWatchers;
	dispatch_queue_t childWatchersQueue;

	void changeObserved();
	dispatch_source_t changeNotificationDelay;
	dispatch_group_t changeNotificationDelayRunning;
	static const double changeNotificationDelaySeconds;
};
