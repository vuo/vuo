/**
 * @file
 * TestVuoRunner implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <dispatch/dispatch.h>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "VuoRunner.hh"
#include "VuoFileUtilities.hh"

extern "C" {
#include "VuoGlContext.h"
}

/**
 * Outputs to file the telemetry data received by some tests.
 */
class TestRunnerDelegate : public VuoRunnerDelegateAdapter
{
private:
	ofstream fout;

public:
	TestRunnerDelegate(string outputPath)
	{
		fout.open(outputPath.c_str());
	}
	~TestRunnerDelegate(void)
	{
		fout.close();
	}
	void receivedTelemetryStats(unsigned long utime, unsigned long stime)
	{
		fout << "VuoTelemetryStats " << utime << " " << stime << endl;
	}
	void receivedTelemetryNodeExecutionStarted(string nodeIdentifier)
	{
		fout << "VuoTelemetryNodeExecutionStarted " << nodeIdentifier << endl;
	}
	void receivedTelemetryNodeExecutionFinished(string nodeIdentifier)
	{
		fout << "VuoTelemetryNodeExecutionFinished " << nodeIdentifier << endl;
	}
	void receivedTelemetryError(string message)
	{
		QFAIL(("receivedTelemetryError: " + message).c_str());
	}
	void lostContactWithComposition(void)
	{
		QFAIL("lostContactWithComposition");
	}
};

#define STRINGIFY(...) #__VA_ARGS__


/**
 * Tests the @c VuoRunner class.
 */
class TestVuoRunner : public QObject
{
	Q_OBJECT

private:
	string executablePath;
	string outputPath;

	void setupPaths(string executablePath)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(executablePath, dir, file, ext);
		this->outputPath = VuoFileUtilities::makeTmpFile(file, "txt");
		this->executablePath = executablePath;
	}

	VuoRunner * setupRunner(void)
	{
		VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromExecutable(executablePath, "");
		return runner;
	}

	vector<string> getLinesOfOutput(VuoRunner *runner)
	{
		vector<string> lines;
		ifstream fin(outputPath.c_str());
		string line;
		while (getline(fin, line))
			lines.push_back(line);
		fin.close();
		return lines;
	}

private slots:

	void testControllingComposition(void)
	{
		setupPaths("./compositionForControlling");

		setenv("TESTVUORUNNER_OUTPUTPATH", outputPath.c_str(), 1);  // pass to compositionForControlling
		VuoRunner *runner = setupRunner();
		runner->start();
		runner->stop();
		delete runner;

		// Wait for compositionForControlling to close the output file.
		{
			int status;
			int ret = wait(&status);
			QVERIFY2(ret != -1 || errno == ECHILD, QString("ret=%1 errno=%2").arg(ret).arg(errno).toUtf8().constData());
		}

		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(executablePath, "");
		runner->start();
		runner->stop();

		// Wait for compositionForControlling to close the output file.
		{
			int status;
			int ret = wait(&status);
			QVERIFY2(ret != -1 || errno == ECHILD, QString("ret=%1 errno=%2").arg(ret).arg(errno).toUtf8().constData());
		}

		vector<string> expectedLines;
		expectedLines.push_back("started");
		expectedLines.push_back("stopped");
		expectedLines.push_back("started");
		expectedLines.push_back("stopped");

		vector<string> actualLines = getLinesOfOutput(runner);
		QCOMPARE(actualLines.size(), expectedLines.size());
		for (uint i = 0; i < actualLines.size(); ++i)
			QCOMPARE(QString(actualLines.at(i).c_str()), QString(expectedLines.at(i).c_str()));

		remove(outputPath.c_str());

		delete runner;
	}

	void testListeningToComposition(void)
	{
		setupPaths("./compositionForListening");
		VuoRunner *runner = setupRunner();

		TestRunnerDelegate delegate(outputPath);
		runner->setDelegate(&delegate);

		runner->start();

		const int SLEEP_SEC = 2;
		sleep(SLEEP_SEC);

		runner->stop();

		vector<string> actualLines = getLinesOfOutput(runner);
		const int TELEMETRY_PER_SEC = 2;
		const int EXPECTED_TELEMETRY_TYPES = 3;
		QVERIFY2(actualLines.size() >= EXPECTED_TELEMETRY_TYPES * (SLEEP_SEC * TELEMETRY_PER_SEC - 1),
				 qPrintable(QString("actualLines: %1").arg(actualLines.size())));

		// When the composition is first starting, it may receive multiple telemetry stats messages
		// in rapid succession. But after that, it should cycle through the telemetry types.
		uint startLine = 0;
		for (uint i = 0; i < actualLines.size(); ++i)
		{
			string actualTelemetryType;
			string line = actualLines.at(i);
			istringstream sin(line);
			sin >> actualTelemetryType;
			if (actualTelemetryType != "VuoTelemetryStats")
			{
				startLine = i;
				break;
			}
		}
		QVERIFY2(startLine > 0 && actualLines.size() - startLine >= EXPECTED_TELEMETRY_TYPES * (SLEEP_SEC * TELEMETRY_PER_SEC - 1),
				 qPrintable(QString("actualLines from first non-VuoTelemetryStats to the end: %1").arg(actualLines.size() - startLine)));

		string expectedTelemetryType = "VuoTelemetryNodeExecutionStarted";
		for (uint i = startLine; i < actualLines.size(); ++i)
		{
			string line = actualLines.at(i);
			istringstream sin(line);
			string telemetryType;

			if (expectedTelemetryType == "VuoTelemetryStats")
			{
				unsigned long utime, stime;
				sin >> telemetryType >> utime >> stime;
				QCOMPARE(QString(telemetryType.c_str()), QString("VuoTelemetryStats"));
				QVERIFY(0 <= utime && utime <= SLEEP_SEC * 1000000);
				QVERIFY(0 <= stime && stime <= SLEEP_SEC * 1000000);
				expectedTelemetryType = "VuoTelemetryNodeExecutionStarted";
			}
			else if (expectedTelemetryType == "VuoTelemetryNodeExecutionStarted")
			{
				string nodeIdentifier;
				sin >> telemetryType >> nodeIdentifier;
				QCOMPARE(QString(telemetryType.c_str()), QString("VuoTelemetryNodeExecutionStarted"));
				QCOMPARE(QString(nodeIdentifier.c_str()), QString("node.started"));
				expectedTelemetryType = "VuoTelemetryNodeExecutionFinished";
			}
			else if (expectedTelemetryType == "VuoTelemetryNodeExecutionFinished")
			{
				string edgeIdentifier;
				sin >> telemetryType >> edgeIdentifier;
				QCOMPARE(QString(telemetryType.c_str()), QString("VuoTelemetryNodeExecutionFinished"));
				QCOMPARE(QString(edgeIdentifier.c_str()), QString("node.finished"));
				expectedTelemetryType = "VuoTelemetryStats";
			}
			else
			{
				QVERIFY(false);
			}
		}

		remove(outputPath.c_str());

		delete runner;
	}

	void testLosingContactWithComposition(void)
	{
		setupPaths("./compositionForLosingContact");
		VuoRunner *runner = setupRunner();

		VuoRunnerDelegateAdapter delegate;
		runner->setDelegate(&delegate);

		runner->start();
		kill(runner->compositionPid, SIGTERM);
		runner->waitUntilStopped();

		delete runner;
	}

	void testSetGlobalRootContext(void)
	{
		CGLPixelFormatObj pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(true);
		CGLContextObj rootContext;
		CGLError error = CGLCreateContext(pf, NULL, &rootContext);
		if (error != kCGLNoError)
			QFAIL(CGLErrorString(error));
		QVERIFY(rootContext);

		VuoGlContext_setGlobalRootContext(rootContext);

		CGLLockContext(rootContext);

		// Ensure the test happens on another thread (since CGLLockContext() allows recursively locking on the same thread).
		dispatch_queue_t queue = dispatch_queue_create("org.vuo.TestVuoRunner", NULL);
		dispatch_async(queue, ^{
						  // This call should not hang even though the CGLContextObj is locked.
						  VuoGlContext c = VuoGlContext_use();
						  VuoGlContext_disuse(c);
					  });
		// Wait for the async operation to finish.
		dispatch_sync(queue, ^{});
		dispatch_release(queue);

		CGLUnlockContext(rootContext);
		CGLReleaseContext(rootContext);
	}

	void testPortDetails(void)
	{
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable("PublishedPorts", ".");
		QVERIFY(runner);

		runner->start();

		// A port with no suggestions.
		{
			VuoRunner::Port *timePort = runner->getPublishedInputPortWithName("Time");
			QCOMPARE(timePort->getName().c_str(), "Time");
			QCOMPARE(timePort->getType().c_str(), "VuoReal");
			QCOMPARE(json_object_get_string(timePort->getDetails()), STRINGIFY({ "default": 0.0 }));
		}

		// A port with standard suggestions.
		{
			VuoRunner::Port *phasePort = runner->getPublishedInputPortWithName("Phase");
			QCOMPARE(phasePort->getName().c_str(), "Phase");
			QCOMPARE(phasePort->getType().c_str(), "VuoReal");
			QCOMPARE(json_object_get_string(phasePort->getDetails()), STRINGIFY({ "suggestedMin": 0, "suggestedMax": 1, "default": 0.0, "suggestedStep": 0.1 }));
		}

		// A port with an enum menu.
		{
			VuoRunner::Port *wavePort = runner->getPublishedInputPortWithName("Wave");
			QCOMPARE(wavePort->getName().c_str(), "Wave");
			QCOMPARE(wavePort->getType().c_str(), "VuoWave");
			QCOMPARE(json_object_get_string(wavePort->getDetails()), STRINGIFY({ "default": "sine", "menuItems": [ { "value": "sine", "name": "Sine" }, { "value": "triangle", "name": "Triangle" }, { "value": "sawtooth", "name": "Sawtooth" } ] }));
		}

		runner->stop();

		delete runner;
	}

};

QTEST_APPLESS_MAIN(TestVuoRunner)
#include "TestVuoRunner.moc"
