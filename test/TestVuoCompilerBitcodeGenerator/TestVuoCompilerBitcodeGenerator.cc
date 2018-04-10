/**
 * @file
 * TestVuoCompilerBitcodeGenerator interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fstream>
#include "TestVuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCable.hh"
#include "VuoFileUtilities.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"


typedef map<string, set<string> > nodesMap;  ///< Typedef needed for Q_DECLARE_METATYPE and QFETCH to compile. (The comma causes an error.)

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(nodesMap);
Q_DECLARE_METATYPE(set<string>);


/**
 * Tests the compiler's ability to generate code for compositions.
 */
class TestVuoCompilerBitcodeGenerator : public TestVuoCompiler
{
	Q_OBJECT

private:

	/**
	 * Returns a list of string representations of the specified @c chainsForTrigger.
	 */
	vector<string> makeChainStrings(map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger)
	{
		vector<string> chainStrings;
		for (map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> >::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
		{
			for (vector<VuoCompilerChain *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				string s;
				vector<VuoCompilerNode *> nodes = (*j)->getNodes();
				for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
					s += (*i)->getBase()->getTitle() + (i+1 == nodes.end() ? "" : " ");
				chainStrings.push_back(s);
			}
		}
		return chainStrings;
	}

private slots:

	void initTestCase()
	{
		initCompiler();
	}

	void cleanupTestCase()
	{
		cleanupCompiler();
	}

	void testEventMayTransmitThroughNode_data()
	{
		QTest::addColumn< QString >("fromNodeTitle");
		QTest::addColumn< QString >("toNodeTitle");
		QTest::addColumn< bool >("mayTransmitThroughNode");

		QTest::newRow("always-transmitting cable and never-transmitting cable") << "FireonStart1" << "MeasureTime1" << true;
		QTest::newRow("never-transmitting cable") << "FireonStart2" << "MeasureTime2" << false;
		QTest::newRow("sometimes-transmitting cable") << "FireonStart3" << "MeasureTime3" << true;
		QTest::newRow("outgoing cables from trigger ports only") << "FireonStart4" << "FirePeriodically1" << false;
	}
	void testEventMayTransmitThroughNode()
	{
		QFETCH(QString, fromNodeTitle);
		QFETCH(QString, toNodeTitle);
		QFETCH(bool, mayTransmitThroughNode);

		string compositionPath = getCompositionPath("Semiconductor.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		set<VuoCompilerCable *> incomingCables;
		set<VuoCompilerCable *> outgoingCables;
		set<VuoCable *> cables = composition.getBase()->getCables();
		for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
		{
			if ((*i)->getFromNode()->getTitle() == fromNodeTitle.toStdString() && (*i)->getToNode()->getTitle() == toNodeTitle.toStdString())
				incomingCables.insert((*i)->getCompiler());
			if ((*i)->getFromNode()->getTitle() == toNodeTitle.toStdString())
				outgoingCables.insert((*i)->getCompiler());
		}
		QVERIFY(! incomingCables.empty());
		QVERIFY(! outgoingCables.empty());

		// Check if an event coming in through incomingCables (fromNode -> toNode) will transmit
		// through outgoingCables (toNode -> connected node).
		QCOMPARE(graph.mayTransmit(incomingCables, outgoingCables), mayTransmitThroughNode);

		delete parser;
	}

	void testNodesDownstream_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< nodesMap >("expectedDownstreamNodes");

		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["TriggerWithOutput1:trigger"].insert("TriggerWithOutput1");
			downstreamNodes["TriggerWithOutput1:trigger"].insert("Count1");
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("Count2");
			QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("FirePeriodically1");
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("Count3");
			downstreamNodes["FirePeriodically2:fired"].insert("Count2");
			downstreamNodes["FirePeriodically2:fired"].insert("Count3");
			QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			QTest::newRow("0 triggers.") << "Add" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Hold1");
			downstreamNodes["FirePeriodically1:fired"].insert("MakeList1");
			downstreamNodes["FirePeriodically1:fired"].insert("Add1");
			downstreamNodes["FirePeriodically1:fired"].insert("ConvertIntegertoText1");
			downstreamNodes["FirePeriodically1:fired"].insert("DisplayConsoleWindow1");
			downstreamNodes["DisplayConsoleWindow1:typedLine"];
			downstreamNodes["DisplayConsoleWindow1:typedWord"];
			downstreamNodes["DisplayConsoleWindow1:typedCharacter"];
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			downstreamNodes["FirePeriodically:fired"].insert("Hold");
			downstreamNodes["FirePeriodically:fired"].insert("Subtract");
			downstreamNodes["FirePeriodically:fired"].insert("ConvertIntegertoText");
			downstreamNodes["DisplayConsoleWindow:typedLine"];
			downstreamNodes["DisplayConsoleWindow:typedWord"];
			downstreamNodes["DisplayConsoleWindow:typedCharacter"];
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically:fired"].insert("Hold");
			downstreamNodes["FirePeriodically:fired"].insert("SelectInput");
			downstreamNodes["FirePeriodically:fired"].insert("Subtract");
			downstreamNodes["FirePeriodically:fired"].insert("ConvertIntegertoText");
			downstreamNodes["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			downstreamNodes["DisplayConsoleWindow:typedLine"];
			downstreamNodes["DisplayConsoleWindow:typedWord"];
			downstreamNodes["DisplayConsoleWindow:typedCharacter"];
			QTest::newRow("Feedback loop with a non-trigger cable bypassing it.") << "PassiveBypassFeedbackLoop" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically:fired"].insert("SelectInput");
			downstreamNodes["FirePeriodically:fired"].insert("Subtract");
			downstreamNodes["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically:fired"].insert("Hold2");
			downstreamNodes["FirePeriodically:fired"].insert("Hold3");
			downstreamNodes["FirePeriodically:fired"].insert("MakeList");
			downstreamNodes["FirePeriodically:fired"].insert("AppendTexts");
			downstreamNodes["FirePeriodically:fired"].insert("CountCharacters");
			downstreamNodes["FirePeriodically:fired"].insert("Subtract");
			downstreamNodes["FirePeriodically:fired"].insert("CutText");
			downstreamNodes["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			downstreamNodes["DisplayConsoleWindow:typedLine"];
			downstreamNodes["DisplayConsoleWindow:typedWord"];
			downstreamNodes["DisplayConsoleWindow:typedCharacter"];
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract1");
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract2");
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract3");
			downstreamNodes["FirePeriodically1:fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract1");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract2");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract3");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("ShareValue");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldValue");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("FireOnStart");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldList");
			downstreamNodes[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("AddToList");
			downstreamNodes["FireOnStart:started"];
			QTest::newRow("Leaf nodes gather at published output node.") << "PublishedOutputGather" << downstreamNodes;
		}
	}
	void testNodesDownstream()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedDownstreamNodes);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);

		vector<string> triggerTitles;
		for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = generator->graph->nodeForTrigger.begin(); i != generator->graph->nodeForTrigger.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			VuoCompilerNode *triggerNode = i->second;
			string triggerTitle = triggerNode->getBase()->getTitle() + ":" + trigger->getBase()->getClass()->getName();
			triggerTitles.push_back(triggerTitle);
			QVERIFY2(expectedDownstreamNodes.find(triggerTitle) != expectedDownstreamNodes.end(), triggerTitle.c_str());
			vector<VuoCompilerNode *> actualDownstreamNodes = generator->graph->getNodesDownstream(trigger);
			vector<string> actualDownstreamNodeTitles;

			for (vector<VuoCompilerNode *>::iterator j = actualDownstreamNodes.begin(); j != actualDownstreamNodes.end(); ++j)
			{
				string nodeTitle = (*j)->getBase()->getTitle();
				actualDownstreamNodeTitles.push_back(nodeTitle);
				QVERIFY2(expectedDownstreamNodes[triggerTitle].find(nodeTitle) != expectedDownstreamNodes[triggerTitle].end(),
						 (triggerTitle + " " + nodeTitle).c_str());
			}
			QVERIFY2(actualDownstreamNodes.size() == expectedDownstreamNodes[triggerTitle].size(),
					 (triggerTitle + " " + VuoStringUtilities::join(actualDownstreamNodeTitles, ',')).c_str());
		}
		QVERIFY2(generator->graph->nodeForTrigger.size() == expectedDownstreamNodes.size(),
				 (VuoStringUtilities::join(triggerTitles, ',')).c_str());

		delete parser;
		delete generator;
	}

	void testOrderedNodes_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< set<string> >("omittedNodes");
		QTest::addColumn< set<string> >("extraNodes");

		set<string> noNodes;

		{
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort" << noNodes << noNodes;
		}
		{
			QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather" << noNodes << noNodes;
		}
		{
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop" << noNodes << noNodes;
		}
		{
			QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count" << noNodes << noNodes;
		}
		{
			QTest::newRow("0 triggers.") << "Add" << noNodes << noNodes;
		}
		{
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop" << noNodes << noNodes;
		}
		{
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop" << noNodes << noNodes;
		}
		{
			QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop" << noNodes << noNodes;
		}
		{
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode" << noNodes << noNodes;
		}
		{
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText" << noNodes << noNodes;
		}
		{
			set<string> extraNodes;
			extraNodes.insert(VuoNodeClass::publishedInputNodeIdentifier);
			extraNodes.insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published" << noNodes << extraNodes;
		}
	}
	void testOrderedNodes()
	{
		QFETCH(QString, compositionName);
		QFETCH(set<string>, omittedNodes);
		QFETCH(set<string>, extraNodes);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);
		vector<VuoCompilerNode *> orderedNodes = generator->orderedNodes;

		set<VuoNode *> expectedNodes = composition.getBase()->getNodes();

		// Check that each element of expectedNodes is also in orderedNodes.
		vector<VuoCompilerNode *> orderedNodesRemaining = orderedNodes;
		for (set<VuoNode *>::iterator i = expectedNodes.begin(); i != expectedNodes.end(); ++i)
		{
			VuoCompilerNode *expectedNode = (*i)->getCompiler();
			string expectedNodeTitle = (*i)->getTitle();
			bool shouldBeInOrderedNodes = (omittedNodes.find(expectedNodeTitle) == omittedNodes.end());
			bool isInOrderedNodes = find(orderedNodes.begin(), orderedNodes.end(), expectedNode) != orderedNodes.end();
			QVERIFY2(shouldBeInOrderedNodes == isInOrderedNodes, expectedNodeTitle.c_str());

			if (isInOrderedNodes)
			{
				vector<VuoCompilerNode *>::iterator orderedNodeIter = find(orderedNodesRemaining.begin(), orderedNodesRemaining.end(), expectedNode);
				orderedNodesRemaining.erase(orderedNodeIter);
			}
		}

		// Check that each element of extraNodes is also in orderedNodes.
		for (set<string>::iterator i = extraNodes.begin(); i != extraNodes.end(); ++i)
		{
			string extraNodeTitle = *i;

			bool found = false;
			vector<VuoCompilerNode *> orderedNodesRemainingCopy = orderedNodesRemaining;
			for (vector<VuoCompilerNode *>::iterator j = orderedNodesRemainingCopy.begin(); j != orderedNodesRemainingCopy.end(); ++j)
			{
				if ((*j)->getBase()->getTitle() == extraNodeTitle)
				{
					found = true;
					orderedNodesRemaining.erase( find(orderedNodesRemaining.begin(), orderedNodesRemaining.end(), *j) );
					break;
				}
			}

			QVERIFY2(found, extraNodeTitle.c_str());
		}

		// Check that there are no other extra elements in orderedNodes.
		for (vector<VuoCompilerNode *>::iterator i = orderedNodesRemaining.begin(); i != orderedNodesRemaining.end(); ++i)
		{
			VuoCompilerNode *remainingNode = *i;
			QFAIL(remainingNode->getBase()->getTitle().c_str());
		}

		delete parser;
		delete generator;
	}

	void testChains_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< nodesMap >("expectedChains");

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			QTest::newRow("Linear chain of nodes.") << "Recur_Count" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count3");
			chains["FirePeriodically2:fired"].insert("Count2 Count3");
			QTest::newRow("2 triggers with partially overlapping paths.") << "2Recur_2Count_Count" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count2");
			chains["FirePeriodically2:fired"].insert("Count1 Count2");
			QTest::newRow("2 triggers with completely overlapping paths.") << "2Recur_Count_Count" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count2");
			chains["FirePeriodically1:fired"].insert("Count3 Count4");
			QTest::newRow("Trigger that immediately scatters.") << "Recur_2Count_Count" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			chains["FirePeriodically1:fired"].insert("Count2");
			chains["FirePeriodically1:fired"].insert("Count3");
			QTest::newRow("Trigger that pushes a node that scatters.") << "Recur_Count_2Count" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			chains["FirePeriodically1:fired"].insert("Count2");
			QTest::newRow("Trigger with a gather.") << "Recur_Count_Count_gather" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("FirePeriodically1");
			QTest::newRow("Trigger with a self-loop.") << "Recur_loop" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 FirePeriodically1");
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1 MakeList1 Add1");
			chains["FirePeriodically1:fired"].insert("ConvertIntegertoText1 DisplayConsoleWindow1");
			chains["FirePeriodically1:fired"].insert("Hold1");
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["TriggerWithOutput1:trigger"].insert("TriggerWithOutput1 Count1");
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1");
			chains["FirePeriodically1:fired"].insert("Subtract1");
			chains["FirePeriodically1:fired"].insert("Subtract2");
			chains["FirePeriodically1:fired"].insert("ConvertIntegertoText1 DisplayConsoleWindow1");
			chains["FirePeriodically1:fired"].insert("Hold1");
			QTest::newRow("2 feedback loops sharing the same leaf.") << "TwoLoops" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1");
			chains["FirePeriodically1:fired"].insert("Subtract1");
			chains["FirePeriodically1:fired"].insert("Hold2");
			chains["FirePeriodically1:fired"].insert("Subtract2");
			chains["FirePeriodically1:fired"].insert("ConvertIntegertoText1 DisplayConsoleWindow1");
			chains["FirePeriodically1:fired"].insert("Hold1");
			chains["FirePeriodically1:fired"].insert("Hold2");
			QTest::newRow("2 feedback loops, one nested inside the other.") << "NestedLoops" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("ConvertIntegertoText");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("SelectInput");
			chains["FirePeriodically:fired"].insert("ConvertIntegertoText");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("SelectInput");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold2");
			chains["FirePeriodically:fired"].insert("Hold3");
			chains["FirePeriodically:fired"].insert("MakeList AppendTexts");
			chains["FirePeriodically:fired"].insert("CountCharacters Subtract");
			chains["FirePeriodically:fired"].insert("CutText");
			chains["FirePeriodically:fired"].insert("Hold3");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Subtract1");
			chains["FirePeriodically1:fired"].insert("Subtract2");
			chains["FirePeriodically1:fired"].insert("Subtract3");
			chains["FirePeriodically1:fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract1");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract2");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("Subtract3");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published" << chains;
		}

		{
			map<string, set<string> > chains;
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("ShareValue");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldValue");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("FireOnStart");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldList");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("AddToList");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldList");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Leaf nodes gather at published output node.") << "PublishedOutputGather" << chains;
		}
	}
	void testChains()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedChains);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);
		map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = generator->graph->nodeForTrigger;

		map<string, set<string> > actualChains;
		map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger = generator->chainsForTrigger;
		for (map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> >::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			VuoCompilerNode *node = nodeForTrigger[trigger];
			string triggerString = node->getBase()->getTitle() + ":" + trigger->getBase()->getClass()->getName();

			for (vector<VuoCompilerChain *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				vector<VuoCompilerNode *> nodes = (*j)->getNodes();

				string chainString;
				for (vector<VuoCompilerNode *>::iterator k = nodes.begin(); k != nodes.end(); ++k)
					chainString += (*k)->getBase()->getTitle() + (k+1 == nodes.end() ? "" : " ");

				actualChains[triggerString].insert(chainString);
			}
		}

		// Check that each element of expectedChains is also in actualChains.
		for (map<string, set<string> >::iterator i = expectedChains.begin(); i != expectedChains.end(); ++i)
		{
			string triggerString = i->first;
			QVERIFY2(actualChains.find(triggerString) != actualChains.end(), triggerString.c_str());
			for (set<string>::iterator j = expectedChains[triggerString].begin(); j != expectedChains[triggerString].end(); ++j)
			{
				string expectedChainString = *j;
				set<string>::iterator actualChainIter = actualChains[triggerString].find(expectedChainString);
				QVERIFY2(actualChainIter != actualChains[triggerString].end(), (triggerString + " " + expectedChainString).c_str());
				actualChains[triggerString].erase(actualChainIter);
			}
		}

		// Check that there are no extra elements in actualChains.
		for (map<string, set<string> >::iterator i = actualChains.begin(); i != actualChains.end(); ++i)
		{
			string triggerString = i->first;
			for (set<string>::iterator j = actualChains[triggerString].begin(); j != actualChains[triggerString].end(); ++j)
			{
				string actualChainString = *j;
				QFAIL((triggerString + " " + actualChainString).c_str());
			}
		}

		delete parser;
		delete generator;
	}

	void testWorkerThreadsNeeded_data()
	{
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<QString>("triggerNode");
		QTest::addColumn<QString>("triggerPort");
		QTest::addColumn<int>("expectedMinThreadsNeeded");
		QTest::addColumn<int>("expectedMaxThreadsNeeded");

		QTest::newRow("Trigger with no downstream nodes") << "TriggerBypassFeedbackLoop" << "DisplayConsoleWindow" << "typedLine" << 1 << 1;
		QTest::newRow("Single chain") << "2Recur_Count_Count" << "FirePeriodically1" << "fired" << 1 << 1;
		QTest::newRow("Bypass") << "Recur_Count_Count_gather_Count" << "FirePeriodically1" << "fired" << 1 << 1;
		QTest::newRow("Scatter") << "Recur_2Count_Count" << "FirePeriodically1" << "fired" << 1 << 2;
		QTest::newRow("Scatter, gather") << "TriggerScatterGather" << "FirePeriodically1" << "fired" << 1 << 2;
		QTest::newRow("Scatter, scatter all branches") << "TriggerScatterScatter" << "FirePeriodically1" << "fired" << 1 << 5;
		QTest::newRow("Scatter, scatter one branch") << "TriggerScatterScatterPartial" << "FirePeriodically1" << "fired" << 1 << 3;
		QTest::newRow("Scatter, gather, scatter") << "TriggerScatterGatherScatter" << "FirePeriodically1" << "fired" << 1 << 6;  // ideal max = 3
		QTest::newRow("Small feedback loop") << "PassiveBypassFeedbackLoop" << "FirePeriodically" << "fired" << 1 << 3;
		QTest::newRow("Large feedback loop") << "StoreRecentText" << "FirePeriodically" << "fired" << 1 << 2;
	}
	void testWorkerThreadsNeeded()
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, triggerNode);
		QFETCH(QString, triggerPort);
		QFETCH(int, expectedMinThreadsNeeded);
		QFETCH(int, expectedMaxThreadsNeeded);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		VuoCompilerTriggerPort *trigger = NULL;
		set<VuoNode *> nodes = composition.getBase()->getNodes();
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoNode *node = *i;
			if (node->getTitle() == triggerNode.toStdString())
			{
				VuoPort *port = node->getOutputPortWithName(triggerPort.toStdString());
				trigger = dynamic_cast<VuoCompilerTriggerPort *>( port->getCompiler() );
			}
		}
		QVERIFY(trigger);

		int actualMinThreadsNeeded, actualMaxThreadsNeeded;
		graph.getWorkerThreadsNeeded(trigger, actualMinThreadsNeeded, actualMaxThreadsNeeded);

		QCOMPARE(actualMinThreadsNeeded, expectedMinThreadsNeeded);
		QCOMPARE(actualMaxThreadsNeeded, expectedMaxThreadsNeeded);
	}

	void testNodesToWaitOn_data()
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< nodesMap >("expectedNodesForTriggerOrNode");

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("Count1");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("Count2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired Count1"].insert("Count3");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired Count2"].insert("Count3");
			QTest::newRow("Triggers with partially overlapping paths") << "2Recur_2Count_Count" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage"].insert("CaptureImageOfScreen");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage ShareValue2"].insert("BlendImages");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage BlendImages"].insert("MakeImageLayer");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeImageLayer"].insert("MakeList1");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeImageLayer"].insert("MakeList2");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeList1"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeList2"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage RenderLayersToImage"].insert("HoldValue");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("HoldValue");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("BlendImages");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("MakeImageLayer");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("MakeList1");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("MakeList2");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame HoldValue"].insert("BlendImages");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame BlendImages"].insert("MakeImageLayer");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame MakeImageLayer"].insert("MakeList1");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame MakeImageLayer"].insert("MakeList2");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame MakeList1"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame MakeList2"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:requestedFrame RenderLayersToImage"].insert("HoldValue");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:showedWindow"].insert("RenderLayersToWindow");
			// expectedNodesForTriggerOrNode["RenderLayersToWindow:renderedLayers"].insert("RenderLayersToWindow");
			QTest::newRow("Triggers entering a feedback loop at different points") << "TriggersEnteringFeedback" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("ShareValue4");
			QTest::newRow("Trigger with immediate scatter, other trigger overlapping a branch") << "TriggerScatterOverlap" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue2"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue4"].insert("ShareValue5");
			QTest::newRow("Trigger with immediate scatter and downstream gather") << "TriggerScatterGather" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue2"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue4"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired ShareValue4"].insert("ShareValue5");
			QTest::newRow("Trigger with immediate scatter and downstream gather, other trigger overlapping a branch") << "TriggerScatterGatherOverlap" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue2"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue4"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired ShareValue4"].insert("ShareValue5");
			QTest::newRow("Trigger with downstream scatter and gather, other trigger overlapping a branch") << "TriggerDownstreamScatterGatherOverlap" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue6");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue2"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue4"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue5"].insert("ShareValue6");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("ShareValue6");
			QTest::newRow("Trigger with immediate scatter and downstream gather, other trigger overlapping after the gather") << "TriggerScatterGatherLaterOverlap" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("FirePeriodically1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue1");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue3");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue5");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue6");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue7");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue8");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired"].insert("ShareValue9");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue1"].insert("ShareValue2");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue2"].insert("ShareValue7");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue3"].insert("ShareValue4");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue4"].insert("ShareValue7");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue5"].insert("ShareValue6");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue6"].insert("ShareValue9");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue7"].insert("ShareValue8");
			expectedNodesForTriggerOrNode["FirePeriodically1:fired ShareValue8"].insert("ShareValue9");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("FirePeriodically2");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired"].insert("ShareValue8");
			expectedNodesForTriggerOrNode["FirePeriodically2:fired ShareValue8"].insert("ShareValue9");
			QTest::newRow("Trigger with immediate scatter and downstream gathers, other trigger overlapping after a partial gather") << "TriggerScatterPartialGatherOverlap" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert(VuoNodeClass::publishedInputNodeIdentifier);
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("ShareValue");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired ShareValue"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldValue");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired HoldValue"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("FireOnStart");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired FireOnStart"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired"].insert("HoldList");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired HoldList"].insert("AddToList");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired AddToList"].insert("HoldList");
			expectedNodesForTriggerOrNode[VuoNodeClass::publishedInputNodeIdentifier + ":fired HoldList"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("FireOnStart");
			QTest::newRow("Leaf nodes gather at published output node") << "PublishedOutputGather" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Fire on Start");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Spin Off Event");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Make RGB Color");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Fire on Start:started Make RGB Color"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Fire on Start:started Make Color Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Fire on Start:started Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff"].insert("Spin Off Event");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff"].insert("Fetch Image");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff Fetch Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:showedWindow"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Render Image to Window");
			QTest::newRow("Trigger overlaps with Spin Off node") << "SpinOffEventFetchImage" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Allow First Event");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Spin Off Value");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Allow First Event"].insert("Spin Off Value");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Allow First Event"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Allow First Event"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Allow First Event"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Make Color Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Spin Off Value");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Fetch Image");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff Fetch Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:showedWindow"].insert("Render Image to Window");
			QTest::newRow("Trigger overlaps with Spin Off node whose downstream nodes are out of order") << "SpinOffValueFetchImage" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Fire on Start");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Spin Off Event");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Make RGB Color");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Fire on Start:started"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Fire on Start:started Make RGB Color"].insert("Make Color Image");
			expectedNodesForTriggerOrNode["Fire on Start:started Make Color Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Fire on Start:started Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff"].insert("Spin Off Event");
			expectedNodesForTriggerOrNode["Spin Off Event:spunOff"].insert("Spin Off Value");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Spin Off Value");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff"].insert("Fetch Image");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff Fetch Image"].insert("Select Latest");
			expectedNodesForTriggerOrNode["Spin Off Value:spunOff Select Latest"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:showedWindow"].insert("Render Image to Window");
			expectedNodesForTriggerOrNode["Render Image to Window:requestedFrame"].insert("Render Image to Window");
			QTest::newRow("Trigger overlaps indirectly with Spin Off node") << "SpinOffEventValueFetchImage" << expectedNodesForTriggerOrNode;
		}
	}
	void testNodesToWaitOn()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedNodesForTriggerOrNode);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);

		map<string, vector<string> > actualNodesForTriggerOrNode;
		map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodesForTriggers = generator->graph->getNodesForTriggerPorts();
		for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodesForTriggers.begin(); i != nodesForTriggers.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			VuoCompilerNode *triggerNode = i->second;
			string triggerId = triggerNode->getBase()->getTitle() + ":" + trigger->getBase()->getClass()->getName();

			vector<VuoCompilerNode *> actualNodesForTrigger = generator->getNodesToWaitOnBeforeTransmission(trigger);
			for (vector<VuoCompilerNode *>::iterator j = actualNodesForTrigger.begin(); j != actualNodesForTrigger.end(); ++j)
				actualNodesForTriggerOrNode[triggerId].push_back( (*j)->getBase()->getTitle() );

			vector<VuoCompilerNode *> downstreamNodes = generator->graph->getNodesDownstream(trigger);
			for (vector<VuoCompilerNode *>::iterator j = downstreamNodes.begin(); j != downstreamNodes.end(); ++j)
			{
				VuoCompilerNode *node = *j;
				string nodeId = triggerId + " " + node->getBase()->getTitle();

				vector<VuoCompilerNode *> actualNodesForNode = generator->getNodesToWaitOnBeforeTransmission(trigger, node);
				for (vector<VuoCompilerNode *>::iterator k = actualNodesForNode.begin(); k != actualNodesForNode.end(); ++k)
					actualNodesForTriggerOrNode[nodeId].push_back( (*k)->getBase()->getTitle() );
			}
		}

		vector<string> actualKeys;
		for (map<string, vector<string> >::iterator i = actualNodesForTriggerOrNode.begin(); i != actualNodesForTriggerOrNode.end(); ++i)
			actualKeys.push_back( i->first );
		vector<string> expectedKeys;
		for (map<string, set<string> >::iterator i = expectedNodesForTriggerOrNode.begin(); i != expectedNodesForTriggerOrNode.end(); ++i)
			expectedKeys.push_back( i->first );
		QVERIFY2(expectedKeys == actualKeys, VuoStringUtilities::join(actualKeys, ',').c_str());

		for (map<string, vector<string> >::iterator i = actualNodesForTriggerOrNode.begin(); i != actualNodesForTriggerOrNode.end(); ++i)
		{
			string triggerOrNode = i->first;
			vector<string> actualNodesVector = i->second;
			set<string> actualNodes(actualNodesVector.begin(), actualNodesVector.end());
			set<string> expectedNodes = expectedNodesForTriggerOrNode[triggerOrNode];

			QVERIFY2(expectedNodes == actualNodes, (triggerOrNode + " : " + VuoStringUtilities::join(actualNodesVector, ',')).c_str());
		}

		delete parser;
		delete generator;
	}

	void testInfiniteFeedbackLoops_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< bool >("hasInfiniteFeedbackLoop");

		QTest::newRow("Infinite feedback loop containing 2 nodes.") << "TriggerPortWithOutputPort_Count_infiniteLoop.vuo" << true;
		QTest::newRow("Infinite feedback loop containing 1 node.") << "Recur_Count_infiniteLoop.vuo" << true;
		QTest::newRow("No feedback loops.") << "Recur_Count_Count_gather_Count.vuo" << false;
	}
	void testInfiniteFeedbackLoops()
	{
		QFETCH(QString, compositionFile);
		QFETCH(bool, hasInfiniteFeedbackLoop);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		bool gotException = false;
		try {
			graph.checkForInfiniteFeedback();
		} catch (const VuoCompilerException &e) {
			gotException = true;
		}
		QCOMPARE(gotException, hasInfiniteFeedbackLoop);

		delete parser;
	}

	void testDeadlockedFeedbackLoops_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< bool >("hasDeadlockedFeedbackLoop");

		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 2-node feedback loop.") << "DeadlockedFeedbackLoop2Nodes.vuo" << true;
		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 3-node feedback loop.") << "DeadlockedFeedbackLoop3Nodes.vuo" << true;
		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 4-node feedback loop.") << "DeadlockedFeedbackLoop4Nodes.vuo" << true;
		QTest::newRow("Trigger unambiguously pushing 2 nodes in a 2-node feedback loop.") << "AsymmetricFeedbackLoop.vuo" << false;
		QTest::newRow("Trigger unambiguously pushing 2 nested feedback loops.") << "NestedLoopsBothTriggered.vuo" << false;
		QTest::newRow("Trigger unambiguously pushing nodes not in a feedback loop.") << "NonDeadlockedWithSemiconductor.vuo" << false;
		QTest::newRow("2 triggers that start at different points in a feedback loop.") << "NonDeadlocked2Triggers.vuo" << false;
	}
	void testDeadlockedFeedbackLoops()
	{
		QFETCH(QString, compositionFile);
		QFETCH(bool, hasDeadlockedFeedbackLoop);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		bool gotException = false;
		try {
			graph.checkForDeadlockedFeedback();
		} catch (const VuoCompilerException &e) {
			gotException = true;
		}
		QCOMPARE(gotException, hasDeadlockedFeedbackLoop);

		delete parser;
	}

	void testStatefulAndStatelessFunctions_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< bool >("isStateful");

		QTest::newRow("Empty composition") << "Empty.vuo" << false;
		QTest::newRow("All stateless nodes") << "Add.vuo" << false;
		QTest::newRow("Some stateless and some stateful nodes") << "StoreRecentText.vuo" << true;
	}
	void testStatefulAndStatelessFunctions()
	{
		QFETCH(QString, compositionFile);
		QFETCH(bool, isStateful);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string bcFile;
		VuoFileUtilities::splitPath(bcPath, dir, bcFile, extension);
		string prefix = VuoStringUtilities::transcodeToIdentifier(bcFile) + "__";

		for (int i = 0; i < 2; ++i)
		{
			bool isTopLevelComposition = (i == 0);
			compiler->compileComposition(compositionPath, bcPath, isTopLevelComposition);

			Module *module = VuoCompiler::readModuleFromBitcode(bcPath);
			remove(bcPath.c_str());

			set<string> functionNames;
			Module::FunctionListType& functions = module->getFunctionList();
			for (Module::FunctionListType::iterator i = functions.begin(); i != functions.end(); ++i)
				functionNames.insert( i->getName() );

			QVERIFY((functionNames.find(prefix + "nodeEvent") != functionNames.end()) == (! isTopLevelComposition && ! isStateful));
			QVERIFY((functionNames.find(prefix + "nodeInstanceEvent") != functionNames.end()) == (! isTopLevelComposition && isStateful));
			QVERIFY((functionNames.find(prefix + "nodeInstanceInit") != functionNames.end()) == isStateful);
			QVERIFY((functionNames.find(prefix + "nodeInstanceFini") != functionNames.end()) == isStateful);
			QVERIFY((functionNames.find(prefix + "nodeInstanceTriggerStart") != functionNames.end()) == isStateful);
			QVERIFY((functionNames.find(prefix + "nodeInstanceTriggerStop") != functionNames.end()));
			QVERIFY((functionNames.find(prefix + "nodeInstanceTriggerUpdate") != functionNames.end()) == (! isTopLevelComposition && isStateful));
		}
	}

	void testPublishedPortGetters()
	{
		string compositionPath = getCompositionPath("Recur_Subtract_published.vuo");
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		compiler->compileComposition(compositionPath, bcPath);

		Module *module = VuoCompiler::readModuleFromBitcode(bcPath);
		remove(bcPath.c_str());
		vector<GenericValue> args;

		{
			GenericValue ret;
			bool success = executeFunction(module, "getPublishedInputPortCount", args, ret);
			QVERIFY(success);
			QCOMPARE(ret.IntVal.getZExtValue(), (uint64_t)2);
		}

		{
			GenericValue ret;
			bool success = executeFunction(module, "getPublishedInputPortNames", args, ret);
			QVERIFY(success);
			char **names = (char **)ret.PointerVal;
			for (int i = 0; i < 2; ++i)
				QVERIFY2(! strcmp(names[i], "publishedIn0") || ! strcmp(names[i], "publishedIn1"), names[i]);  // std::string::operator== doesn't work here
		}
	}

};

QTEST_APPLESS_MAIN(TestVuoCompilerBitcodeGenerator)
#include "TestVuoCompilerBitcodeGenerator.moc"
