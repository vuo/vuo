/**
 * @file
 * TestNodeExecutionOrder interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <sstream>


/**
 * Represents information about the execution of a node during a test.
 */
typedef struct {
	string triggerID;
	unsigned int eventCount;
	string nodeID;
} nodeExecution_t;

typedef vector<string> sequence_t;  ///< A sequence of nodeIDs.
typedef map<string, set<sequence_t> > sequencesForTriggerMapping_t;  ///< Maps each triggerID to 0 or more sequences of nodeIDs.

/**
 * A runner delegate that executes a composition, triggering random events on the published input ports
 * and recording the output port values of all vuo.test.conductor nodes' nodeInfo ports.
 */
class TestNodeExecutionOrderRunnerDelegate : public VuoRunnerDelegateAdapter
// class TestNodeExecutionOrderRunnerDelegate : public TestRunnerDelegate   /// @todo https://b33p.net/kosada/node/6021
{
private:
	VuoRunner *runner;
	bool isStopping;

public:
	vector<nodeExecution_t> nodeExecutions;

	TestNodeExecutionOrderRunnerDelegate(string compositionPath, VuoCompiler *compiler)
	{
		isStopping = false;

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition-linked", "");
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);
		compiler->compileComposition(generator, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath);
		remove(compiledCompositionPath.c_str());
		runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, true);

		runner->setDelegate(this);
		runner->start();

		VuoRunner::Port *startPort = runner->getPublishedInputPortWithName("start");
		runner->firePublishedInputPortEvent(startPort);

		runner->waitUntilStopped();

		const size_t NUM_EVENTS = 100;
		size_t maxNumNodeExecutions = generator->composition->getBase()->getNodes().size() * 2 * NUM_EVENTS;  // For each event, each node can be executed at most twice (feedback).
		QVERIFY2(NUM_EVENTS <= nodeExecutions.size() && nodeExecutions.size() <= maxNumNodeExecutions,
				 QString("%1 <= %2 <= %3").arg(NUM_EVENTS).arg(nodeExecutions.size()).arg(maxNumNodeExecutions).toUtf8().constData());
		delete generator;
	}

	void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
	{
		if (! VuoStringUtilities::beginsWith(portIdentifier, "vuo_test_conductor") && ! VuoStringUtilities::beginsWith(portIdentifier, "vuo_test_semiconductor"))
			return;

		if (portIdentifier.find("nodeInfo") == string::npos)
			return;

		QVERIFY(dataSummary.find("...") == string::npos);

		const char *SENTINEL = "*";
		if (VuoStringUtilities::beginsWith(dataSummary, SENTINEL))
		{
			if (! isStopping)
			{
				// runner->stop() has to be called asynchronously because it waits for this function to return.
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
				isStopping = true;
			}
			return;
		}

		nodeExecution_t nodeExecution;
		istringstream sin(dataSummary);
		sin >> nodeExecution.triggerID >> nodeExecution.eventCount >> nodeExecution.nodeID;

		if (nodeExecution.triggerID.empty() || nodeExecution.nodeID.empty())
			QFAIL(dataSummary.c_str());

		nodeExecutions.push_back(nodeExecution);
	}
};


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(sequencesForTriggerMapping_t);
Q_DECLARE_METATYPE(vector<nodeExecution_t>);
Q_DECLARE_METATYPE(set<sequence_t>);
Q_DECLARE_METATYPE(vector<string>);


/**
 * Tests that nodes in the composition execute in a correct order.
 */
class TestNodeExecutionOrder : public TestCompositionExecution
{
	Q_OBJECT

protected:

	/**
	 * Prints each nodeExecution_t in the list. For debugging.
	 */
	void printNodeExecutions(const vector<nodeExecution_t> &nodeExecutions)
	{
		for (vector<nodeExecution_t>::const_iterator i = nodeExecutions.begin(); i != nodeExecutions.end(); ++i)
			printf("nodeExecution: %s %u %s\n", i->triggerID.c_str(), i->eventCount, i->nodeID.c_str());
	}

	/**
	 * Checks that every trigger-node combination in @c expectedSequences got executed at least once.
	 */
	void checkCoverage(const sequencesForTriggerMapping_t &sequencesForTrigger, const vector<nodeExecution_t> &nodeExecutions)
	{
		try
		{
			for (sequencesForTriggerMapping_t::const_iterator i = sequencesForTrigger.begin(); i != sequencesForTrigger.end(); ++i)
			{
				string triggerID = i->first;
				set<sequence_t> expectedSequences = i->second;

				for (set<sequence_t>::const_iterator j = expectedSequences.begin(); j != expectedSequences.end(); ++j)
				{
					sequence_t expectedSequence = *j;

					for (sequence_t::const_iterator k = expectedSequence.begin(); k != expectedSequence.end(); ++k)
					{
						string nodeID = *k;

						bool foundTriggerNode = false;
						for (vector<nodeExecution_t>::const_iterator m = nodeExecutions.begin(); m != nodeExecutions.end(); ++m)
						{
							nodeExecution_t nodeExecution = *m;
							if (nodeExecution.triggerID == triggerID && nodeExecution.nodeID == nodeID)
							{
								foundTriggerNode = true;
								break;
							}
						}

						if (! foundTriggerNode)
						{
							throw QString("%1:%2: not executed")
									.arg(triggerID.c_str())
									.arg(nodeID.c_str());
						}
					}
				}
			}
		}
		catch (QString message)
		{
			// EXPECT_FAIL tests need this function to make just one assertion, since EXPECT_FAIL only applies to the subsequent assertion.
			QVERIFY2(false, qPrintable(message));
		}
	}

	/**
	 * Checks that the actual sequence of nodes executed conforms to the partial ordering defined in @c sequencesForTrigger.
	 */
	void checkSequences(const sequencesForTriggerMapping_t &sequencesForTrigger, const vector<nodeExecution_t> &nodeExecutions)
	{
		try
		{
			map<string, int> eventCountForTrigger;
			map<string, vector<nodeExecution_t> > nodeExecutionsInProgress;

			for (vector<nodeExecution_t>::const_iterator i = nodeExecutions.begin(); i != nodeExecutions.end(); ++i)
			{
				nodeExecution_t nodeExecution = *i;

				sequencesForTriggerMapping_t::const_iterator expectedSequencesIter = sequencesForTrigger.find(nodeExecution.triggerID);
				if (expectedSequencesIter == sequencesForTrigger.end())
					throw QString("%1: unknown trigger")
						.arg(nodeExecution.triggerID.c_str());
				set<sequence_t> expectedSequences = expectedSequencesIter->second;

				unsigned int prevEventCount = eventCountForTrigger[nodeExecution.triggerID];
				eventCountForTrigger[nodeExecution.triggerID] = nodeExecution.eventCount;

				// Check the previous event, since nodeExecution begins a new event for this trigger.
				if (nodeExecution.eventCount != prevEventCount)
				{
					if (nodeExecution.eventCount != prevEventCount + 1)
						throw QString("%1: found %2, expected %3")
							.arg(nodeExecution.triggerID.c_str())
							.arg(nodeExecution.eventCount)
							.arg(prevEventCount + 1);

					checkSequence(expectedSequences, nodeExecutionsInProgress[nodeExecution.triggerID]);

					nodeExecutionsInProgress[nodeExecution.triggerID].clear();
				}

				nodeExecutionsInProgress[nodeExecution.triggerID].push_back(nodeExecution);
			}

			// Check any remaining events.
			for (map<string, vector<nodeExecution_t> >::iterator i = nodeExecutionsInProgress.begin(); i != nodeExecutionsInProgress.end(); ++i)
			{
				string triggerID = i->first;

				sequencesForTriggerMapping_t::const_iterator expectedSequencesIter = sequencesForTrigger.find(triggerID);
				if (expectedSequencesIter == sequencesForTrigger.end())
					throw QString("%1: unknown trigger")
						.arg(triggerID.c_str());
				set<sequence_t> expectedSequences = expectedSequencesIter->second;

				checkSequence(expectedSequences, i->second);
			}
		}
		catch (QString message)
		{
			// EXPECT_FAIL tests need this function to make just one assertion, since EXPECT_FAIL only applies to the subsequent assertion.
			QVERIFY2(false, qPrintable(message));
		}
	}

	/**
	 * Helper function for @c checkSequences.
	 *
	 * Checks that each of the expected node sequences for this trigger, if executed for this event,
	 * was executed in an acceptable order. That is, nodes in each expected node sequence were executed
	 * in sequential order (though not necessarily consecutively), and no nodes in the middle were skipped.
	 */
	void checkSequence(const set<sequence_t> &expectedSequences, const vector<nodeExecution_t> &nodeExecutionsForThisEvent)
	{
		// Check that nodeExecutionsForThisEvent contains no extraneous nodes.
		for (vector<nodeExecution_t>::const_iterator i = nodeExecutionsForThisEvent.begin(); i != nodeExecutionsForThisEvent.end(); ++i)
		{
			nodeExecution_t nodeExecution = *i;

			bool foundNodeInExpectedSequences = false;
			for (set<sequence_t>::const_iterator j = expectedSequences.begin(); j != expectedSequences.end(); ++j)
			{
				sequence_t expectedSequence = *j;
				if (find(expectedSequence.begin(), expectedSequence.end(), nodeExecution.nodeID) != expectedSequence.end())
				{
					foundNodeInExpectedSequences = true;
					break;
				}
			}

			if (! foundNodeInExpectedSequences)
				throw QString("%1:%2:%3: node shouldn't have been executed by this trigger")
						.arg(nodeExecution.triggerID.c_str())
						.arg(nodeExecution.eventCount)
						.arg(nodeExecution.nodeID.c_str());
		}

		// Check that the nodes in nodeExecutionsForThisEvent are in an acceptable order.
		for (set<sequence_t>::iterator j = expectedSequences.begin(); j != expectedSequences.end(); ++j)
		{
			int prevIndexInNodeExecutions = 0;
			for (sequence_t::const_iterator k = j->begin(); k != j->end(); ++k)
			{
				string expectedNodeID = *k;

				// Find expectedNodeID in nodeExecutionsForThisEvent.
				int currIndexInNodeExecutions = 0;
				for (vector<nodeExecution_t>::const_iterator m = nodeExecutionsForThisEvent.begin(); m != nodeExecutionsForThisEvent.end(); ++m, ++currIndexInNodeExecutions)
					if (m->nodeID == expectedNodeID)
						break;

				// Check that either the current node was executed after the previous node in the sequence or else the current node wasn't executed.
				if (prevIndexInNodeExecutions > currIndexInNodeExecutions)
				{
					throw QString("%1:%2: node %3 executed too early")
							.arg(nodeExecutionsForThisEvent.at(0).triggerID.c_str())
							.arg(nodeExecutionsForThisEvent.at(0).eventCount)
							.arg(expectedNodeID.c_str());
				}

				prevIndexInNodeExecutions = currIndexInNodeExecutions;
			}
		}
	}

	/**
	 * Checks that the actual sequence of nodes executed respects the critical sections defined in @c criticalSections.
	 * Any linear chain of nodes that may be triggered by more than one trigger is a critical section, since only one
	 * trigger's event should be inside that linear chain of nodes at any given time.
	 */
	void checkCriticalSections(const set<sequence_t> &criticalSections, const vector<nodeExecution_t> &nodeExecutions)
	{
		try
		{
			map<string, unsigned int> eventCountForTrigger;
			map<string, vector<nodeExecution_t> > nodeExecutionsInProgress;

			for (vector<nodeExecution_t>::const_iterator i = nodeExecutions.begin(); i != nodeExecutions.end(); ++i)
			{
				nodeExecution_t nodeExecution = *i;

				unsigned int prevEventCount = eventCountForTrigger[nodeExecution.triggerID];
				eventCountForTrigger[nodeExecution.triggerID] = nodeExecution.eventCount;

				if (nodeExecution.eventCount > prevEventCount)
				{
					// Check that when the trigger finished the previous event, it wasn't in the middle of a critical section.
					vector<nodeExecution_t> nodeExecutionsForPrevEvent = nodeExecutionsInProgress[nodeExecution.triggerID];
					for (set<sequence_t>::const_iterator j = criticalSections.begin(); j != criticalSections.end(); ++j)
					{
						sequence_t criticalSection = *j;
						string firstNode = criticalSection.at(0);
						string lastNode = criticalSection.at(criticalSection.size() - 1);

						// Search for the first and last node of the critical section in nodeExecutionsForPrevEvent.
						bool foundFirstNode = false;
						bool foundLastNode = false;
						for (vector<nodeExecution_t>::iterator k = nodeExecutionsForPrevEvent.begin(); k != nodeExecutionsForPrevEvent.end(); ++k)
						{
							if (k->nodeID == firstNode)
								foundFirstNode = true;
							if (k->nodeID == lastNode)
								foundLastNode = true;
						}

						if (foundFirstNode && !foundLastNode)
							throw QString("%1:%2:%3: overlaps critical section of %4:%5")
								.arg(nodeExecution.triggerID.c_str())
								.arg(nodeExecution.eventCount)
								.arg(nodeExecution.nodeID.c_str())
								.arg(nodeExecution.triggerID.c_str())
								.arg(prevEventCount);
					}

					nodeExecutionsInProgress[nodeExecution.triggerID].clear();
				}

				nodeExecutionsInProgress[nodeExecution.triggerID].push_back(nodeExecution);

				// Check that nodeExecution doesn't overlap with any critical sections in nodeExecutionsInProgress.
				checkCriticalSection(criticalSections, nodeExecutionsInProgress, nodeExecution);
			}
		}
		catch (QString message)
		{
			// EXPECT_FAIL tests need this function to make just one assertion, since EXPECT_FAIL only applies to the subsequent assertion.
			QVERIFY2(false, qPrintable(message));
		}
	}

	/**
	 * Helper function for @c checkCriticalSections.
	 *
	 * Checks that the event for @c nodeExecution is the only event inside of @c nodeExecution's critical sections right now.
	 */
	void checkCriticalSection(const set<sequence_t> &criticalSections,
							  const map<string, vector<nodeExecution_t> > &nodeExecutionsInProgress,
							  nodeExecution_t nodeExecution)
	{
		for (set<sequence_t>::const_iterator i = criticalSections.begin(); i != criticalSections.end(); ++i)
		{
			sequence_t criticalSection = *i;

			// Does the critical section contain the node?
			if (find(criticalSection.begin(), criticalSection.end(), nodeExecution.nodeID) == criticalSection.end())
				break;

			// Is any other trigger inside the critical section right now?
			string lastNodeInCriticalSection = criticalSection.at(criticalSection.size() - 1);
			for (map<string, vector<nodeExecution_t> >::const_iterator j = nodeExecutionsInProgress.begin(); j != nodeExecutionsInProgress.end(); ++j)
			{
				vector<nodeExecution_t> nodeExecutions = j->second;

				if (nodeExecutions.empty() || nodeExecutions.at(0).triggerID == nodeExecution.triggerID)
					continue;

				// Search for nodeExecution.nodeID in nodeExecutionsInProgress for the current trigger.
				bool foundCurrNode = false;
				vector<nodeExecution_t>::iterator k;
				for (k = nodeExecutions.begin(); k != nodeExecutions.end(); ++k)
				{
					if (k->nodeID == nodeExecution.nodeID)
					{
						foundCurrNode = true;
						break;
					}
				}
				if (! foundCurrNode)
					continue;

				// Search for lastNodeInCriticalSection in nodeExecutionsInProgress for the current trigger.
				bool foundLastNode = false;
				for ( ; k != nodeExecutions.end(); ++k)
				{
					if (k->nodeID == lastNodeInCriticalSection)
					{
						foundLastNode = true;
						break;
					}
				}

				if (! foundLastNode)
					throw QString("%1:%2:%3 overlaps critical section of %4:%5\n")
						.arg(nodeExecution.triggerID.c_str())
						.arg(nodeExecution.eventCount)
						.arg(nodeExecution.nodeID.c_str())
						.arg(nodeExecutions.at(0).triggerID.c_str())
						.arg(nodeExecutions.at(0).eventCount);
			}
		}
	}

private slots:

	void testCheckCoverage_data()
	{
		QTest::addColumn< sequencesForTriggerMapping_t >("sequencesForTrigger");
		QTest::addColumn< vector<nodeExecution_t> >("nodeExecutions");

		sequencesForTriggerMapping_t sequencesForTrigger;
		sequence_t seq1;
		seq1.push_back("A");
		seq1.push_back("C");
		sequence_t seq2;
		seq2.push_back("B");
		seq2.push_back("D");
		sequencesForTrigger["Gen1"].insert(seq1);
		sequencesForTrigger["Gen1"].insert(seq2);
		sequencesForTrigger["Gen2"].insert(seq2);

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});

			QTest::newRow("valid node executions") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});

			QTest::newRow("SHOULD FAIL - Gen2:D executed prematurely") << sequencesForTrigger << nodeExecutions;
		}
	}
	void testCheckCoverage()
	{
		QFETCH(sequencesForTriggerMapping_t, sequencesForTrigger);
		QFETCH(vector<nodeExecution_t>, nodeExecutions);

		QEXPECT_FAIL("SHOULD FAIL - Gen2:D executed prematurely", "", Continue);

		checkCoverage(sequencesForTrigger, nodeExecutions);
	}

	void testCheckSequences_data()
	{
		QTest::addColumn< sequencesForTriggerMapping_t >("sequencesForTrigger");
		QTest::addColumn< vector<nodeExecution_t> >("nodeExecutions");

		map<string, set<vector<string> > > sequencesForTrigger;
		vector<string> seq1;
		seq1.push_back("A");
		seq1.push_back("C");
		vector<string> seq2;
		seq2.push_back("B");
		seq2.push_back("D");
		sequencesForTrigger["Gen1"].insert(seq1);
		sequencesForTrigger["Gen1"].insert(seq2);
		sequencesForTrigger["Gen2"].insert(seq2);

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});

			QTest::newRow("valid node executions for 1 event from Gen1 (1)") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});

			QTest::newRow("valid node executions for 1 event from Gen1 (2)") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});

			QTest::newRow("valid node executions for 1 event from Gen1 and 1 event from Gen2") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 2, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 2, "D"});

			QTest::newRow("valid node executions for 2 events from Gen2") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 2, "A"});

			QTest::newRow("valid node executions for 2 events from Gen1, with non-conductive node A") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});  // wrong

			QTest::newRow("SHOULD FAIL - D before B in event 1") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 2, "D"});  // wrong

			QTest::newRow("SHOULD FAIL - D before B in event 2") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 2, "B"});  // wrong

			QTest::newRow("SHOULD FAIL - event 2 before 1") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 3, "B"});  // wrong

			QTest::newRow("SHOULD FAIL - event 3 before 2") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 2, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});  // wrong

			QTest::newRow("SHOULD FAIL - event 1 after 2") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen3", 1, "B"});  // wrong

			QTest::newRow("SHOULD FAIL - no such trigger") << sequencesForTrigger << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "A"});  // wrong

			QTest::newRow("SHOULD FAIL - A triggered by Gen2") << sequencesForTrigger << nodeExecutions;
		}
	}
	void testCheckSequences()
	{
		QFETCH(sequencesForTriggerMapping_t, sequencesForTrigger);
		QFETCH(vector<nodeExecution_t>, nodeExecutions);

		QEXPECT_FAIL("SHOULD FAIL - D before B in event 1", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - D before B in event 2", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - event 2 before 1", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - event 3 before 2", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - event 1 after 2", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - no such trigger", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - A triggered by Gen2", "", Continue);

		checkSequences(sequencesForTrigger, nodeExecutions);
	}

	void testCheckCriticalSections_data()
	{
		QTest::addColumn< set<sequence_t> >("criticalSections");
		QTest::addColumn< vector<nodeExecution_t> >("nodeExecutions");

		set<vector<string> > criticalSections;
		vector<string> sec;
		sec.push_back("B");
		sec.push_back("D");
		criticalSections.insert(sec);

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});

			QTest::newRow("valid node executions (1)") << criticalSections << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "A"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "C"});

			QTest::newRow("valid node executions (2)") << criticalSections << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 2, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 2, "D"});

			QTest::newRow("valid node executions (3)") << criticalSections << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});

			QTest::newRow("valid node executions (4)") << criticalSections << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen2", 1, "B"});  // wrong
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "D"});

			QTest::newRow("SHOULD FAIL - Gen2 interleaves with Gen1's critical section") << criticalSections << nodeExecutions;
		}

		{
			vector<nodeExecution_t> nodeExecutions;
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 1, "B"});
			nodeExecutions.push_back((nodeExecution_t){"Gen1", 2, "B"});  // wrong

			QTest::newRow("SHOULD FAIL - Gen1:2 interleaves with Gen1:1's critical section") << criticalSections << nodeExecutions;
		}
	}
	void testCheckCriticalSections()
	{
		QFETCH(set<sequence_t>, criticalSections);
		QFETCH(vector<nodeExecution_t>, nodeExecutions);

		QEXPECT_FAIL("SHOULD FAIL - Gen2 interleaves with Gen1's critical section", "", Continue);
		QEXPECT_FAIL("SHOULD FAIL - Gen1:2 interleaves with Gen1:1's critical section", "", Continue);

		checkCriticalSections(criticalSections, nodeExecutions);
	}

	void testNodeExecutionOrder_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< sequencesForTriggerMapping_t >("sequencesForTrigger");
		QTest::addColumn< set<sequence_t> >("criticalSections");

		set<sequence_t> noCriticalSections;

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			sequence_t s;
			s.push_back("Conductor1");
			s.push_back("Conductor2");
			sequencesForTrigger["FirePer1"].insert(s);

			QTest::newRow("Linear chain of nodes") << "LinearChain.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("Z-shaped scatter and gather") << "Z.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("X-shaped scatter and gather") << "X.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			sequence_t s;
			s.push_back("Conductor1");
			s.push_back("Conductor2");
			sequencesForTrigger["FirePer1"].insert(s);

			QTest::newRow("Scatter to 2 nodes and gather at the 2nd node") << "Bypass.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor5");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("Scatter, then gather, then scatter") << "ScatterGatherScatter.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor6");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor7");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor8");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor8");
				s.push_back("Conductor4");
				s.push_back("Conductor9");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor8");
				s.push_back("Conductor5");
				s.push_back("Conductor10");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("Scatter, then gather, then scatter, with linear sequences of nodes in between") << "ScatterGatherScatterExtended.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor5");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor4");
				s.push_back("Conductor5");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				s.push_back("Conductor5");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("Scatter, then scatter, then gather") << "ScatterScatterGather.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			sequence_t s;
			s.push_back("SemiConductor1");
			s.push_back("Conductor1");
			s.push_back("Conductor2");
			sequencesForTrigger["FirePer1"].insert(s);

			QTest::newRow("Feedback loop of 3 nodes.") << "FeedbackIntoSemiconductor.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			sequence_t s;
			s.push_back("Conductor1");
			sequencesForTrigger["FirePer1"].insert(s);

			QTest::newRow("Feedback loop between a trigger node and a conductor node.") << "FeedbackIntoTrigger.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer2"].insert(s);
			}

			set<sequence_t> criticalSections;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				criticalSections.insert(s);
			}

			QTest::newRow("2 triggers pushing events along the same linear path") << "LinearChainTwoTriggers.vuo" << sequencesForTrigger << criticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor2");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer2"].insert(s);
			}

			set<sequence_t> criticalSections;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor2");
				criticalSections.insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor4");
				criticalSections.insert(s);
			}

			QTest::newRow("2 triggers pushing events along overlapping paths") << "LinearChainsTwoOverlappingTriggers.vuo" << sequencesForTrigger << criticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("SemiConductor1");
				s.push_back("Conductor2");
				sequencesForTrigger["FirePer2"].insert(s);
			}

			QTest::newRow("1 trigger pushing a conductive port and 1 trigger pushing a non-conductive port") << "SemiconductorCoordinatingTwoTriggers.vuo" << sequencesForTrigger << noCriticalSections;
		}

		{
			sequencesForTriggerMapping_t sequencesForTrigger;
			{
				sequence_t s;
				s.push_back("Conductor1");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor2");
				s.push_back("Conductor3");
				sequencesForTrigger["FirePer1"].insert(s);
			}
			{
				sequence_t s;
				s.push_back("Conductor3");
				s.push_back("Conductor4");
				sequencesForTrigger["FirePer1"].insert(s);
			}

			QTest::newRow("2 event cables into an input port from 1 trigger") << "MultipleEventCablesFrom1Trigger.vuo" << sequencesForTrigger << noCriticalSections;
		}
	}
	void testNodeExecutionOrder()
	{
		QFETCH(QString, compositionFile);
		QFETCH(sequencesForTriggerMapping_t, sequencesForTrigger);
		QFETCH(set<sequence_t>, criticalSections);

		printf("    %s\n", compositionFile.toUtf8().constData());

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompiler *compiler = initCompiler();
		TestNodeExecutionOrderRunnerDelegate delegate(compositionPath, compiler);

		vector<nodeExecution_t> nodeExecutions = delegate.nodeExecutions;
		checkCoverage(sequencesForTrigger, nodeExecutions);
		checkSequences(sequencesForTrigger, nodeExecutions);
		checkCriticalSections(criticalSections, nodeExecutions);
	}

};

QTEST_APPLESS_MAIN(TestNodeExecutionOrder)
#include "TestNodeExecutionOrder.moc"
