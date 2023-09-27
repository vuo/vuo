/**
 * @file
 * TestVuoCompilerBitcodeGenerator interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <fstream>
#include "TestVuoCompiler.hh"

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

	string getPublishedInputTriggerIdentifier()
	{
		return VuoCompilerGraph::getPublishedInputTriggerNodeIdentifier() + ":spunOff";;
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
		compiler->setCompositionPath(compositionPath);
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
			string publishedInputTriggerIdentifier = getPublishedInputTriggerIdentifier();
			downstreamNodes[publishedInputTriggerIdentifier].insert("Subtract1");
			downstreamNodes[publishedInputTriggerIdentifier].insert("Subtract2");
			downstreamNodes[publishedInputTriggerIdentifier].insert("Subtract3");
			downstreamNodes[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedInputNodeIdentifier);
			downstreamNodes[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Published input and output ports.") << "Subtract_published" << downstreamNodes;
		}
		{
			map<string, set<string> > downstreamNodes;
			string publishedInputTriggerIdentifier = getPublishedInputTriggerIdentifier();
			downstreamNodes[publishedInputTriggerIdentifier].insert("ShareValue");
			downstreamNodes[publishedInputTriggerIdentifier].insert("HoldValue");
			downstreamNodes[publishedInputTriggerIdentifier].insert("FireOnStart");
			downstreamNodes[publishedInputTriggerIdentifier].insert("HoldList");
			downstreamNodes[publishedInputTriggerIdentifier].insert("AddToList");
			downstreamNodes[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedInputNodeIdentifier);
			downstreamNodes[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			downstreamNodes["FireOnStart:started"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Leaf nodes gather at published output node.") << "PublishedOutputGather" << downstreamNodes;
		}
	}
	void testNodesDownstream()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedDownstreamNodes);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);

		vector<string> autoAddedTriggers(1, VuoCompilerGraph::getManuallyFirableTriggerNodeIdentifier());

		vector<string> triggerTitles;
		for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = generator->graph->nodeForTrigger.begin(); i != generator->graph->nodeForTrigger.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			VuoCompilerNode *triggerNode = i->second;

			if (find(autoAddedTriggers.begin(), autoAddedTriggers.end(), triggerNode->getIdentifier()) != autoAddedTriggers.end())
				continue;

			string triggerIdentifier = triggerNode->getIdentifier() + ":" + trigger->getBase()->getClass()->getName();
			triggerTitles.push_back(triggerIdentifier);
			QVERIFY2(expectedDownstreamNodes.find(triggerIdentifier) != expectedDownstreamNodes.end(), triggerIdentifier.c_str());
			vector<VuoCompilerNode *> actualDownstreamNodes = generator->graph->getNodesDownstream(trigger);
			vector<string> actualDownstreamNodeTitles;

			for (VuoCompilerNode *downstreamNode : actualDownstreamNodes)
			{
				string nodeIdentifier = downstreamNode->getIdentifier();
				actualDownstreamNodeTitles.push_back(nodeIdentifier);
				QVERIFY2(expectedDownstreamNodes[triggerIdentifier].find(nodeIdentifier) != expectedDownstreamNodes[triggerIdentifier].end(),
						 (triggerIdentifier + " " + nodeIdentifier).c_str());
			}
			QVERIFY2(actualDownstreamNodes.size() == expectedDownstreamNodes[triggerIdentifier].size(),
					 (triggerIdentifier + " " + VuoStringUtilities::join(actualDownstreamNodeTitles, ',')).c_str());
		}

		QVERIFY2(generator->graph->nodeForTrigger.size() - autoAddedTriggers.size() == expectedDownstreamNodes.size(),
				 (VuoStringUtilities::join(triggerTitles, ',')).c_str());

		delete parser;
		delete generator;
	}

	void testOrderedNodes_data()
	{
		QTest::addColumn< QString >("compositionName");

		{
			QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort";
		}
		{
			QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather";
		}
		{
			QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop";
		}
		{
			QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count";
		}
		{
			QTest::newRow("0 triggers.") << "Add";
		}
		{
			QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop";
		}
		{
			QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop";
		}
		{
			QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop";
		}
		{
			QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode";
		}
		{
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText";
		}
		{
			QTest::newRow("Published input and output ports.") << "Subtract_published";
		}
	}
	void testOrderedNodes()
	{
		QFETCH(QString, compositionName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);
		vector<VuoCompilerNode *> orderedNodes = generator->orderedNodes;

		set<VuoCompilerNode *> expectedNodes = composition.getCachedGraph(compiler)->getNodes();

		vector<string> orderedNodeIdentifiers;
		for (VuoCompilerNode *node : orderedNodes)
			orderedNodeIdentifiers.push_back(node->getIdentifier());

		vector<string> expectedNodeIdentifiers;
		for (VuoCompilerNode *node : expectedNodes)
			expectedNodeIdentifiers.push_back(node->getIdentifier());

		std::sort(orderedNodeIdentifiers.begin(), orderedNodeIdentifiers.end());
		std::sort(expectedNodeIdentifiers.begin(), expectedNodeIdentifiers.end());

		QCOMPARE(QString::fromStdString(VuoStringUtilities::join(orderedNodeIdentifiers, " ")),
				 QString::fromStdString(VuoStringUtilities::join(expectedNodeIdentifiers, " ")));

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
			string publishedInputTriggerIdentifier = getPublishedInputTriggerIdentifier();
			chains[publishedInputTriggerIdentifier].insert("Subtract1");
			chains[publishedInputTriggerIdentifier].insert("Subtract2");
			chains[publishedInputTriggerIdentifier].insert("Subtract3");
			chains[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedInputNodeIdentifier);
			chains[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Published input and output ports.") << "Subtract_published" << chains;
		}

		{
			map<string, set<string> > chains;
			string publishedInputTriggerIdentifier = getPublishedInputTriggerIdentifier();
			chains[publishedInputTriggerIdentifier].insert("ShareValue");
			chains[publishedInputTriggerIdentifier].insert("HoldValue");
			chains[publishedInputTriggerIdentifier].insert("FireOnStart");
			chains[publishedInputTriggerIdentifier].insert("HoldList");
			chains[publishedInputTriggerIdentifier].insert("AddToList");
			chains[publishedInputTriggerIdentifier].insert("HoldList");
			chains[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedInputNodeIdentifier);
			chains[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			chains["FireOnStart:started"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Leaf nodes gather at published output node.") << "PublishedOutputGather" << chains;
		}
	}
	void testChains()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedChains);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);
		map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = generator->graph->nodeForTrigger;

		map<string, set<string> > actualChains;
		map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger = generator->chainsForTrigger;
		for (const map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> >::value_type i : chainsForTrigger)
		{
			VuoCompilerTriggerPort *trigger = i.first;
			VuoCompilerNode *triggerNode = nodeForTrigger[trigger];
			string triggerString = triggerNode->getIdentifier() + ":" + trigger->getBase()->getClass()->getName();

			for (VuoCompilerChain *chain : i.second)
			{
				vector<string> chainNodeIdentifiers;
				for (VuoCompilerNode *chainNode : chain->getNodes())
					chainNodeIdentifiers.push_back(chainNode->getIdentifier());

				string chainString = VuoStringUtilities::join(chainNodeIdentifiers, " ");
				actualChains[triggerString].insert(chainString);
			}
		}

		// Check that each element of expectedChains is also in actualChains.
		for (map<string, set<string> >::iterator i = expectedChains.begin(); i != expectedChains.end(); ++i)
		{
			string triggerString = i->first;
			QVERIFY2(actualChains.find(triggerString) != actualChains.end(), triggerString.c_str());
			for (const string &expectedChainString : expectedChains[triggerString])
			{
				set<string>::iterator actualChainIter = actualChains[triggerString].find(expectedChainString);
				QVERIFY2(actualChainIter != actualChains[triggerString].end(), (triggerString + " " + expectedChainString).c_str());
				actualChains[triggerString].erase(actualChainIter);
			}
		}

		// Check that there are no extra elements in actualChains.
		for (map<string, set<string> >::iterator i = actualChains.begin(); i != actualChains.end(); ++i)
		{
			string triggerString = i->first;
			for (const string &actualChainString : actualChains[triggerString])
			{
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
		compiler->setCompositionPath(compositionPath);
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
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeImageLayer"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeImageLayer"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeImageLayer"].insert("HoldValue");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeList1"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage MakeList2"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["CaptureImageOfScreen:capturedImage RenderLayersToImage"].insert("HoldValue");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("FireOnDisplayRefresh");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("HoldValue");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("BlendImages");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("MakeImageLayer");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("MakeList1");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("MakeList2");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame HoldValue"].insert("BlendImages");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame BlendImages"].insert("MakeImageLayer");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeImageLayer"].insert("MakeList1");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeImageLayer"].insert("MakeList2");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeImageLayer"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeImageLayer"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeImageLayer"].insert("HoldValue");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeList1"].insert("RenderLayersToWindow");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame MakeList2"].insert("RenderLayersToImage");
			expectedNodesForTriggerOrNode["FireOnDisplayRefresh:requestedFrame RenderLayersToImage"].insert("HoldValue");
			expectedNodesForTriggerOrNode["RenderLayersToWindow:updatedWindow"].insert("RenderLayersToWindow");
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
			string publishedInputTriggerIdentifier = getPublishedInputTriggerIdentifier();
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier].insert(VuoCompilerGraph::getPublishedInputTriggerNodeIdentifier());
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier].insert(VuoNodeClass::publishedInputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert("ShareValue");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert("HoldValue");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert("FireOnStart");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert("HoldList");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert("AddToList");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " " + VuoNodeClass::publishedInputNodeIdentifier].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " ShareValue"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " HoldValue"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " FireOnStart"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " HoldList"].insert("AddToList");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " HoldList"].insert("HoldList");
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " HoldList"].insert(VuoNodeClass::publishedOutputNodeIdentifier);
			expectedNodesForTriggerOrNode[publishedInputTriggerIdentifier + " AddToList"].insert("HoldList");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("FireOnStart " + VuoNodeClass::publishedOutputNodeIdentifier);
			QTest::newRow("Leaf nodes gather at published output node") << "PublishedOutputGather" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("FireOnStart");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("SpinOffEvent");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("MakeRGBColor");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["FireOnStart:started MakeRGBColor"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["FireOnStart:started MakeColorImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["FireOnStart:started SelectLatest"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff"].insert("SpinOffEvent");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff"].insert("FetchImage");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff FetchImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff SelectLatest"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("RenderImageToWindow");
			QTest::newRow("Trigger overlaps with Spin Off node") << "SpinOffEventFetchImage" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("AllowFirstEvent");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("SpinOffValue");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow AllowFirstEvent"].insert("SpinOffValue");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow AllowFirstEvent"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow AllowFirstEvent"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow AllowFirstEvent"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow MakeColorImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow SelectLatest"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("SpinOffValue");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("FetchImage");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff FetchImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff SelectLatest"].insert("RenderImageToWindow");
			QTest::newRow("Trigger overlaps with Spin Off node whose downstream nodes are out of order") << "SpinOffValueFetchImage" << expectedNodesForTriggerOrNode;
		}

		{
			map<string, set<string> > expectedNodesForTriggerOrNode;
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("FireOnStart");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("SpinOffEvent");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("MakeRGBColor");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["FireOnStart:started"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["FireOnStart:started MakeRGBColor"].insert("MakeColorImage");
			expectedNodesForTriggerOrNode["FireOnStart:started MakeColorImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["FireOnStart:started SelectLatest"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff"].insert("SpinOffEvent");
			expectedNodesForTriggerOrNode["SpinOffEvent:spunOff"].insert("SpinOffValue");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("SpinOffValue");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff"].insert("FetchImage");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff FetchImage"].insert("SelectLatest");
			expectedNodesForTriggerOrNode["SpinOffValue:spunOff SelectLatest"].insert("RenderImageToWindow");
			expectedNodesForTriggerOrNode["RenderImageToWindow:updatedWindow"].insert("RenderImageToWindow");
			QTest::newRow("Trigger overlaps indirectly with Spin Off node") << "SpinOffEventValueFetchImage" << expectedNodesForTriggerOrNode;
		}
	}
	void testNodesToWaitOn()
	{
		QFETCH(QString, compositionName);
		QFETCH(nodesMap, expectedNodesForTriggerOrNode);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, compositionName.toStdString(), compiler);

		map<string, vector<string> > actualNodesForTriggerOrNode;
		for (VuoCompilerTriggerPort *trigger : generator->graph->getTriggerPorts())
		{
			if (trigger == generator->graph->getManuallyFirableTrigger())
				continue;

			VuoCompilerNode *triggerNode = generator->graph->getNodeForTriggerPort(trigger);
			string triggerId = triggerNode->getIdentifier() + ":" + trigger->getBase()->getClass()->getName();

			vector<VuoCompilerNode *> actualNodesForTrigger = generator->getNodesToWaitOnBeforeTransmission(trigger);
			for (VuoCompilerNode *node : actualNodesForTrigger)
				actualNodesForTriggerOrNode[triggerId].push_back( node->getIdentifier() );

			vector<VuoCompilerNode *> downstreamNodes = generator->graph->getNodesDownstream(trigger);
			for (VuoCompilerNode *node : downstreamNodes)
			{
				string nodeId = triggerId + " " + node->getIdentifier();

				vector<VuoCompilerNode *> actualNodesForNode = generator->getNodesToWaitOnBeforeTransmission(trigger, node);
				for (VuoCompilerNode *nodeToWaitOn : actualNodesForNode)
					actualNodesForTriggerOrNode[nodeId].push_back( nodeToWaitOn->getIdentifier() );
			}
		}

		vector<string> actualKeys;
		for (map<string, vector<string> >::iterator i = actualNodesForTriggerOrNode.begin(); i != actualNodesForTriggerOrNode.end(); ++i)
			actualKeys.push_back( i->first );
		vector<string> expectedKeys;
		for (map<string, set<string> >::iterator i = expectedNodesForTriggerOrNode.begin(); i != expectedNodesForTriggerOrNode.end(); ++i)
			expectedKeys.push_back( i->first );
		QCOMPARE(QString::fromStdString(VuoStringUtilities::join(actualKeys, ", ")),
				 QString::fromStdString(VuoStringUtilities::join(expectedKeys, ", ")));

		for (map<string, vector<string> >::iterator i = actualNodesForTriggerOrNode.begin(); i != actualNodesForTriggerOrNode.end(); ++i)
		{
			string triggerOrNode = i->first;
			vector<string> actualNodes = i->second;

			vector<string> expectedNodes;
			for (string nodeId : expectedNodesForTriggerOrNode[triggerOrNode])
				expectedNodes.push_back(nodeId);

			std::sort(actualNodes.begin(), actualNodes.end());
			std::sort(expectedNodes.begin(), expectedNodes.end());

			QCOMPARE(QString::fromStdString(triggerOrNode + " : " + VuoStringUtilities::join(actualNodes, " ")),
					 QString::fromStdString(triggerOrNode + " : " + VuoStringUtilities::join(expectedNodes, " ")));
		}

		delete parser;
		delete generator;
	}

	void testNodesDownstreamViaDataOnlyTransmission_data()
	{
		QTest::addColumn<QString>("compositionFile");
		QTest::addColumn<QString>("selectedNodeIdentifier");
		QTest::addColumn<bool>("isSourceNode");
		QTest::addColumn<QStringList>("expectedDownstreamNodeIdentifiers");

		QString publishedInputNodeIdentifier = QString::fromStdString(VuoNodeClass::publishedInputNodeIdentifier);

		QTest::newRow("Node can't transmit data only") << "Recur_Count_2Count.vuo" << "Count1" << false << QStringList();
		QTest::newRow("Node can transmit data only but no outgoing cables") << "PublishedInputNotConnected.vuo" << publishedInputNodeIdentifier << false << QStringList();
		QTest::newRow("Drawer without incoming cable") << "PublishedOutputGather.vuo" << "MakeList" << true << QStringList("HoldList");
		QTest::newRow("Drawer with incoming cable") << "Recur_Hold_Add_Write_loop.vuo" << "MakeList1" << true << QStringList("Add1");
		QTest::newRow("Published input transmits events only") << "PublishedOutputGather.vuo" << publishedInputNodeIdentifier << false << QStringList();

		{
			QStringList downstreamNodes;
			downstreamNodes << "Subtract1" << "Subtract2";
			QTest::newRow("Published input transmits data") << "Subtract_published.vuo" << publishedInputNodeIdentifier << true << downstreamNodes;
		}
		{
			QStringList downstreamNodes;
			downstreamNodes << "MakeList" << "Add";
			QTest::newRow("Published input into drawer : published input node") << "PublishedInputIntoDrawer.vuo" << publishedInputNodeIdentifier << true << downstreamNodes;
		}

		QTest::newRow("Published input into drawer : drawer node") << "PublishedInputIntoDrawer.vuo" << "MakeList" << false << QStringList("Add");
	}
	void testNodesDownstreamViaDataOnlyTransmission()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, selectedNodeIdentifier);
		QFETCH(bool, isSourceNode);
		QFETCH(QStringList, expectedDownstreamNodeIdentifiers);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		VuoCompilerNode *selectedNode = NULL;
		for (VuoCompilerNode *node : graph.getNodes())
		{
			if (node->getIdentifier() == selectedNodeIdentifier.toStdString())
			{
				selectedNode = node;
				break;
			}
		}
		QVERIFY(selectedNode);

		set<VuoCompilerNode *> sourceNodes = graph.getSourceNodesOfDataOnlyTransmission();
		QCOMPARE(sourceNodes.find(selectedNode) != sourceNodes.end(), isSourceNode);

		vector<VuoCompilerNode *> downstreamNodes = graph.getNodesDownstreamViaDataOnlyTransmission(selectedNode);

		QStringList downstreamNodeIdentifiers;
		for (VuoCompilerNode *downstreamNode : downstreamNodes)
			downstreamNodeIdentifiers.append(QString::fromStdString(downstreamNode->getIdentifier()));

		downstreamNodeIdentifiers.sort();
		expectedDownstreamNodeIdentifiers.sort();

		QCOMPARE(downstreamNodeIdentifiers, expectedDownstreamNodeIdentifiers);

		delete parser;
	}

	void testPublishedOutputTriggers_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< QString >("publishedOutput");
		QTest::addColumn< bool >("isTrigger");

		QTest::newRow("Internal trigger overlapping published input trigger immediately") << "TriggerOverlappingPublishedInputImmediately.vuo" << "Remainder" << false;
		QTest::newRow("Internal trigger overlapping published input trigger downstream") << "TriggerOverlappingPublishedInputDownstream.vuo" << "Remainder" << false;
		QTest::newRow("Internal trigger overlapping published input trigger at published output port") << "TriggerOverlappingPublishedInputAtOutput.vuo" << "SameValue" << false;
		QTest::newRow("No cables connected to published inputs") << "PublishedInputNotConnected.vuo" << "Remainder" << true;
		QTest::newRow("Internal trigger not overlapping published input trigger") << "TriggerSeparateFromPublishedInput.vuo" << "Quotient" << true;
		QTest::newRow("Published input trigger not overlapping internal trigger") << "TriggerSeparateFromPublishedInput.vuo" << "Remainder" << false;
		QTest::newRow("Overlap but published input events don't reach published outputs") << "TriggerOverlappingBlockedPublishedInput.vuo" << "BuiltList" << true;
		QTest::newRow("Overlap but internal trigger events don't reach published outputs") << "BlockedTriggerOverlappingPublishedInput.vuo" << "HeldValue" << false;
		QTest::newRow("Overlap but neither events reach published outputs") << "BlockedTriggerOverlappingBlockedPublishedInput.vuo" << "HeldValue" << false;
	}
	void testPublishedOutputTriggers()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, publishedOutput);
		QFETCH(bool, isTrigger);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		compiler->setCompositionPath(compositionPath);
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		VuoCompilerGraph graph(&composition);

		QVERIFY(composition.getBase()->getPublishedOutputPortWithName(publishedOutput.toStdString()));

		set<string> triggers = graph.getPublishedOutputTriggers();
		QCOMPARE(triggers.find(publishedOutput.toStdString()) != triggers.end(), isTrigger);

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
		compiler->setCompositionPath(compositionPath);
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string bcFile;
		VuoFileUtilities::splitPath(bcPath, dir, bcFile, extension);
		string prefix = VuoStringUtilities::transcodeToIdentifier(bcFile) + "__";

		for (int i = 0; i < 2; ++i)
		{
			bool isTopLevelComposition = (i == 0);
			VuoCompilerIssues issues;
			compiler->compileComposition(compositionPath, bcPath, isTopLevelComposition, &issues);

			Module *module = VuoCompiler::readModuleFromBitcode(bcPath, compiler->getArch());
			QVERIFY(module);
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

};

QTEST_APPLESS_MAIN(TestVuoCompilerBitcodeGenerator)
#include "TestVuoCompilerBitcodeGenerator.moc"
