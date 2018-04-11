/**
 * @file
 * TestVuoCompilerComposition interface and implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestVuoCompiler.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include <sstream>

typedef map<string, string> PublishedPortReplacements;  ///< Typedef needed for Q_DECLARE_METATYPE and QFETCH to compile.

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoComposition *);
Q_DECLARE_METATYPE(set<string>);
Q_DECLARE_METATYPE(set<VuoCompilerComposition::NodeReplacement>);
Q_DECLARE_METATYPE(PublishedPortReplacements);

/**
 * Tests for using compositions.
 */
class TestVuoCompilerComposition : public TestVuoCompiler
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		initCompiler();
	}

	void cleanupTestCase()
	{
		cleanupCompiler();
	}

	void testDiff_data()
	{
		QTest::addColumn<VuoComposition *>("oldComposition");
		QTest::addColumn<VuoComposition *>("newComposition");
		QTest::addColumn< set<VuoCompilerComposition::NodeReplacement> >("nodeReplacements");
		QTest::addColumn<PublishedPortReplacements>("publishedPortReplacements");
		QTest::addColumn<QString>("expectedDiff");

		VuoNode *startNode = compiler->getNodeClass("vuo.event.fireOnStart")->newNode("FireOnStart1");
		VuoNode *roundNode = compiler->getNodeClass("vuo.math.round")->newNode("Round1");
		VuoNode *makeList1Node = compiler->getNodeClass("vuo.list.make.1.VuoInteger")->newNode("MakeList1");
		VuoNode *makeList2Node = compiler->getNodeClass("vuo.list.make.2.VuoInteger")->newNode("MakeList2");

		VuoCompilerPublishedPortClass *portClass1 = new VuoCompilerPublishedPortClass("publishedInput1", VuoPortClass::eventOnlyPort, NULL);
		VuoPublishedPort *publishedInput1 = static_cast<VuoPublishedPort *>(portClass1->newPort()->getBase());

		VuoCompilerPublishedPortClass *portClass2 = new VuoCompilerPublishedPortClass("publishedInput2", VuoPortClass::eventOnlyPort, NULL);
		VuoPublishedPort *publishedInput2 = static_cast<VuoPublishedPort *>(portClass2->newPort()->getBase());

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[]";
			QTest::newRow("empty composition -> empty composition") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			QString expectedDiff = "[{\"add\":\"FireOnStart1\",\"value\":{\"nodeClass\":\"vuo.event.fireOnStart\"}}]";
			QTest::newRow("empty composition -> one node") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"}]";
			QTest::newRow("one node -> empty composition") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(roundNode);
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"},{\"add\":\"Round1\",\"value\":{\"nodeClass\":\"vuo.math.round\"}}]";
			QTest::newRow("remove a node and add a node") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			oldComposition->addNode(roundNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			newComposition->addNode(roundNode);
			QString expectedDiff = "[]";
			QTest::newRow("keep the same nodes") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(makeList1Node);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(makeList2Node);
			VuoCompilerComposition::NodeReplacement nodeReplacement;
			nodeReplacement.oldNodeIdentifier = "MakeList1";
			nodeReplacement.newNodeIdentifier = "MakeList2";
			nodeReplacement.oldAndNewPortIdentifiers["1"] = "1";
			nodeReplacement.oldAndNewPortIdentifiers["list"] = "list";
			set<VuoCompilerComposition::NodeReplacement> nodeReplacements;
			nodeReplacements.insert(nodeReplacement);
			QString expectedDiff = "[{\"remove\":\"MakeList1\"},{\"add\":\"MakeList2\",\"value\":{\"nodeClass\":\"vuo.list.make.2.VuoInteger\"}},{\"map\":\"MakeList1\",\"to\":\"MakeList2\",\"ports\":[{\"map\":\"1\",\"to\":\"1\"},{\"map\":\"list\",\"to\":\"list\"}]}]";
			QTest::newRow("replace a node") << oldComposition << newComposition << nodeReplacements << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addPublishedInputPort(publishedInput1, 0);
			QString expectedDiff = "[{\"add\":\"PublishedInputs\"},{\"add\":\"PublishedOutputs\"}]";
			QTest::newRow("add first published port") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addPublishedInputPort(publishedInput1, 0);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addPublishedInputPort(publishedInput1, 0);
			newComposition->addPublishedInputPort(publishedInput2, 0);
			QString expectedDiff = "[{\"remove\":\"PublishedInputs\"},{\"add\":\"PublishedInputs\"},{\"map\":\"PublishedInputs\",\"to\":\"PublishedInputs\",\"ports\":[{\"map\":\"publishedInput1\",\"to\":\"publishedInput1\"}]}]";
			QTest::newRow("add second published port") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << map<string, string>() << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addPublishedInputPort(publishedInput1, 0);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addPublishedInputPort(publishedInput2, 0);
			map<string, string> publishedPortReplacements;
			publishedPortReplacements["publishedInput1"] = "publishedInput2";
			QString expectedDiff = "[{\"remove\":\"PublishedInputs\"},{\"add\":\"PublishedInputs\"},{\"map\":\"PublishedInputs\",\"to\":\"PublishedInputs\",\"ports\":[]}]";
			QTest::newRow("rename a published port") << oldComposition << newComposition << set<VuoCompilerComposition::NodeReplacement>() << publishedPortReplacements << expectedDiff;
		}
	}
	void testDiff()
	{
		QFETCH(VuoComposition *, oldComposition);
		QFETCH(VuoComposition *, newComposition);
		QFETCH(set<VuoCompilerComposition::NodeReplacement>, nodeReplacements);
		QFETCH(PublishedPortReplacements, publishedPortReplacements);
		QFETCH(QString, expectedDiff);

		VuoCompilerComposition oldCompilerComposition(oldComposition, NULL);
		string oldCompositionSerialized = oldCompilerComposition.getGraphvizDeclaration();
		VuoCompilerComposition newCompilerComposition(newComposition, NULL);

		string actualDiff = newCompilerComposition.diffAgainstOlderComposition(oldCompositionSerialized, compiler, nodeReplacements);

		json_object *expectedDiffObj = json_tokener_parse(expectedDiff.toStdString().c_str());
		json_object *actualDiffObj = json_tokener_parse(actualDiff.c_str());

		int expectedNumChanges = json_object_array_length(expectedDiffObj);
		int actualNumChanges = json_object_array_length(actualDiffObj);
		QVERIFY2(actualNumChanges == expectedNumChanges, actualDiff.c_str());

		map<string, set<string> > changesForNode[2];
		map<string, map<string, map<string, string> > > portMappingsForNodes[2];
		for (int i = 0; i < 2; ++i)
		{
			json_object *diffObj = (i == 0 ? expectedDiffObj : actualDiffObj);
			for (int j = 0; j < expectedNumChanges; ++j)
			{
				json_object *change = json_object_array_get_idx(diffObj, j);
				json_object_object_foreach(change, key, val)
				{
					string node;
					if (json_object_get_type(val) == json_type_string)
						node = json_object_get_string(val);

					changesForNode[i][node].insert(key);

					if (! strcmp(key, "map"))
					{
						json_object *o;

						string oldNode = node;

						string newNode;
						if (json_object_object_get_ex(change, "to", &o))
							newNode = json_object_get_string(o);

						json_object *portMappingArray = NULL;
						if (json_object_object_get_ex(change, "ports", &o))
							portMappingArray = o;

						int numPortMappings = json_object_array_length(portMappingArray);
						for (int k = 0; k < numPortMappings; ++k)
						{
							json_object *portMapping = json_object_array_get_idx(portMappingArray, k);

							string oldPort;
							if (json_object_object_get_ex(portMapping, "map", &o))
								oldPort = json_object_get_string(o);

							string newPort;
							if (json_object_object_get_ex(portMapping, "to", &o))
								newPort = json_object_get_string(o);

							portMappingsForNodes[i][oldNode][newNode][oldPort] = newPort;
						}
					}
				}
			}
		}

		QVERIFY2(changesForNode[0] == changesForNode[1], actualDiff.c_str());
		QVERIFY2(portMappingsForNodes[0] == portMappingsForNodes[1], actualDiff.c_str());
	}

	void testGenericPortTypes()
	{
		string compositionPath = getCompositionPath("Generics.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		delete parser;

		map<QString, QString> typeForNodeAndPort;
		set<VuoNode *> nodes = composition.getBase()->getNodes();
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoNode *node = *i;
			vector<VuoPort *> inputPorts = node->getInputPorts();
			vector<VuoPort *> outputPorts = node->getOutputPorts();
			vector<VuoPort *> ports;
			ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
			ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
			for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
			{
				VuoPort *port = *j;
				VuoType *type = dynamic_cast<VuoCompilerPort *>(port->getCompiler())->getDataVuoType();
				if (type)
				{
					QString nodeAndPort = QString::fromStdString(node->getTitle() + ":" + port->getClass()->getName());
					QString typeName = QString::fromStdString(type->getModuleKey());
					typeForNodeAndPort[nodeAndPort] = typeName;
				}
			}
		}

		// Non-generic types
		QCOMPARE(typeForNodeAndPort["HoldValue1:heldValue"], QString("VuoText"));

		// Generic types
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue2:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue3:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue4:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue5:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue6:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["HoldValue7:heldValue"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["Subtract1:a"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["Subtract2:a"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["Subtract3:a"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["GetMessageValues1:data1"].toStdString()));
		QVERIFY(VuoGenericType::isGenericTypeName(typeForNodeAndPort["GetMessageValues1:data2"].toStdString()));

		// Ports on same node with same generic types
		QCOMPARE(typeForNodeAndPort["HoldValue2:initialValue"],typeForNodeAndPort["HoldValue2:heldValue"]);
		QCOMPARE(typeForNodeAndPort["Subtract1:a"], typeForNodeAndPort["Subtract1:difference"]);
		QCOMPARE(typeForNodeAndPort["Subtract3:a"], typeForNodeAndPort["Subtract3:b"]);

		// Ports on same node with different generic types
		QVERIFY(typeForNodeAndPort["GetMessageValues1:data1"] != typeForNodeAndPort["GetMessageValues1:data2"]);

		// Unconnected ports - different generic types
		QVERIFY(typeForNodeAndPort["HoldValue2:heldValue"] != typeForNodeAndPort["Subtract1:a"]);
		QVERIFY(typeForNodeAndPort["HoldValue2:heldValue"] != typeForNodeAndPort["HoldValue3:heldValue"]);
		QVERIFY(typeForNodeAndPort["HoldValue2:heldValue"] != typeForNodeAndPort["HoldValue5:heldValue"]);
		QVERIFY(typeForNodeAndPort["HoldValue2:heldValue"] != typeForNodeAndPort["GetMessageValues1:data1"]);
		QVERIFY(typeForNodeAndPort["HoldValue2:heldValue"] != typeForNodeAndPort["GetMessageValues1:data2"]);
		QVERIFY(typeForNodeAndPort["Subtract1:a"] != typeForNodeAndPort["HoldValue3:heldValue"]);
		QVERIFY(typeForNodeAndPort["Subtract1:a"] != typeForNodeAndPort["HoldValue5:heldValue"]);
		QVERIFY(typeForNodeAndPort["Subtract1:a"] != typeForNodeAndPort["GetMessageValues1:data1"]);
		QVERIFY(typeForNodeAndPort["Subtract1:a"] != typeForNodeAndPort["GetMessageValues1:data2"]);
		QVERIFY(typeForNodeAndPort["HoldValue3:heldValue"] != typeForNodeAndPort["HoldValue5:heldValue"]);
		QVERIFY(typeForNodeAndPort["HoldValue3:heldValue"] != typeForNodeAndPort["GetMessageValues1:data1"]);
		QVERIFY(typeForNodeAndPort["HoldValue3:heldValue"] != typeForNodeAndPort["GetMessageValues1:data2"]);

		// Connected ports - same generic types
		QCOMPARE(typeForNodeAndPort["HoldValue3:heldValue"], typeForNodeAndPort["HoldValue4:heldValue"]);
		QCOMPARE(typeForNodeAndPort["HoldValue3:heldValue"], typeForNodeAndPort["HoldValue6:heldValue"]);
		QCOMPARE(typeForNodeAndPort["HoldValue5:heldValue"], typeForNodeAndPort["Subtract2:a"]);
		QCOMPARE(typeForNodeAndPort["HoldValue5:heldValue"], typeForNodeAndPort["HoldValue7:heldValue"]);
		QCOMPARE(typeForNodeAndPort["GetMessageValues1:data2"], typeForNodeAndPort["Subtract3:a"]);
	}

	void testGenericPortTypeCompatibility_data()
	{
		QTest::addColumn<QString>("nodeTitle");
		QTest::addColumn<QString>("portName");
		QTest::addColumn< set<string> >("compatibleTypeNames");

		set<string> allTypes;
		set<string> numericalTypes;
		numericalTypes.insert("VuoInteger");
		numericalTypes.insert("VuoReal");
		set<string> numericalAndPointTypes;
		numericalAndPointTypes.insert("VuoInteger");
		numericalAndPointTypes.insert("VuoReal");
		numericalAndPointTypes.insert("VuoPoint2d");
		numericalAndPointTypes.insert("VuoPoint3d");
		numericalAndPointTypes.insert("VuoPoint4d");
		set<string> oscTypes;
		oscTypes.insert("VuoBoolean");
		oscTypes.insert("VuoInteger");
		oscTypes.insert("VuoReal");
		oscTypes.insert("VuoText");

		QTest::newRow("generic port, unconnected, compatible with any")										<< "HoldValue2"			<< "heldValue"	<< allTypes;
		QTest::newRow("generic port, unconnected, compatible with some")									<< "Subtract1"			<< "a"			<< numericalAndPointTypes;
		QTest::newRow("generic port, unconnected but on same node as connected, compatible with some")		<< "GetMessageValues1"	<< "data1"		<< oscTypes;
		QTest::newRow("generic port, connected directly, compatible with any")								<< "HoldValue3"			<< "heldValue"	<< allTypes;
		QTest::newRow("generic port, connected directly, was compatible with any, compatibility reduced")	<< "HoldValue5"			<< "heldValue"	<< numericalAndPointTypes;
		QTest::newRow("generic port, connected directly, was compatible with some, compatibility reduced")	<< "GetMessageValues1"	<< "data2"		<< numericalTypes;
		QTest::newRow("generic port, connected indirectly, compatible with any")							<< "HoldValue6"			<< "newValue"	<< allTypes;
		QTest::newRow("generic port, connected indirectly, was compatible with any, compatibility reduced") << "HoldValue7"			<< "newValue"	<< numericalAndPointTypes;
	}
	void testGenericPortTypeCompatibility()
	{
		QFETCH(QString, nodeTitle);
		QFETCH(QString, portName);
		QFETCH(set<string>, compatibleTypeNames);

		string compositionPath = getCompositionPath("Generics.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		delete parser;

		bool foundType = false;
		set<VuoNode *> nodes = composition.getBase()->getNodes();
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoNode *node = *i;
			if (node->getTitle() != nodeTitle.toStdString())
				continue;

			vector<VuoPort *> inputPorts = node->getInputPorts();
			vector<VuoPort *> outputPorts = node->getOutputPorts();
			vector<VuoPort *> ports;
			ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
			ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
			for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
			{
				VuoPort *port = *j;
				if (port->getClass()->getName() != portName.toStdString())
					continue;

				foundType = true;

				VuoType *type = dynamic_cast<VuoCompilerPort *>(port->getCompiler())->getDataVuoType();
				QVERIFY(type != NULL);

				VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(type);
				QVERIFY(genericType != NULL);

				VuoGenericType::Compatibility compatibility;
				vector<string> actualCompatibleTypeNames = genericType->getCompatibleSpecializedTypes(compatibility);
				for (set<string>::iterator k = compatibleTypeNames.begin(); k != compatibleTypeNames.end(); ++k)
					QVERIFY2(find(actualCompatibleTypeNames.begin(), actualCompatibleTypeNames.end(), *k) != actualCompatibleTypeNames.end(), (*k).c_str());
				QCOMPARE(compatibleTypeNames.size(), actualCompatibleTypeNames.size());
			}
		}
		QVERIFY(foundType);
	}

	void testUpdateGenericPortTypesRepeatedly()
	{
		VuoCompilerComposition composition(new VuoComposition(), NULL);
		VuoCompilerNodeClass *holdNodeClass = compiler->getNodeClass("vuo.data.hold");

		for (unsigned int i = 1; i <= 5; ++i)
		{
			ostringstream nodeClassName;
			nodeClassName << "Hold" << i;
			VuoNode *hold = holdNodeClass->newNode(nodeClassName.str());
			composition.getBase()->addNode(hold);

			composition.updateGenericPortTypes();

			map<string, bool> genericTypesSeen;
			set<VuoNode *> nodes = composition.getBase()->getNodes();
			for (set<VuoNode *>::iterator j = nodes.begin(); j != nodes.end(); ++j)
			{
				VuoNode *node = *j;

				vector<VuoPort *> outputPorts = node->getOutputPorts();
				for (vector<VuoPort *>::iterator k = outputPorts.begin(); k != outputPorts.end(); ++k)
				{
					VuoPort *port = *k;

					VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
					VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(compilerPort->getDataVuoType());
					string genericTypeName = genericType->getModuleKey();
					QVERIFY2(! genericTypesSeen[genericTypeName], genericTypeName.c_str());
					genericTypesSeen[genericTypeName] = true;
				}
			}

			for (unsigned int j = 1; j <= i; ++j)
			{
				string genericTypeName = VuoGenericType::createGenericTypeName(j);
				QVERIFY2(genericTypesSeen[genericTypeName], genericTypeName.c_str());
			}
			QVERIFY(genericTypesSeen.size() == i);
		}
	}

	void testNearestUpstreamTriggerPort_data()
	{
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<QString>("nodeTitle");
		QTest::addColumn<QString>("triggerNodeTitle");
		QTest::addColumn<QString>("triggerPortName");

		{
			QTest::newRow("no upstream trigger — no cable") << "NoUpstream-NoCable" << "Count Characters" << "" << "";
		}
		{
			QTest::newRow("no upstream trigger — wall") << "NoUpstream-Wall" << "Ripple Image" << "" << "";
		}
		{
			QTest::newRow("one upstream trigger — direct cable") << "OneUpstream-DirectCable" << "Make Text Layer" << "Fire on Start" << "started";
		}
		{
			QTest::newRow("one upstream trigger — no event blocking") << "OneUpstream-NoBlocking" << "Combine Layers" << "Render Layers to Window" << "requestedFrame";
		}
		{
			QTest::newRow("one upstream trigger — same node") << "OneUpstream-NoBlocking" << "Render Layers to Window" << "Render Layers to Window" << "requestedFrame";
		}
		{
			QTest::newRow("one upstream trigger — door") << "OneUpstream-Door" << "Render Image to Window" << "Receive Live Video" << "receivedFrame";
		}
		{
			QTest::newRow("two upstream triggers — no event blocking, one closer") << "TwoUpstream-OneCloser" << "Make Sphere" << "Render Scene to Window" << "requestedFrame";
		}
		{
			QTest::newRow("two upstream triggers — no event blocking, equidistant") << "TwoUpstream-Equidistant" << "Display Console Window" << "Fire on Start" << "started";
		}
		{
			QTest::newRow("two upstream triggers — one with no event blocking, one with door") << "TwoUpstream-Door" << "Count" << "Fire on Start" << "started";
		}
	}
	void testNearestUpstreamTriggerPort()
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, nodeTitle);
		QFETCH(QString, triggerNodeTitle);
		QFETCH(QString, triggerPortName);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition composition(new VuoComposition(), parser);
		delete parser;

		// Set node identfiers for ports.
		VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(&composition, true, "", compiler);
		delete generator;

		map<string, VuoNode *> nodeForTitle = makeNodeForTitle(composition.getBase()->getNodes());

		VuoNode *node = nodeForTitle[nodeTitle.toStdString()];

		VuoPort *expectedTriggerPort = NULL;
		if (! triggerNodeTitle.isEmpty())
		{
			VuoNode *triggerNode = nodeForTitle[triggerNodeTitle.toStdString()];
			expectedTriggerPort = triggerNode->getOutputPortWithName(triggerPortName.toStdString());
		}

		VuoPort *actualTriggerPort = composition.findNearestUpstreamTriggerPort(node);
		QVERIFY2(actualTriggerPort == expectedTriggerPort, actualTriggerPort ? actualTriggerPort->getClass()->getName().c_str() : "(null)");
	}

	void testGraphHash_data()
	{
		QTest::addColumn<long>("hash1");
		QTest::addColumn<long>("hash2");
		QTest::addColumn<bool>("hashesEqual");

		string compositionPath = getCompositionPath("GraphHashTest.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("identical VuoCompilerCompositions") << hash1 << hash2 << true;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			VuoCompilerGraphvizParser *parser2 = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			VuoCompilerComposition composition2(new VuoComposition(), parser2);
			delete parser2;
			long hash1 = VuoCompilerGraph::getHash(&composition);
			long hash2 = VuoCompilerGraph::getHash(&composition2);
			QTest::newRow("same source code, different VuoCompilerCompositions") << hash1 << hash2 << false;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			VuoNode *addedNode1 = compiler->createNode(compiler->getNodeClass("vuo.layer.make.text"));
			composition.getBase()->addNode(addedNode1);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			composition.getBase()->removeNode(addedNode1);
			delete addedNode1;
			VuoNode *addedNode2 = compiler->createNode(compiler->getNodeClass("vuo.layer.make.text"));
			composition.getBase()->addNode(addedNode2);
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("compositions differ by a node replaced with a similar node") << hash1 << hash2 << false;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			map<string, VuoNode *> nodeForTitle = makeNodeForTitle(composition.getBase()->getNodes());
			VuoNode *blurNode = nodeForTitle["Blur Image"];
			VuoPort *blurredPort = blurNode->getOutputPortWithName("blurredImage");
			VuoCable *cable = blurredPort->getConnectedCables().at(0);
			composition.getBase()->removeCable(cable);
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("compositions differ by a cable") << hash1 << hash2 << false;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			map<string, VuoNode *> nodeForTitle = makeNodeForTitle(composition.getBase()->getNodes());
			VuoCompilerNode *captureNode = nodeForTitle["Capture Image of Screen"]->getCompiler();
			VuoCompilerPort *screenPort = static_cast<VuoCompilerPort *>( captureNode->getBase()->getInputPortWithName("screen")->getCompiler() );
			VuoCompilerCable *publishedCable = new VuoCompilerCable(NULL, NULL, captureNode, screenPort);
			VuoPort *imagePort = composition.getBase()->getPublishedInputPortWithName("Image");
			VuoNode *publishedInputNode = imagePort->getConnectedCables().at(0)->getFromNode();
			publishedCable->getBase()->setFrom(publishedInputNode, imagePort);
			publishedCable->setAlwaysEventOnly(true);
			composition.getBase()->addCable(publishedCable->getBase());
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("compositions differ by a published cable") << hash1 << hash2 << false;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			composition.getBase()->removePublishedInputPort(1);
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("compositions differ by a published input port") << hash1 << hash2 << false;
		}
		{
			VuoCompilerComposition composition(new VuoComposition(), parser);
			long hash1 = VuoCompilerGraph::getHash(&composition);
			VuoCompilerPublishedPortClass *publishedPortClass = new VuoCompilerPublishedPortClass("test", VuoPortClass::eventOnlyPort, NULL);
			VuoPublishedPort *publishedPort = static_cast<VuoPublishedPort *>( publishedPortClass->newPort()->getBase() );
			composition.getBase()->addPublishedInputPort(publishedPort, 2);
			long hash2 = VuoCompilerGraph::getHash(&composition);
			QTest::newRow("compositions differ by a published output port") << hash1 << hash2 << false;
		}
		{
			string compositionPath2 = getCompositionPath("UnknownNodeClass.vuo");
			VuoCompilerGraphvizParser *parser2 = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath2, compiler);
			VuoCompilerComposition composition2(new VuoComposition(), parser2);
			delete parser2;
			long hash1 = VuoCompilerGraph::getHash(&composition2);
			long hash2 = VuoCompilerGraph::getHash(&composition2);
			QTest::newRow("composition contains an unknown node class") << hash1 << hash2 << true;
		}

		delete parser;
	}
	void testGraphHash()
	{
		QFETCH(long, hash1);
		QFETCH(long, hash2);
		QFETCH(bool, hashesEqual);

		QCOMPARE((hash1 == hash2), hashesEqual);
	}

};

QTEST_APPLESS_MAIN(TestVuoCompilerComposition)
#include "TestVuoCompilerComposition.moc"
