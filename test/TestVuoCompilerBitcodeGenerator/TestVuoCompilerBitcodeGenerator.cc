/**
 * @file
 * TestVuoCompilerBitcodeGenerator interface and implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fstream>
#include "TestVuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGraph.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"
#include "VuoPort.hh"


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
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< nodesMap >("expectedDownstreamNodes");

		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["TriggerWithOutput1:trigger"].insert("TriggerWithOutput1");
			downstreamNodes["TriggerWithOutput1:trigger"].insert("Count1");
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("Count2");
			QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("FirePeriodically1");
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Count1");
			downstreamNodes["FirePeriodically1:fired"].insert("Count3");
			downstreamNodes["FirePeriodically2:fired"].insert("Count2");
			downstreamNodes["FirePeriodically2:fired"].insert("Count3");
			QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			QTest::newRow("0 triggers.") << "Add.vuo" << downstreamNodes;
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
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop.vuo" << downstreamNodes;
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
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop.vuo" << downstreamNodes;
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
			QTest::newRow("Feedback loop with a non-trigger cable bypassing it.") << "PassiveBypassFeedbackLoop.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically:fired"].insert("SelectInput");
			downstreamNodes["FirePeriodically:fired"].insert("Subtract");
			downstreamNodes["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode.vuo" << downstreamNodes;
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
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText.vuo" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract1");
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract2");
			downstreamNodes["FirePeriodically1:fired"].insert("Subtract3");
			downstreamNodes["PublishedInputs:publishedIn0"].insert("Subtract1");
			downstreamNodes["PublishedInputs:publishedIn0"].insert("Subtract2");
			downstreamNodes["PublishedInputs:publishedIn0"].insert("Subtract3");
			downstreamNodes["PublishedInputs:publishedIn1"].insert("Subtract1");
			downstreamNodes["PublishedInputs:publishedIn1"].insert("Subtract3");
			downstreamNodes["PublishedInputs:vuoSimultaneous"].insert("Subtract1");
			downstreamNodes["PublishedInputs:vuoSimultaneous"].insert("Subtract2");
			downstreamNodes["PublishedInputs:vuoSimultaneous"].insert("Subtract3");
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published.vuo" << downstreamNodes;
		}
	}
	void testNodesDownstream()
	{
		QFETCH(QString, compositionFile);
		QFETCH(nodesMap, expectedDownstreamNodes);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, compiler);

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
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< set<string> >("omittedNodes");

		set<string> noOmittedNodes;

		{
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count.vuo" << noOmittedNodes;
		}
		{
			set<string> omittedNodes;
			omittedNodes.insert("Add");
			QTest::newRow("0 triggers.") << "Add.vuo" << omittedNodes;
		}
		{
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText.vuo" << noOmittedNodes;
		}
		{
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published.vuo" << noOmittedNodes;
		}
	}
	void testOrderedNodes()
	{
		QFETCH(QString, compositionFile);
		QFETCH(set<string>, omittedNodes);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, compiler);
		vector<VuoCompilerNode *> orderedNodes = generator->orderedNodes;

		set<VuoNode *> expectedNodes = composition.getBase()->getNodes();
		VuoNode *publishedInputNode = composition.getPublishedInputNode();
		if (publishedInputNode)
			expectedNodes.insert(publishedInputNode);

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

		// Check that there are no extra elements in orderedNodes.
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
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< nodesMap >("expectedChains");

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			QTest::newRow("Linear chain of nodes.") << "Recur_Count.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count3");
			chains["FirePeriodically2:fired"].insert("Count2 Count3");
			QTest::newRow("2 triggers with partially overlapping paths.") << "2Recur_2Count_Count.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count2");
			chains["FirePeriodically2:fired"].insert("Count1 Count2");
			QTest::newRow("2 triggers with completely overlapping paths.") << "2Recur_Count_Count.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 Count2");
			chains["FirePeriodically1:fired"].insert("Count3 Count4");
			QTest::newRow("Trigger that immediately scatters.") << "Recur_2Count_Count.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			chains["FirePeriodically1:fired"].insert("Count2");
			chains["FirePeriodically1:fired"].insert("Count3");
			QTest::newRow("Trigger that pushes a node that scatters.") << "Recur_Count_2Count.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1");
			chains["FirePeriodically1:fired"].insert("Count2");
			QTest::newRow("Trigger with a gather.") << "Recur_Count_Count_gather.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("FirePeriodically1");
			QTest::newRow("Trigger with a self-loop.") << "Recur_loop.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Count1 FirePeriodically1");
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1 MakeList1 Add1");
			chains["FirePeriodically1:fired"].insert("ConvertIntegertoText1 DisplayConsoleWindow1");
			chains["FirePeriodically1:fired"].insert("Hold1");
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["TriggerWithOutput1:trigger"].insert("TriggerWithOutput1 Count1");
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1");
			chains["FirePeriodically1:fired"].insert("Subtract1");
			chains["FirePeriodically1:fired"].insert("Subtract2");
			chains["FirePeriodically1:fired"].insert("ConvertIntegertoText1 DisplayConsoleWindow1");
			chains["FirePeriodically1:fired"].insert("Hold1");
			QTest::newRow("2 feedback loops sharing the same leaf.") << "TwoLoops.vuo" << chains;
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
			QTest::newRow("2 feedback loops, one nested inside the other.") << "NestedLoops.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("ConvertIntegertoText");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("SelectInput");
			chains["FirePeriodically:fired"].insert("ConvertIntegertoText");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically:fired"].insert("Hold");
			chains["FirePeriodically:fired"].insert("Subtract");
			chains["FirePeriodically:fired"].insert("SelectInput");
			chains["FirePeriodically:fired"].insert("Hold");
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode.vuo" << chains;
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
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Subtract1");
			chains["FirePeriodically1:fired"].insert("Subtract2");
			chains["FirePeriodically1:fired"].insert("Subtract3");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":publishedIn0"].insert("Subtract1");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":publishedIn0"].insert("Subtract2");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":publishedIn0"].insert("Subtract3");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":publishedIn1"].insert("Subtract1 Subtract3");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":vuoSimultaneous"].insert("Subtract1");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":vuoSimultaneous"].insert("Subtract2");
			chains[VuoNodeClass::publishedInputNodeIdentifier + ":vuoSimultaneous"].insert("Subtract3");
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published.vuo" << chains;
		}
	}
	void testChains()
	{
		QFETCH(QString, compositionFile);
		QFETCH(nodesMap, expectedChains);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, compiler);
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
