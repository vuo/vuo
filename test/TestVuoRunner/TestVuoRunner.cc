/**
 * @file
 * TestVuoRunner implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <QtCore/QString>
#include <QtTest/QtTest>
#include <Vuo/Vuo.h>
#include <dispatch/dispatch.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>

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
		fout << "VuoTelemetryHeartbeat" << endl;
	}
	void receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier)
	{
		fout << "VuoTelemetryNodeExecutionStarted " << nodeIdentifier << endl;
	}
	void receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier)
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
		VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromExecutable(executablePath, "", true, false);
		runner->setRuntimeChecking(true);
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
		setupPaths(BINARY_DIR "/bin/compositionForControlling");

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

		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(executablePath, "", true, false);
		runner->setRuntimeChecking(true);
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
		setupPaths(BINARY_DIR "/bin/compositionForListening");
		VuoRunner *runner = setupRunner();

		TestRunnerDelegate delegate(outputPath);
		runner->setDelegate(&delegate);

		runner->startPaused();
		runner->subscribeToEventTelemetry("");
		runner->unpause();

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
			if (actualTelemetryType != "VuoTelemetryHeartbeat")
			{
				startLine = i;
				break;
			}
		}
		QVERIFY2(startLine > 0 && actualLines.size() - startLine >= EXPECTED_TELEMETRY_TYPES * (SLEEP_SEC * TELEMETRY_PER_SEC - 1),
				 qPrintable(QString("actualLines from first non-VuoTelemetryHeartbeat to the end: %1").arg(actualLines.size() - startLine)));

		string expectedTelemetryType = "VuoTelemetryNodeExecutionStarted";
		for (uint i = startLine; i < actualLines.size(); ++i)
		{
			string line = actualLines.at(i);
			istringstream sin(line);
			string telemetryType;

			if (expectedTelemetryType == "VuoTelemetryHeartbeat")
			{
				sin >> telemetryType;
				QCOMPARE(QString(telemetryType.c_str()), QString("VuoTelemetryHeartbeat"));
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
				expectedTelemetryType = "VuoTelemetryHeartbeat";
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
		setupPaths(BINARY_DIR "/bin/compositionForLosingContact");
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		CGLPixelFormatObj pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(true, false, -1);
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
#pragma clang diagnostic pop
	}

	void testPortDetails(void)
	{
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(BINARY_DIR "/test/TestVuoRunner/PublishedPorts", ".", true, false);
		QVERIFY(runner);

		runner->setRuntimeChecking(true);
		runner->start();

		// A port with no suggestions.
		{
			VuoRunner::Port *timePort = runner->getPublishedInputPortWithName("Time");
			QVERIFY(timePort);
			QCOMPARE(timePort->getName().c_str(), "Time");
			QCOMPARE(timePort->getType().c_str(), "VuoReal");
			QCOMPARE(json_object_get_string(timePort->getDetails()), VUO_STRINGIFY({ "default": 0.0 }));
		}

		// A port with standard suggestions.
		{
			VuoRunner::Port *phasePort = runner->getPublishedInputPortWithName("Phase");
			QVERIFY(phasePort);
			QCOMPARE(phasePort->getName().c_str(), "Phase");
			QCOMPARE(phasePort->getType().c_str(), "VuoReal");
			QCOMPARE(json_object_get_string(phasePort->getDetails()), VUO_STRINGIFY({ "suggestedMin": 0, "suggestedMax": 1, "default": 0.0, "suggestedStep": 0.1 }));
		}

		// A port with an enum menu.
		{
			VuoRunner::Port *wavePort = runner->getPublishedInputPortWithName("Wave");
			QVERIFY(wavePort);
			QCOMPARE(wavePort->getName().c_str(), "Wave");
			QCOMPARE(wavePort->getType().c_str(), "VuoWave");
			QCOMPARE(json_object_get_string(wavePort->getDetails()), VUO_STRINGIFY({ "menuItems": [ { "value": "sine", "name": "Sine" }, { "value": "triangle", "name": "Triangle" }, { "value": "sawtooth", "name": "Sawtooth" } ], "default": "sine" }));
		}

		// A port with a VuoInteger "easy-enum" menu (https://b33p.net/kosada/node/9903).
		{
			VuoRunner::Port *csPort = runner->getPublishedInputPortWithName("CoordinateSpace");
			QVERIFY(csPort);
			QCOMPARE(csPort->getName().c_str(), "CoordinateSpace");
			QCOMPARE(csPort->getType().c_str(), "VuoInteger");
			QCOMPARE(json_object_get_string(csPort->getDetails()), VUO_STRINGIFY({ "menuItems": [ { "value": 0, "name": "World" }, { "value": 1, "name": "Local" } ], "default": 0 }));
		}

		runner->stop();

		delete runner;
	}

	/**
	 * Measures how long a ZMQ IPC REQ/REP roundtrip takes.
	 */
	void testZMQReqRepPerformance_data(void)
	{
		QTest::addColumn<int>("dataParts");
		QTest::addColumn<int>("dataSizePerPart");

		QTest::newRow("1x4")    << 1 << 4;

		QTest::newRow("1x16")   << 1 << 16;
		QTest::newRow("4x4")    << 4 << 4;
		QTest::newRow("16x1")   << 16 << 1;

		QTest::newRow("1x256")  << 1 << 256;
		QTest::newRow("16x16")  << 16 << 16;
		QTest::newRow("256x1")  << 256 << 1;

		QTest::newRow("1x1024") << 1 << 1024;
		QTest::newRow("2x512")  << 2 << 512;
		QTest::newRow("4x256")  << 4 << 256;
		QTest::newRow("32x32")  << 32 << 32;
		QTest::newRow("256x4")  << 256 << 4;
		QTest::newRow("512x2")  << 512 << 2;
		QTest::newRow("1024x1") << 1024 << 1;
	}
	void testZMQReqRepPerformance(void)
	{
		QFETCH(int, dataParts);
		QFETCH(int, dataSizePerPart);

		char *port = strdup(("ipc://" + VuoFileUtilities::makeTmpFile("v", "")).c_str());

		pid_t pid = fork();
		if (pid == 0)
		{
			// Composition process

			void *context = zmq_init(1);
			QVERIFY(context);

			void *control = zmq_socket(context, ZMQ_REP);
			QVERIFY(control);
			QVERIFY(zmq_bind(control, port) == 0);

			forever
			{
				zmq_pollitem_t items[]={
					{control,0,ZMQ_POLLIN,0},
				};
				zmq_poll(items, 1, -1);

				zmq_msg_t message;
				char data[1024];
				size_t size;

				// Receive command from host.
				for (int i = 0; i < dataParts; ++i)
				{
					zmq_msg_init(&message);
					zmq_msg_recv(&message, control, 0);
					size = zmq_msg_size(&message);
					QCOMPARE((size_t)dataSizePerPart, size);
					memcpy(data, zmq_msg_data(&message), size);
					zmq_msg_close(&message);
				}

				// Send reply to host.
				for (int i = 0; i < dataParts; ++i)
				{
					zmq_msg_init_size(&message, size);
					memcpy(zmq_msg_data(&message), &data, size);
					QVERIFY(zmq_msg_send(&message, control, i < dataParts-1 ? ZMQ_SNDMORE : 0) != -1);
					zmq_msg_close(&message);
				}
			}

			zmq_close(control);
			zmq_ctx_term(context);
		}
		else
		{
			// Host process

			void *context = zmq_init(1);
			QVERIFY(context);

			void *control = zmq_socket(context, ZMQ_REQ);
			QVERIFY(control);
			QVERIFY(zmq_connect(control, port) == 0);

			QBENCHMARK
			{
				// Send command to composition.
				zmq_msg_t message;
				char data[1024];
				for (int i = 0; i < dataParts; ++i)
				{
					zmq_msg_init_size(&message, dataSizePerPart);
					memcpy(zmq_msg_data(&message), &data, dataSizePerPart);
					QVERIFY(zmq_msg_send(&message, control, i < dataParts-1 ? ZMQ_SNDMORE : 0) != -1);
					zmq_msg_close(&message);
				}

				// Receive reply from composition.
				for (int i = 0; i < dataParts; ++i)
				{
					zmq_msg_init(&message);
					zmq_msg_recv(&message, control, 0);
					size_t size = zmq_msg_size(&message);
					QCOMPARE((size_t)dataSizePerPart, size);
					memcpy(&data, zmq_msg_data(&message), size);
					zmq_msg_close(&message);
				}
			}

			kill(pid, SIGTERM);
			zmq_close(control);
			zmq_ctx_term(context);
		}

		free(port);
	}

	/**
	 * Tests performance of a recommended implementation of a host app
	 * that runs an image generator with varying numbers of published input ports.
	 */
	void testImageGeneratorPerformance_data(void)
	{
		QTest::addColumn<QString>("composition");

		QTest::newRow("no published inputs") << "imagegenerator-0.vuo";
		QTest::newRow("4 real inputs") << "imagegenerator-real-4.vuo";
		QTest::newRow("4 color inputs") << "imagegenerator-color-4.vuo";
		QTest::newRow("16 real inputs") << "imagegenerator-real-16.vuo";
		QTest::newRow("16 color inputs") << "imagegenerator-color-16.vuo";
	}
	void testImageGeneratorPerformance(void)
	{
		QFETCH(QString, composition);

		VuoCompilerIssues *issues = NULL;
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(("composition/" + composition).toUtf8().data(), issues);
		QVERIFY(runner);

		runner->setRuntimeChecking(true);
		runner->start();

		vector<VuoRunner::Port *> inputPortsVec = runner->getPublishedInputPorts();
		set<VuoRunner::Port *> inputPorts(inputPortsVec.begin(), inputPortsVec.end());
		QVERIFY(inputPorts.size());

		VuoRunner::Port *outputPort = runner->getPublishedOutputPortWithName("outputImage");
		QVERIFY(outputPort);

		map<VuoRunner::Port *, json_object *> portsAndValues;
		for (auto *port : inputPorts)
			portsAndValues[port] = NULL;

		QBENCHMARK
		{
			runner->setPublishedInputPortValues(portsAndValues);

			runner->firePublishedInputPortEvent(inputPorts);
			runner->waitForFiredPublishedInputPortEvent();

			json_object *outputValue = runner->getPublishedOutputPortValue(outputPort);
			QVERIFY(outputValue);
			VuoImage outputImage = VuoImage_makeFromJson(outputValue);
			QVERIFY(outputImage);
			VuoRetain(outputImage);
			VuoRelease(outputImage);
		}

		runner->stop();

		delete runner;
	}
};

QTEST_APPLESS_MAIN(TestVuoRunner)
#include "TestVuoRunner.moc"
