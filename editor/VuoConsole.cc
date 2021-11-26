/**
 * @file
 * VuoConsole implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoConsole.hh"
#include "VuoConsoleWindow.hh"
#include "VuoEditor.hh"

VuoConsole *VuoConsole::singleton = nullptr;
const int VuoConsole::maxLogs = 500;

/**
 * Creates a VuoConsole that is not yet showing or tracking console logs.
 */
VuoConsole::VuoConsole(QObject *parent) :
    QObject(parent)
{
	areModelUpdatesPaused = false;
	oldLogsDeletedWhilePaused = 0;

	window = nullptr;
}

/**
 * Starts recording console logs from the editor process and any composition processes.
 *
 * This function records data written to the `stdout` and `stderr` streams. Buffered streams must
 * be flushed for this function to see the data. A newline is not enough to force a flush, since
 * line buffering does not happen automatically here (https://stackoverflow.com/a/4201325/238387);
 * `fflush` is needed.
 *
 * If this function is called more than once, calls after the first are ignored.
 *
 * The thread on which this is called determines the thread in which the singleton instance lives.
 * Later calls to VuoConsole functions (unless otherwise specified) must happen on the same thread.
 */
void VuoConsole::startListening(void)
{
	startListeningInternal({LogStream::STDOUT, LogStream::STDERR});
}

/**
 * Exposes @a streams for testing.
 */
void VuoConsole::startListeningInternal(const vector<LogStream> &streams)
{
	if (singleton)
		return;

	singleton = new VuoConsole;

	connect(singleton, &VuoConsole::logsReceived, singleton, &VuoConsole::appendLogs);

	dispatch_queue_t queue = dispatch_queue_create("org.vuo.editor.console", NULL);

	for (LogStream stream : streams)
	{
		int origFd = (stream == LogStream::STDOUT ? STDOUT_FILENO : STDERR_FILENO);
		const char *name = (stream == LogStream::STDOUT ? "stdout" : "stderr");

		// Save a file descriptor (copyFd) that points to stdout/stderr, since the original file descriptor
		// will be redirected and will no longer go to the original destination.
		int copyFd = dup(origFd);
		if (copyFd == -1)
		{
			VUserLog("Console can't show logs from %s (dup failed): %s", name, strerror(errno));
			continue;
		}

		// Create the pipe and allocate a pair of file descriptors for it.
		int readWriteFds[2];
		int ret = pipe(readWriteFds);
		if (ret == -1)
		{
			VUserLog("Console can't show logs from %s (pipe failed): %s", name, strerror(errno));
			close(copyFd);
			continue;
		}

		// Set the write end of the pipe to a file descriptor that points to stdout/stderr.
		int writeFd = dup2(readWriteFds[1], origFd);
		if (writeFd == -1)
		{
			VUserLog("Console can't show logs from %s (dup2 failed): %s", name, strerror(errno));
			close(copyFd);
			close(readWriteFds[0]);
			close(readWriteFds[1]);
			continue;
		}

		// Close the file descriptor on the write end of the pipe, since this function no longer needs
		// to refer to it. Writes to stdout/stderr will still be redirected through the pipe.
		close(readWriteFds[1]);

		// Set up a dispatch source whose event handler is called when data becomes available on the
		// read end of the pipe.
		int readFd = readWriteFds[0];
		dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, readFd, 0, queue);

		dispatch_source_set_event_handler(source, ^{
											  size_t estimated = dispatch_source_get_data(source);
											  char *buffer = (char *)calloc(estimated + 1, sizeof(char));
											  if (buffer)
											  {
												  // Read the text that was written to our redirected stdout/stderr.
												  read(readFd, buffer, estimated);

												  // Write the text to the original stdout/stderr.
												  write(copyFd, buffer, estimated);

												  // Store the text in the list of logs.
												  singleton->parseLogs(buffer, stream);
											  }
										  });

		dispatch_resume(source);
	}
}

/**
 * Parses the logs from @a buffer and schedules appending them to the stored logs.
 *
 * Takes ownership of @a buffer.
 *
 * @threadAny
 */
void VuoConsole::parseLogs(char *buffer, LogStream stream)
{
	QString bufferStr(buffer);  // interprets buffer as UTF-8
	free(buffer);

	bufferStr.prepend(partialLog[stream]);
	partialLog[stream] = "";

	bufferStr.replace(QRegExp("\\033\[[0-9;]+m"), "");

	QString separator = "\n";
	QStringList newLogs = bufferStr.split(separator, QString::SkipEmptyParts);

	if (bufferStr.back() != separator)
		partialLog[stream] = newLogs.takeLast();

	if (! newLogs.isEmpty())
		emit logsReceived(newLogs);
}

/**
 * Adds @a newLogs to the stored logs, culling the oldest stored logs if needed to make room.
 */
void VuoConsole::appendLogs(QStringList newLogs)
{
	logs.append(newLogs);

	int oldLogsDeleted = 0;
	if (logs.size() > maxLogs)
	{
		oldLogsDeleted = logs.size() - maxLogs;
		logs.erase(logs.begin(), logs.begin() + logs.size() - maxLogs);
	}

	updateModel(oldLogsDeleted);
}

/**
 * Deletes all stored logs.
 */
void VuoConsole::clear(void)
{
	logs.clear();

	updateModel(0);
}

/**
 * Displays the most recent console logs in a window.
 *
 * @ref startListening must be called before this function.
 *
 * @param screenMate The console window will appear on the same screen as this widget.
 */
void VuoConsole::show(QWidget *screenMate)
{
	singleton->showInternal(screenMate);
}

/**
 * Displays the most recent console logs in a window.
 */
void VuoConsole::showInternal(QWidget *screenMate)
{
	if (! window)
	{
		window = new VuoConsoleWindow(this, screenMate);
		connect(window, &VuoConsoleWindow::destroyed, this, [this](){ window = nullptr; });

		window->setModel(logs);

		window->show();
	}

	window->raise();
	window->activateWindow();
}

/**
 * Updates the logs displayed in the window.
 *
 * @param oldLogsDeleted The number of log messages that have been deleted since the previous model update
 *     to keep from exceeding VuoConsole::maxLogs.
 */
void VuoConsole::updateModel(int oldLogsDeleted)
{
	if (window)
	{
		if (! areModelUpdatesPaused)
		{
			emit modelAboutToChange(oldLogsDeleted);
			window->setModel(logs);
		}
		else
		{
			oldLogsDeletedWhilePaused += oldLogsDeleted;
		}
	}
}

/**
 * Temporarily stops updating the displayed logs while the user is interacting with them.
 */
void VuoConsole::pauseModelUpdates(void)
{
	areModelUpdatesPaused = true;
}

/**
 * Resumes updating the displayed logs after a call to VuoConsole::pauseModelUpdates.
 */
void VuoConsole::resumeModelUpdates(void)
{
	areModelUpdatesPaused = false;
	int oldLogsDeleted = oldLogsDeletedWhilePaused;
	oldLogsDeletedWhilePaused = 0;
	updateModel(oldLogsDeleted);
}

/**
 * Presents a save dialog to write the currently displayed logs to a file.
 */
void VuoConsole::save(void)
{
	pauseModelUpdates();

	QFileDialog dialog(window, Qt::Sheet);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDefaultSuffix("log");

	QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
	QString userName = QString::fromStdString(static_cast<VuoEditor *>(qApp)->getUserName());
	dialog.selectFile(QString("Vuo %1 %2.log").arg(timestamp).arg(userName));

	if (dialog.exec() == QDialog::Accepted)
	{
		QString path = dialog.selectedFiles()[0];
		QFile file(path);
		if (file.open(QIODevice::WriteOnly))
		{
			QTextStream stream(&file);

			QStringList logsCopy = window->getModel();
			for (QString log : logsCopy)
				stream << log << endl;
		}
		else
			VUserLog("Couldn't open file for writing: %s", path.toStdString().c_str());
	}

	resumeModelUpdates();
}

/**
 * Puts either the currently selected logs (if any) or all currently displayed logs on the clipboard.
 */
void VuoConsole::copy(void)
{
	pauseModelUpdates();

	QString copiedText;
	QTextStream stream(&copiedText);

	QStringList logsCopy = window->getModel();
	QList<QVariant> selectedIndices = window->getSelectedIndices();

	stream << "```" << endl;

	if (! selectedIndices.isEmpty())
		for (QVariant index : selectedIndices)
			stream << logsCopy.at(index.toInt()) << endl;
	else
		for (QString log : logsCopy)
			stream << log << endl;

	stream << "```" << endl;

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(copiedText);

	resumeModelUpdates();
}
