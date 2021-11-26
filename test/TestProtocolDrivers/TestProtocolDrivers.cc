/**
 * @file
 * TestProtocolDrivers interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

#include "TestCompositionExecution.hh"

/**
 * Tests that protocol compositions will run as expected in Vuo Editor and exported apps.
 */
class TestProtocolDrivers : public TestCompositionExecution
{
	Q_OBJECT

private:

	class TestProtocolDriversRunnerDelegate : public VuoRunnerDelegateAdapter
	{
	public:
		QMap<QString, int> outputEvents;
		VuoRunner *runner;

		TestProtocolDriversRunnerDelegate(VuoRunner *runner)
		{
			this->runner = runner;
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			++outputEvents[QString::fromStdString(port->getName())];

			if (port->getName() == "protocolTime" && outputEvents[QString::fromStdString(port->getName())] == 11)
			{
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	// Expected behavior: https://b33p.net/kosada/node/12369
	void testPublishedInputEvents_data()
	{
		QTest::addColumn<QString>("protocolName");
		QTest::addColumn<QStringList>("expectingOneEvent");
		QTest::addColumn<QStringList>("expectingFewEvents");
		QTest::addColumn<QStringList>("expectingAllEvents");

		QStringList comparable;
		comparable << "point2d" << "point3d" << "point4d" << "boolean" << "color" << "integer" << "imageOut" << "real" << "text";

		QStringList nonComparable;
		nonComparable << "layer" << "sceneObject";

		{
			QStringList protocolAllEvents;
			protocolAllEvents << "protocolTime" << "protocolImage";

			QTest::newRow("image filter") << QString::fromStdString(VuoProtocol::imageFilter) << comparable << QStringList() << nonComparable + protocolAllEvents;
		}
		{
			QStringList protocolAllEvents;
			protocolAllEvents << "protocolTime";
			QStringList protocolFewEvents;
			protocolFewEvents << "protocolWidth" << "protocolHeight";

			QTest::newRow("image generator") << QString::fromStdString(VuoProtocol::imageGenerator) << comparable << protocolFewEvents << nonComparable + protocolAllEvents;
		}
		{
			QStringList protocolAllEvents;
			protocolAllEvents << "protocolTime" << "protocolProgress" << "protocolStartImage" << "protocolEndImage";

			QTest::newRow("image transition") << QString::fromStdString(VuoProtocol::imageTransition) << comparable << QStringList() << nonComparable + protocolAllEvents;
		}
	}
	void testPublishedInputEvents()
	{
		QFETCH(QString, protocolName);
		QFETCH(QStringList, expectingOneEvent);
		QFETCH(QStringList, expectingFewEvents);
		QFETCH(QStringList, expectingAllEvents);

		VuoCompiler *compiler = new VuoCompiler();

		VuoCompilerDriver *driver = VuoCompilerDriver::driverForProtocol(compiler, protocolName.toStdString());

		string compositionName = protocolName.toStdString() + "CommonTypes";
		string compositionPath = getCompositionPath(compositionName + ".vuo");
		string compositionAsString = VuoFileUtilities::readFileToString(compositionPath);
		VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionAsString, compiler);
		driver->applyToComposition(composition, compiler);

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionName, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(compositionName + "-linked", "");
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(composition, compiledCompositionPath, true, issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		delete issues;
		delete compiler;

		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, "", false, true);
		TestProtocolDriversRunnerDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->setRuntimeChecking(true);

		runner->startPaused();
		runner->subscribeToAllTelemetry("");
		runner->unpause();
		runner->waitUntilStopped();
		delete runner;

		QStringList actualEvents = delegate.outputEvents.keys();
		QStringList expectedEvents = expectingOneEvent + expectingFewEvents + expectingAllEvents;
		actualEvents.sort();
		expectedEvents.sort();
		QCOMPARE(actualEvents, expectedEvents);

		for (QString portName : expectingOneEvent)
			QVERIFY2(delegate.outputEvents[portName] == 1, QString("%1: %2").arg(portName).arg(delegate.outputEvents[portName]).toStdString().c_str());

		for (QString portName : expectingFewEvents)
			QVERIFY2(2 <= delegate.outputEvents[portName] && delegate.outputEvents[portName] <= 3, QString("%1: %2").arg(portName).arg(delegate.outputEvents[portName]).toStdString().c_str());

		for (QString portName : expectingAllEvents)
			QVERIFY2(delegate.outputEvents[portName] >= 10, QString("%1: %2").arg(portName).arg(delegate.outputEvents[portName]).toStdString().c_str());
	}
};

QTEST_APPLESS_MAIN(TestProtocolDrivers)
#include "TestProtocolDrivers.moc"
