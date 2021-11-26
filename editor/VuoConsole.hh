/**
 * @file
 * VuoEditorConsole interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoConsoleWindow;

/**
 * Collects messages logged to console and displays them in a window.
 */
class VuoConsole : public QObject
{
	Q_OBJECT

public:
	static void startListening(void);
	static void show(QWidget *screenMate);

	void clear(void);
	void copy(void);
	void save(void);

signals:
	void logsReceived(QStringList newLogs);  ///< When logs are added asynchronously, the signal-slot connection puts the handler code on the VuoConsole's thread.
	void modelAboutToChange(int oldLogsDeleted);  ///< Emitted when this object is about to set the model for the ListView defined in QML.
	void selectedAllLogs(void);  ///< Emitted by the Select All menu item.

private:
	enum LogStream
	{
		STDOUT,
		STDERR
	};

	static VuoConsole *singleton;
	VuoConsole(QObject *parent = nullptr);

	static void startListeningInternal(const vector<LogStream> &streams);

	void parseLogs(char *buffer, LogStream stream);
	void appendLogs(QStringList newLogs);

	void showInternal(QWidget *screenMate);

	void updateModel(int oldLogsDeleted);
	void startModelUpdates(void);
	void stopModelUpdates(void);
	void pauseModelUpdates(void);
	void resumeModelUpdates(void);

	static const int maxLogs;

	QStringList logs;
	QString partialLog[2];

	bool areModelUpdatesPaused;
	int oldLogsDeletedWhilePaused;

	VuoConsoleWindow *window;

	friend class TestConsole;
};
