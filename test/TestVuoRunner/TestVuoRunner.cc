/**
 * @file
 * TestVuoRunner implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

		// There is a delay between when the composition starts sending telemetry data (runner->start()) and
		// when the delegate starts receiving it (listenThread). So we don't know which telemetry type
		// the delegate will receive first. But after that, it should cycle through the telemetry types.
		string expectedTelemetryType;
		{
			string line = actualLines.at(0);
			istringstream sin(line);
			sin >> expectedTelemetryType;
		}
		for (uint i = 0; i < actualLines.size(); ++i)
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
};

QTEST_APPLESS_MAIN(TestVuoRunner)
#include "TestVuoRunner.moc"
