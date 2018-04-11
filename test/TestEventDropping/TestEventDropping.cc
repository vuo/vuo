/**
 * @file
 * TestEventDropping interface and implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>


/**
 * Tests that triggers drop events when they're supposed to, and benchmarks counts of events dropped.
 */
class TestEventDropping : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;

	class TestEventDroppingRunnerDelegate : public TestRunnerDelegate
	{
	public:
		map<string, int> eventsFired;
		map<string, int> eventsDropped;

		TestEventDroppingRunnerDelegate(string compositionName, VuoCompiler *compiler)
		{
			string compositionPath = getCompositionPath(compositionName + ".vuo");

			string directory, file, extension;
			VuoFileUtilities::splitPath(compositionPath, directory, file, extension);
			string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
			string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file + "-linked", "");
			compiler->compileComposition(compositionPath, compiledCompositionPath);
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
			remove(compiledCompositionPath.c_str());
			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, directory, false, true);

			runner->setDelegate(this);
			runner->start();
			runner->subscribeToEventTelemetry();

			sleep(5);

			runner->stop();
			delete runner;
		}

		void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
		{
			if (portIdentifier.find("fired") != string::npos)
				++eventsFired[portIdentifier];
		}

		void receivedTelemetryEventDropped(string portIdentifier)
		{
			if (portIdentifier.find("fired") != string::npos)
				++eventsDropped[portIdentifier];
		}
	};

private slots:

	void initTestCase()
	{
		compiler = initCompiler();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	void testWhetherEventsDropped_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QString >("triggerNodeIdentifier");
		QTest::addColumn< bool >("shouldDrop");

		QTest::newRow("Fast-firing drop trigger overlapping slow-firing enqueue trigger immediately") << "FastDropSlowEnqueueImmediately" << "FirePeriodically1" << true;
		QTest::newRow("Fast-firing enqueue trigger with slow nodes downstream") << "FastEnqueueSlowOutgoing" << "FirePeriodically1" << false;
		QTest::newRow("Fast-firing drop trigger with slow nodes downstream") << "FastDropSlowOutgoing" << "FirePeriodically1" << true;
		QTest::newRow("Slow-firing drop trigger with no outgoing cables") << "SlowDropNoOutgoing" << "FirePeriodically1" << false;
	}
	void testWhetherEventsDropped()
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, triggerNodeIdentifier);
		QFETCH(bool, shouldDrop);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		TestEventDroppingRunnerDelegate delegate(compositionName.toStdString(), compiler);

		bool wereEventsFired = false;
		for (map<string, int>::iterator i = delegate.eventsFired.begin(); i != delegate.eventsFired.end(); ++i)
		{
			string portIdentifier = i->first;
			if (portIdentifier.find(triggerNodeIdentifier.toStdString()) != string::npos)
			{
				wereEventsFired = true;
				break;
			}
		}
		QVERIFY(wereEventsFired);

		int numEventsDropped = 0;
		for (map<string, int>::iterator i = delegate.eventsDropped.begin(); i != delegate.eventsDropped.end(); ++i)
		{
			string portIdentifier = i->first;
			if (portIdentifier.find(triggerNodeIdentifier.toStdString()) != string::npos)
			{
				numEventsDropped = i->second;
				break;
			}
		}
		QVERIFY2(shouldDrop ? numEventsDropped > 0 : numEventsDropped == 0, QString("%1 events dropped").arg(numEventsDropped).toUtf8().data());
	}

	void testPercentOfEventsDropped_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QStringList >("subcompositionNames");
		QTest::addColumn< QString >("triggerNodeIdentifier");

		QStringList noSubcompositions;

		QTest::newRow("2 fast-firing overlapping immediately") << "2FastDropsImmediately" << noSubcompositions << "";
		QTest::newRow("2 fast-firing overlapping downstream") << "2FastDropsDownstream" << noSubcompositions << "";
		QTest::newRow("1 fast-firing overlapping 1 slow-firing immediately") << "FastDropSlowDropImmediately" << noSubcompositions << "FirePeriodically1";
		QTest::newRow("1 slow-firing overlapping 1 fast-firing immediately") << "FastDropSlowDropImmediately" << noSubcompositions << "FirePeriodically2";
		QTest::newRow("50 triggers") << "ManyTriggers" << noSubcompositions << "";
		{
			QStringList subcompositions;
			subcompositions.append("vuo.test.passthru1");
			subcompositions.append("vuo.test.passthru2");
			subcompositions.append("vuo.test.passthru3");
			subcompositions.append("vuo.test.passthru4");
			subcompositions.append("vuo.test.passthru5");
			QTest::newRow("1 trigger into 10 multi-level subcompositions") << "ManyPassthruSubcompositions" << subcompositions << "";
		}
		{
			QStringList subcompositions;
			subcompositions.append("vuo.test.passthru1");
			subcompositions.append("vuo.test.passthru2");
			subcompositions.append("vuo.test.passthru3");
			subcompositions.append("vuo.test.passthru4");
			subcompositions.append("vuo.test.passthru5");
			QTest::newRow("10 triggers each into a multi-level subcomposition") << "ManyPassthruSubcompositionsAndTriggers" << subcompositions << "";
		}
		{
			QStringList subcompositions;
			subcompositions.append("vuo.test.fireFromWithin1");
			subcompositions.append("vuo.test.fireFromWithin2");
			subcompositions.append("vuo.test.fireFromWithin3");
			subcompositions.append("vuo.test.fireFromWithin4");
			subcompositions.append("vuo.test.fireFromWithin5");
			QTest::newRow("5 multi-level subcompositions that fire events, drop at top level") << "ManyFiringSubcompositions_drop" << subcompositions << "";
		}
		{
			QStringList subcompositions;
			subcompositions.append("vuo.test.fireFromWithin1_drop");
			subcompositions.append("vuo.test.fireFromWithin2_drop");
			subcompositions.append("vuo.test.fireFromWithin3_drop");
			subcompositions.append("vuo.test.fireFromWithin4_drop");
			subcompositions.append("vuo.test.fireFromWithin5_drop");
			QTest::newRow("5 multi-level subcompositions that fire events, drop at bottom level") << "ManyFiringSubcompositions" << subcompositions << "";
		}
	}
	void testPercentOfEventsDropped()
	{
		QFETCH(QString, compositionName);
		QFETCH(QStringList, subcompositionNames);
		QFETCH(QString, triggerNodeIdentifier);
		printf("	%s\n", compositionName.toUtf8().data()); fflush(stdout);

		foreach (QString s, subcompositionNames)
		{
			string nodeClassName = s.toStdString();
			string subcompositionPath = getCompositionPath(nodeClassName + ".vuo");
			installSubcomposition(subcompositionPath, nodeClassName, compiler);
		}

		TestEventDroppingRunnerDelegate delegate(compositionName.toStdString(), compiler);

		int numEventsFired = 0;
		for (map<string, int>::iterator i = delegate.eventsFired.begin(); i != delegate.eventsFired.end(); ++i)
		{
			string portIdentifier = i->first;
			if (triggerNodeIdentifier.isEmpty() || portIdentifier.find(triggerNodeIdentifier.toStdString()) != string::npos)
			{
				numEventsFired = i->second;
				break;
			}
		}

		int numEventsDropped = 0;
		for (map<string, int>::iterator i = delegate.eventsDropped.begin(); i != delegate.eventsDropped.end(); ++i)
		{
			string portIdentifier = i->first;
			if (triggerNodeIdentifier.isEmpty() || portIdentifier.find(triggerNodeIdentifier.toStdString()) != string::npos)
			{
				numEventsDropped = i->second;
				break;
			}
		}
		QVERIFY(numEventsFired > 0 || numEventsDropped > 0);

		// This benchmark result actually represents % of total events that are dropped. QTest::Events was the closest option available.
		QTest::setBenchmarkResult(100 * numEventsDropped / (qreal)(numEventsFired + numEventsDropped), QTest::Events);

		foreach (QString s, subcompositionNames)
		{
			string nodeClassName = s.toStdString();
			uninstallSubcomposition(nodeClassName, compiler);
		}
	}
};


QTEST_APPLESS_MAIN(TestEventDropping)
#include "TestEventDropping.moc"
