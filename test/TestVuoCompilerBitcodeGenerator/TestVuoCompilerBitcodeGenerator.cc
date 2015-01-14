/**
 * @file
 * TestVuoCompilerBitcodeGenerator interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fstream>
#include "TestVuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerDebug.hh"
#include "VuoFileUtilities.hh"
#include "VuoPort.hh"


typedef map<string, set<string> > chainsMap;  ///< Typedef needed for Q_DECLARE_METATYPE and QFETCH to compile. (The comma causes an error.)

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(chainsMap);


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
			for (vector<VuoCompilerChain *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				chainStrings.push_back(  VuoCompilerDebug::chainToString(*j) );
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

	void testEdges_data()
	{
		QTest::addColumn< QString >("nodeTitle");
		QTest::addColumn< size_t >("triggerInEdgeCount");
		QTest::addColumn< size_t >("passiveInEdgeCount");
		QTest::addColumn< size_t >("passiveOutEdgeCount");

		QTest::newRow("Trigger node") << "FirePeriodically1" << (size_t)0 << (size_t)0 << (size_t)0;
		QTest::newRow("Node between trigger and non-trigger") << "Count1" << (size_t)1 << (size_t)0 << (size_t)1;
		QTest::newRow("Node between two non-triggers") << "Convert Integer to Text1" << (size_t)0 << (size_t)1 << (size_t)1;
		QTest::newRow("Node after non-trigger") << "DisplayConsoleWindow1" << (size_t)0 << (size_t)1 << (size_t)0;
	}
	void testEdges()
	{
		QFETCH(QString, nodeTitle);
		QFETCH(size_t, triggerInEdgeCount);
		QFETCH(size_t, passiveInEdgeCount);
		QFETCH(size_t, passiveOutEdgeCount);

		string compositionPath = getCompositionPath("Recur_Count_Write.vuo");
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);

		map<string, VuoNode *> nodeForTitle = makeNodeForTitle(generator->composition->getBase()->getNodes());
		VuoCompilerNode *node = nodeForTitle[nodeTitle.toUtf8().constData()]->getCompiler();
		QVERIFY(node != NULL);

		QCOMPARE(generator->triggerInEdgesForNode[node].size(), triggerInEdgeCount);
		QCOMPARE(generator->passiveInEdgesForNode[node].size(), passiveInEdgeCount);
		QCOMPARE(generator->passiveOutEdgesForNode[node].size(), passiveOutEdgeCount);

		delete generator;
	}

	void testEdgeMayTransmitThroughNode_data()
	{
		QTest::addColumn< QString >("fromNodeTitle");
		QTest::addColumn< QString >("toNodeTitle");
		QTest::addColumn< bool >("mayTransmitThroughNode");

		QTest::newRow("always-transmitting cable and never-transmitting cable") << "FireonStart1" << "SelectInput1" << true;
		QTest::newRow("never-transmitting cable") << "FireonStart2" << "SelectInput2" << false;
		QTest::newRow("sometimes-transmitting cable") << "FireonStart3" << "SelectInput3" << true;
	}
	void testEdgeMayTransmitThroughNode()
	{
		QFETCH(QString, fromNodeTitle);
		QFETCH(QString, toNodeTitle);
		QFETCH(bool, mayTransmitThroughNode);

		string compositionPath = getCompositionPath("Semiconductor.vuo");
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);

		map<string, VuoNode *> nodeForTitle = makeNodeForTitle(generator->composition->getBase()->getNodes());
		VuoNode *toNode = nodeForTitle[toNodeTitle.toUtf8().constData()];
		QVERIFY(toNode != NULL);

		set<VuoCompilerTriggerEdge *> triggerInEdges = generator->triggerInEdgesForNode[toNode->getCompiler()];
		QCOMPARE(triggerInEdges.size(), (size_t)1);

		VuoCompilerTriggerEdge *edge = *triggerInEdges.begin();
		QCOMPARE(QString(edge->getFromNode()->getBase()->getTitle().c_str()), fromNodeTitle);
		QCOMPARE(QString(edge->getToNode()->getBase()->getTitle().c_str()), toNodeTitle);
		QVERIFY(edge->mayTransmitThroughNode() == mayTransmitThroughNode);

		delete generator;
	}

	void testOrderedNodes_data()
	{
		QTest::addColumn< QString >("compositionFile");

		QTest::newRow("Trigger with self-loop and non-blocking output port.") << "TriggerPortWithOutputPort.vuo";
		QTest::newRow("Trigger pushing 2 nodes that gather at the 2nd node.") << "Recur_Count_Count_gather.vuo";
		QTest::newRow("Trigger in a loop with 1 other node.") << "Recur_Count_loop.vuo";
		QTest::newRow("2 triggers with overlapping paths.") << "2Recur_2Count_Count.vuo";
		QTest::newRow("0 triggers.") << "Add.vuo";
		QTest::newRow("Trigger pushing several nodes, including 2 nodes in a feedback loop.") << "Recur_Hold_Add_Write_loop.vuo";
		QTest::newRow("Feedback loop with a trigger cable bypassing it.") << "TriggerBypassFeedbackLoop.vuo";
		QTest::newRow("Feedback loop with a passive cable bypassing it.") << "PassiveBypassFeedbackLoop.vuo";
		QTest::newRow("Trigger cable to each node in a feedback loop.") << "TriggerEachFeedbackLoopNode.vuo";
		QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText.vuo";
		QTest::newRow("Published input and output ports.") << "Recur_Subtract_published.vuo";
	}
	void testOrderedNodes()
	{
		QFETCH(QString, compositionFile);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);
		set<VuoNode *> expectedNodes = generator->composition->getBase()->getNodes();
		vector<VuoCompilerNode *> orderedNodes = generator->orderedNodes;
		set<VuoCompilerNode *> loopEndNodes = generator->loopEndNodes;

		// Check that each element of expectedNodes is also in orderedNodes.
		vector<VuoCompilerNode *> orderedNodesRemaining = orderedNodes;
		for (set<VuoNode *>::iterator i = expectedNodes.begin(); i != expectedNodes.end(); ++i)
		{
			VuoCompilerNode *expectedNode = (*i)->getCompiler();
			bool shouldBeInOrderedNodes = (! generator->passiveInEdgesForNode[expectedNode].empty() ||
										   ! generator->triggerInEdgesForNode[expectedNode].empty());  /// @todo This doesn't handle a composition with two connected nodes and no trigger.
			bool isInOrderedNodes = find(orderedNodes.begin(), orderedNodes.end(), expectedNode) != orderedNodes.end();
			QVERIFY2(shouldBeInOrderedNodes == isInOrderedNodes, (*i)->getTitle().c_str());

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

		// Check that, for each edge in the composition, the from-node precedes the to-node in orderedNodes.
		for (set<VuoNode *>::iterator i = expectedNodes.begin(); i != expectedNodes.end(); ++i)
		{
			VuoCompilerNode *expectedNode = (*i)->getCompiler();
			set<VuoCompilerPassiveEdge *> outEdges = generator->passiveOutEdgesForNode[expectedNode];
			for (set<VuoCompilerPassiveEdge *>::iterator j = outEdges.begin(); j != outEdges.end(); ++j)
			{
				VuoCompilerPassiveEdge *outEdge = *j;
				int fromNodeIndex;
				for (fromNodeIndex = 0; fromNodeIndex < orderedNodes.size(); ++fromNodeIndex)
					if (orderedNodes.at(fromNodeIndex) == outEdge->getFromNode())
						break;
				int toNodeIndex;
				for (toNodeIndex = 0; toNodeIndex < orderedNodes.size(); ++toNodeIndex)
					if (orderedNodes.at(toNodeIndex) == outEdge->getToNode())
						break;
				QVERIFY(fromNodeIndex < orderedNodes.size());
				QVERIFY(toNodeIndex < orderedNodes.size());
				QVERIFY(fromNodeIndex < toNodeIndex ||
						loopEndNodes.find(outEdge->getToNode()) != loopEndNodes.end());
			}
		}

		delete generator;
	}

	void testChains_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< chainsMap >("expectedChains");

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
			chains["FirePeriodically1:fired"].insert("Count1 Count2");
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
			chains["FirePeriodically1:fired"].insert("Subtract1 DiscardDatafromEvent2");
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
			chains["FirePeriodically:fired"].insert("ConvertIntegertoText DisplayConsoleWindow");
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
			chains["FirePeriodically:fired"].insert("DiscardDatafromEvent");
			chains["FirePeriodically:fired"].insert("Hold3");
			chains["FirePeriodically:fired"].insert("MakeList Append");
			chains["FirePeriodically:fired"].insert("CountCharacters Subtract");
			chains["FirePeriodically:fired"].insert("Cut");
			chains["FirePeriodically:fired"].insert("Hold3");
			chains["FirePeriodically:fired"].insert("DisplayConsoleWindow");
			QTest::newRow("Scatters and gathers involving nodes in a feedback loop.") << "StoreRecentText.vuo" << chains;
		}

		{
			map<string, set<string> > chains;
			chains["FirePeriodically1:fired"].insert("Hold1 Subtract2");
			QTest::newRow("Node with incoming cable into walled port and outgoing cable from done port.") << "DonePort.vuo" << chains;
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
			QTest::newRow("Published input and output ports.") << "Recur_Subtract_published.vuo" << chains;
		}
	}
	void testChains()
	{
		QFETCH(QString, compositionFile);
		QFETCH(chainsMap, expectedChains);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);
		map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = generator->nodeForTrigger;

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

		delete generator;
	}

	void testInfiniteFeedbackLoops_data()
	{
		QTest::addColumn< QString >("compositionFile");

		QTest::newRow("Infinite feedback loop containing 2 nodes.") << "TriggerPortWithOutputPort_Count_infiniteLoop.vuo";
		QTest::newRow("Infinite feedback loop containing 1 node.") << "Recur_Count_infiniteLoop.vuo";
		QTest::newRow("No feedback loops.") << "Recur_Count_Count_gather_Count.vuo";
	}
	void testInfiniteFeedbackLoops()
	{
		QFETCH(QString, compositionFile);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);

		QEXPECT_FAIL("No feedback loops.", "@todo: Check that no exception was thrown - https://b33p.net/kosada/node/2341", Continue);
		QVERIFY(generator->downstreamEdgesForEdge.empty());

		delete generator;
	}

	void testDeadlockedFeedbackLoops_data()
	{
		QTest::addColumn< QString >("compositionFile");

		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 2-node feedback loop.") << "DeadlockedFeedbackLoop2Nodes.vuo";
		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 3-node feedback loop.") << "DeadlockedFeedbackLoop3Nodes.vuo";
		QTest::newRow("Trigger ambiguously pushing 2 nodes in a 4-node feedback loop.") << "DeadlockedFeedbackLoop4Nodes.vuo";
		QTest::newRow("Trigger unambiguously pushing 2 nodes in a 2-node feedback loop.") << "AsymmetricFeedbackLoop.vuo";
		QTest::newRow("Trigger unambiguously pushing 2 nested feedback loops.") << "NestedLoopsBothTriggered.vuo";
		QTest::newRow("Trigger unambiguously pushing nodes not in a feedback loop.") << "NonDeadlockedWithSemiconductor.vuo";
	}
	void testDeadlockedFeedbackLoops()
	{
		QFETCH(QString, compositionFile);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(compositionPath, compiler);

		QEXPECT_FAIL("Trigger unambiguously pushing 2 nodes in a 2-node feedback loop.", "@todo: Check that no exception was thrown - https://b33p.net/kosada/node/2341", Continue);
		QEXPECT_FAIL("Trigger unambiguously pushing 2 nested feedback loops.", "@todo: Check that no exception was thrown - https://b33p.net/kosada/node/2341", Continue);
		QEXPECT_FAIL("Trigger unambiguously pushing nodes not in a feedback loop.", "@todo: Check that no exception was thrown - https://b33p.net/kosada/node/2341", Continue);
		QVERIFY(generator->downstreamEdgesForEdge.empty());

		delete generator;
	}

	void testCompilingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Trigger node with no other nodes and no cables.") << "Recur";
		QTest::newRow("Trigger node with a cable to itself.") << "Recur_Count_loop";
		QTest::newRow("Trigger port carrying a VuoInteger.") << "TriggerCarryingInteger";
		QTest::newRow("Trigger port carrying a VuoReal.") << "TriggerCarryingReal";
		QTest::newRow("Trigger port carrying a 32-bit float.") << "TriggerCarryingFloat";
		QTest::newRow("Trigger port carrying a VuoPoint2d.") << "TriggerCarryingPoint2d";
		QTest::newRow("Trigger port carrying a VuoPoint3d.") << "TriggerCarryingPoint3d";
		QTest::newRow("Trigger port carrying a VuoMidiNote.") << "TriggerCarryingMIDINote";
		QTest::newRow("Trigger port carrying a VuoFrameRequest.") << "TriggerCarryingFrameRequest";
		QTest::newRow("Passive output port carrying a VuoMidiNote.") << "OutputCarryingMIDINote";
		QTest::newRow("Passive cable carrying a struct passed by value.") << "CableCarryingStructByValue";
		QTest::newRow("Passive cable carrying a struct coerced to a vector.") << "CableCarryingVuoPoint2d";
		QTest::newRow("Passive cable carrying a struct coerced to a struct containing a vector and a singleton.") << "CableCarryingVuoPoint3d";
		QTest::newRow("Passive cable carrying a struct coerced to a struct containing two vectors.") << "CableCarryingVuoPoint4d";
		QTest::newRow("Node with an input port and output port whose types are pointers to structs.") << "StructPointerPorts";
	}
	void testCompilingWithoutCrashing()
	{
		QFETCH(QString, compositionName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string bcPath = VuoFileUtilities::makeTmpFile(compositionName.toUtf8().constData(), "bc");
		compiler->compileComposition(compositionPath, bcPath);
		remove(bcPath.c_str());
	}

	void testLinkingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("linkType");

		QTest::newRow("Executable") << "EXECUTABLE";
		QTest::newRow("Combined dylib") << "COMBINED_DYLIB";
		QTest::newRow("Composition dylib and resource dylib") << "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB";
	}
	void testLinkingWithoutCrashing()
	{
		QFETCH(QString, linkType);

		string compositionPath = getCompositionPath("Recur.vuo");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, (linkType == "EXECUTABLE" ? "" : "dylib"));
		string linkedResourcePath = VuoFileUtilities::makeTmpFile(file + "-resource", "dylib");

		compiler->compileComposition(compositionPath, compiledCompositionPath);

		if (linkType == "EXECUTABLE")
		{
			compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath);
		}
		else if (linkType == "COMBINED_DYLIB")
		{
			compiler->linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath);
		}
		else if (linkType == "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB")
		{
			vector<string> alreadyLinkedResourcePaths;
			set<string> alreadyLinkedResources;
			compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, linkedResourcePath,
															  alreadyLinkedResourcePaths, alreadyLinkedResources);
		}
		remove(compiledCompositionPath.c_str());

		QVERIFY2(access(linkedCompositionPath.c_str(), 0) == 0, ("Expected to find file " + linkedCompositionPath).c_str());
		remove(linkedCompositionPath.c_str());

		if (linkType == "COMPOSITION_DYLIB_AND_RESOURCE_DYLIB")
		{
			QVERIFY2(access(linkedResourcePath.c_str(), 0) == 0, ("Expected to find file " + linkedResourcePath).c_str());
			remove(linkedResourcePath.c_str());
		}
	}

	void testLinkingMultipleTimes()
	{
		string compositionPath = getCompositionPath("Recur_Count_Write.vuo");
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string exePath = VuoFileUtilities::makeTmpFile(file, "");

		remove(bcPath.c_str());
		remove(exePath.c_str());

		for (int i = 0; i < 2; ++i)
		{
			compiler->compileComposition(compositionPath, bcPath);
			compiler->linkCompositionToCreateExecutable(bcPath, exePath);
			ifstream file(exePath.c_str());
			QVERIFY2(file, qPrintable(QString("Failed to link on iteration %1").arg(i)));
			file.close();

			remove(bcPath.c_str());
			remove(exePath.c_str());
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
