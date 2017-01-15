/**
 * @file
 * TestVuoCompilerComposition interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestVuoCompiler.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerPort.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include <sstream>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoComposition *);
Q_DECLARE_METATYPE(set<string>);

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
		QTest::addColumn<QString>("expectedDiff");

		VuoNode *startNode = compiler->getNodeClass("vuo.event.fireOnStart")->newNode("FireOnStart1");
		VuoNode *roundNode = compiler->getNodeClass("vuo.math.round")->newNode("Round1");

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[]";
			QTest::newRow("empty composition -> empty composition") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			QString expectedDiff = "[{\"add\":\"FireOnStart1\",\"value\":{\"nodeClass\":\"vuo.event.fireOnStart\"}}]";
			QTest::newRow("empty composition -> one node") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"}]";
			QTest::newRow("one node -> empty composition") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(roundNode);
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"},{\"add\":\"Round1\",\"value\":{\"nodeClass\":\"vuo.math.round\"}}]";
			QTest::newRow("remove a node and add a node") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			oldComposition->addNode(roundNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			newComposition->addNode(roundNode);
			QString expectedDiff = "[]";
			QTest::newRow("keep the same nodes") << oldComposition << newComposition << expectedDiff;
		}
	}
	void testDiff()
	{
		QFETCH(VuoComposition *, oldComposition);
		QFETCH(VuoComposition *, newComposition);
		QFETCH(QString, expectedDiff);

		VuoCompilerComposition oldCompilerComposition(oldComposition, NULL);
		string oldCompositionSerialized = oldCompilerComposition.getGraphvizDeclaration();
		VuoCompilerComposition newCompilerComposition(newComposition, NULL);

		string actualDiff = newCompilerComposition.diffAgainstOlderComposition(oldCompositionSerialized, compiler,
																			   set<VuoCompilerComposition::NodeReplacement>());
		QCOMPARE(QString::fromStdString(actualDiff), expectedDiff);
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

};

QTEST_APPLESS_MAIN(TestVuoCompilerComposition)
#include "TestVuoCompilerComposition.moc"
