/**
 * @file
 * TestConsole implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <QMainWindow>
#include <Vuo/Vuo.h>
#include "VuoConsole.hh"
#include "VuoConsoleWindow.hh"
#include "VuoEditor.hh"

/**
 * Tests for the VuoConsole class.
 */
class TestConsole : public QObject
{
	Q_OBJECT

public:
	/**
	 * Only listen for stderr, not stdout, so that the Qt Test output doesn't get mixed into the logs.
	 */
	static void startListening(void)
	{
		vector<VuoConsole::LogStream> streams;
		streams.push_back(VuoConsole::LogStream::STDERR);
		VuoConsole::startListeningInternal(streams);
	}

private:
	QString lastLinesInClipboard(void)
	{
		QString separator = "\n";
		QStringList lines = QApplication::clipboard()->text().split(separator);
		return lines.mid(lines.length() - 4).join(separator);
	}

private slots:

	void testBasicInteraction()
	{
		QClipboard *clipboard = QApplication::clipboard();

		QMainWindow *w = new QMainWindow();
		w->show();

		const char *log0 = "VUserLog before the console has been shown";
		VUserLog("%s", log0);
		VuoConsole::show(w);
		qApp->processEvents();  // Let VuoConsole use the main thread to display the window and update its stored logs.

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(log0), lastLinesInClipboard().toUtf8().constData());

		const char *log1 = "VUserLog after the console has been shown";
		VUserLog("%s", log1);
		qApp->processEvents();

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(log1), lastLinesInClipboard().toUtf8().constData());
		QVERIFY2(clipboard->text().contains(log0), lastLinesInClipboard().toUtf8().constData());
		QVERIFY2(clipboard->text().indexOf(log0) < clipboard->text().indexOf(log1), lastLinesInClipboard().toUtf8().constData());

		VuoConsole::singleton->clear();

		VuoConsole::singleton->copy();
		QVERIFY2(! clipboard->text().contains("TestConsole"), lastLinesInClipboard().toUtf8().constData());

		const char *log2 = "VUserLog before the console is closed and reopened";
		VUserLog("%s", log2);
		qApp->processEvents();

		VuoConsole::singleton->window->close();
		VuoConsole::show(w);
		qApp->processEvents();

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(log2), lastLinesInClipboard().toUtf8().constData());
		QVERIFY2(! clipboard->text().contains(log1), lastLinesInClipboard().toUtf8().constData());
	}

	void testLogTypes()
	{
		QClipboard *clipboard = QApplication::clipboard();

		QMainWindow *w = new QMainWindow();
		w->show();
		VuoConsole::show(w);
		qApp->processEvents();

		const char *vuserlog0 = "VUserLog from the editor process";
		VUserLog("%s", vuserlog0);
		const char *vuserlog1 = "Another VUserLog from the editor process, for good measure";
		VUserLog("%s", vuserlog1);
		qApp->processEvents();

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(vuserlog0), lastLinesInClipboard().toUtf8().constData());
		QVERIFY2(clipboard->text().contains(vuserlog1), lastLinesInClipboard().toUtf8().constData());
		int indexAfter0 = clipboard->text().indexOf(vuserlog0) + strlen(vuserlog0);
		QCOMPARE(clipboard->text().at(indexAfter0), "\n");
		QCOMPARE(clipboard->text().at(indexAfter0 + 1), "[");

		const char *stderr0 = "stderr from the editor process\n";
		fprintf(stderr, "%s", stderr0);
		qApp->processEvents();

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().endsWith(QString("%1```\n").arg(stderr0)), lastLinesInClipboard().toUtf8().constData());

		const char *composition0 = "VUserLog from the composition process";
		QString compositionPath = QDir::current().filePath("composition/Log.vuo");
		VuoCompilerIssues issues;
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath.toStdString(), &issues);
		QVERIFY2(runner, issues.getLongDescription(false).c_str());
		runner->start();
		VuoRunner::Port *port = runner->getPublishedInputPortWithName("logMessage");
		json_object *value = json_object_new_string(composition0);
		runner->setPublishedInputPortValues({{port, value}});
		runner->firePublishedInputPortEvent(port);
		runner->waitForFiredPublishedInputPortEvent();
		runner->stop();
		qApp->processEvents();

		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(composition0), lastLinesInClipboard().toUtf8().constData());
	}

	void testExceedingMaxLogs()
	{
		QClipboard *clipboard = QApplication::clipboard();

		QMainWindow *w = new QMainWindow();
		w->show();
		VuoConsole::show(w);
		qApp->processEvents();

		VuoConsole::singleton->clear();

		auto logMessage = [](int i) { return QString("^%1^").arg(i); };

		int i = 1;
		while (i <= VuoConsole::maxLogs)
			VUserLog("%s", logMessage(i++).toUtf8().constData());
		qApp->processEvents();

		QCOMPARE(VuoConsole::singleton->logs.size(), VuoConsole::maxLogs);
		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(logMessage(VuoConsole::maxLogs)), lastLinesInClipboard().toUtf8().constData());

		VUserLog("%s", logMessage(i++).toUtf8().constData());
		qApp->processEvents();

		QCOMPARE(VuoConsole::singleton->logs.size(), VuoConsole::maxLogs);
		VuoConsole::singleton->copy();
		QVERIFY2(clipboard->text().contains(logMessage(VuoConsole::maxLogs + 1)), lastLinesInClipboard().toUtf8().constData());
		QVERIFY2(! clipboard->text().contains(logMessage(1)), lastLinesInClipboard().toUtf8().constData());
	}
};


int main(int argc, char *argv[])
{
	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../QtPlugins").c_str()));

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	// https://bugreports.qt.io/browse/QTBUG-81370
	qputenv("QT_MAC_WANTS_LAYER", "1");

	TestConsole::startListening();

	VuoEditor *app = nullptr;
	if (argc > 1 && strcmp(argv[1], "-datatags") != 0)
	{
		app = new VuoEditor(argc, argv);

		// VuoModuleManager::loadedModules needs to run a block on the main thread.
		QTest::qWait(1000);
	}

	TestConsole tc;

	int ret = QTest::qExec(&tc, argc, argv);
	delete app;
	return ret;
}

#include "TestConsole.moc"
