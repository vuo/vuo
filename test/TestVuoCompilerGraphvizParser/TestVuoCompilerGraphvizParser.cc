/**
 * @file
 * TestVuoCompilerGraphvizParser interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <libgen.h>
#include <fcntl.h>
#include <fstream>
#include "TestVuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerInputData.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoType.hh"


class TestVuoCompilerGraphvizParser;
typedef Module * (TestVuoCompilerGraphvizParser::*moduleFunction_t)(void);  ///< A function that creates a @c Module.

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(vector<string>);
Q_DECLARE_METATYPE(moduleFunction_t);


/**
 * Tests for the compiler's Graphviz parser.
 */
class TestVuoCompilerGraphvizParser : public TestVuoCompiler
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

	void testConstants_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< QString >("nodeTitle");
		QTest::addColumn< QString >("portName");
		QTest::addColumn< QString >("expectedConstant");

		QTest::newRow("no cable") << "Constants.vuo" << "Subtract1" << "a" << "11";
		QTest::newRow("data-and-event cable") << "Constants.vuo" << "Subtract1" << "b" << "0";
		QTest::newRow("event cable from event port") << "Constants.vuo" << "Subtract2" << "a" << "33";
		QTest::newRow("event cable from event port") << "Constants.vuo" << "Subtract2" << "b" << "44";
		QTest::newRow("non-ASCII text") << "LengthOfConstantString.vuo" << "Count Characters" << "text" << "流";
		QTest::newRow("empty text overriding non-empty default value") << "NonEmptyDefaultString.vuo" << "UnicodeDefaultString1" << "string" << "\"\"";
		QTest::newRow("published data-and-event cable with published canstant") << "Recur_Subtract_published.vuo" << "Subtract1" << "a" << "10";
		QTest::newRow("published data-and-event cable without published canstant") << "Recur_Subtract_published.vuo" << "Subtract1" << "b" << "";
		QTest::newRow("published data-and-event cable") << "Constants.vuo" << "Subtract4" << "a" << "0";
		QTest::newRow("published event cable from published event port") << "Constants.vuo" << "Subtract3" << "a" << "55";
		QTest::newRow("published event cable from published data-and-event port") << "Constants.vuo" << "Subtract3" << "b" << "66";
	}
	void testConstants()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, nodeTitle);
		QFETCH(QString, portName);
		QFETCH(QString, expectedConstant);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		bool foundNode = false;
		foreach (VuoNode *n, parser->getNodes())
		{
			if (n->getTitle() == nodeTitle.toStdString())
			{
				VuoPort *basePort = n->getInputPortWithName(portName.toStdString());
				VuoCompilerInputEventPort *compilerPort = dynamic_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
				VuoCompilerInputData *data = compilerPort->getData();
				QCOMPARE(QString(data->getInitialValue().c_str()), expectedConstant);

				foundNode = true;
				break;
			}
		}
		QVERIFY(foundNode);

		delete parser;
	}

	void testParsingRendererAttributes()
	{
		string compositionPath = getCompositionPath("RenderingAttributes.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		{
			VuoNode * n = parser->getNodes()[0];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddTinted"));
			QCOMPARE(n->getX(), 22);
			QCOMPARE(n->getY(), 42);
			QCOMPARE(n->getTintColor(), VuoNode::TintYellow);
			QVERIFY(! n->isCollapsed());
		}

		{
			VuoNode * n = parser->getNodes()[1];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddCollapsed"));
			QVERIFY(n->isCollapsed());
		}

		{
			VuoNode * n = parser->getNodes()[2];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddUncollapsed"));
			QVERIFY(!n->isCollapsed());
		}

		delete parser;
	}

	void testCables_data()
	{
		QTest::addColumn< QString >("fromNodeTitle");
		QTest::addColumn< QString >("fromPort");
		QTest::addColumn< QString >("toNodeTitle");
		QTest::addColumn< QString >("toPort");
		QTest::addColumn< bool >("carriesData");
		QTest::addColumn< bool >("isHidden");

		QTest::newRow("data-and-event cable") << "ShareValue1" << "sameValue" << "ShareValue2" << "value" << true << false;
		QTest::newRow("event-only cable between event-only ports") << "BecameTrue1" << "becameTrue" << "ShareValue3" << "refresh" << false << false;
		QTest::newRow("event-only cable between data-and-event port and event-only port") << "BecameTrue2" << "becameTrue" << "ShareValue4" << "value" << false << false;
		QTest::newRow("event-only cable between data-and-event ports") << "ShareValue5" << "sameValue" << "ShareValue6" << "value" << false << false;

		QTest::newRow("data-and-event published input cable") << "PublishedInputs" << "value1" << "ShareValue7" << "value" << true << false;
		QTest::newRow("event-only published input cable between event-only ports") << "PublishedInputs" << "refresh1" << "ShareValue8" << "refresh" << false << false;
		QTest::newRow("event-only published input cable between data-and-event ports") << "PublishedInputs" << "value2" << "ShareValue9" << "value" << false << false;

		QTest::newRow("data-and-event published output cable") << "ShareValue10" << "sameValue" << "PublishedOutputs" << "sameValue1" << true << false;
		QTest::newRow("event-only published output cable between event-only ports") << "BecameTrue3" << "becameTrue" << "PublishedOutputs" << "becameTrue1" << false << false;
		QTest::newRow("event-only published output cable between data-and-event ports") << "ShareValue11" << "sameValue" << "PublishedOutputs" << "sameValue2" << false << false;
		
		QTest::newRow("hidden data-and-event cable") << "ShareValue12" << "sameValue" << "ShareValue13" << "value" << true << true;
		QTest::newRow("hidden always-event-only cable") << "ShareValue14" << "sameValue" << "ShareValue15" << "value" << false << true;
	}
	void testCables()
	{
		QFETCH(QString, fromNodeTitle);
		QFETCH(QString, fromPort);
		QFETCH(QString, toNodeTitle);
		QFETCH(QString, toPort);
		QFETCH(bool, carriesData);
		QFETCH(bool, isHidden);

		string compositionPath = getCompositionPath("Cables.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		bool foundCable = false;
		vector<VuoCable *> cables = parser->getCables();

		foreach (VuoCable *cable, cables)
		{
			if (cable->getFromNode()->getTitle() == fromNodeTitle.toStdString() &&
					cable->getFromPort()->getClass()->getName() == fromPort.toStdString() &&
					cable->getToNode()->getTitle() == toNodeTitle.toStdString() &&
					cable->getToPort()->getClass()->getName() == toPort.toStdString())
			{
				QVERIFY(! foundCable);
				foundCable = true;

				QVERIFY(cable->getCompiler()->carriesData() == carriesData);
				QVERIFY(cable->getCompiler()->getHidden() == isHidden);
			}
		}
		QVERIFY(foundCable);

		delete parser;
	}

	void testDummyNodes_data()
	{
		QTest::addColumn< QString >("nodeTitle");
		QTest::addColumn< QString >("expectedNodeClass");
		QTest::addColumn< vector<string> >("expectedInputPortNames");
		QTest::addColumn< vector<string> >("expectedOutputPortNames");

		{
			vector<string> inputs;
			inputs.push_back("refresh");
			inputs.push_back("seconds");
			vector<string> outputs;
			outputs.push_back("fired");
			QTest::newRow("Fire Periodically node") << "FirePeriodically1" << "vuo.time.firePeriodically" << inputs << outputs;
		}

		{
			vector<string> inputs;
			inputs.push_back("refresh");
			inputs.push_back("increment");
			inputs.push_back("decrement");
			vector<string> outputs;
			outputs.push_back("count");
			QTest::newRow("Count node") << "Count1" << "vuo.math.count.VuoInteger" << inputs << outputs;
		}
	}
	void testDummyNodes()
	{
		QFETCH(QString, nodeTitle);
		QFETCH(QString, expectedNodeClass);
		QFETCH(vector<string>, expectedInputPortNames);
		QFETCH(vector<string>, expectedOutputPortNames);

		// Instantiate parser _without_ a compiler.
		string compositionPath = getCompositionPath("Recur_Count.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath);

		vector<VuoNode *> nodes = parser->getNodes();
		QCOMPARE(nodes.size(), (size_t)2);

		map<string, VuoNode *> nodeForTitle = makeNodeForTitle(nodes);
		VuoNode *node = nodeForTitle[qPrintable(nodeTitle)];
		QVERIFY(! node->hasCompiler());

		string actualNodeClass = node->getNodeClass()->getClassName();
		QCOMPARE(QString(actualNodeClass.c_str()), expectedNodeClass);

		vector<VuoPort *> actualInputs = node->getInputPorts();
		QCOMPARE(actualInputs.size(), expectedInputPortNames.size());
		for (uint i = 0; i < actualInputs.size(); ++i)
		{
			string actualInputPortName = actualInputs.at(i)->getClass()->getName();
			string expectedInputPortName = expectedInputPortNames.at(i);
			QCOMPARE(QString(actualInputPortName.c_str()), QString(expectedInputPortName.c_str()));
		}

		vector<VuoPort *> actualOutputs = node->getOutputPorts();
		QCOMPARE(actualOutputs.size(), expectedOutputPortNames.size());
		for (uint i = 0; i < actualOutputs.size(); ++i)
		{
			string actualOutputPortName = actualOutputs.at(i)->getClass()->getName();
			string expectedOutputPortName = expectedOutputPortNames.at(i);
			QCOMPARE(QString(actualOutputPortName.c_str()), QString(expectedOutputPortName.c_str()));
		}

		QVERIFY(node->getRefreshPort() != NULL);

		delete parser;
	}

	void testPublishedPorts()
	{
		string compositionPath = getCompositionPath("Recur_Subtract_published.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		vector<VuoPublishedPort *> publishedInputs = parser->getPublishedInputPorts();
		QCOMPARE(publishedInputs.size(), (size_t)2);
		{
			QCOMPARE(QString(publishedInputs[0]->getClass()->getName().c_str()), QString("publishedA"));
			string typeName = static_cast<VuoCompilerPublishedPort *>(publishedInputs[0]->getCompiler())->getDataVuoType()->getModuleKey();
			QCOMPARE(QString(typeName.c_str()), QString("VuoInteger"));
		}
		{
			QCOMPARE(QString(publishedInputs[1]->getClass()->getName().c_str()), QString("publishedB"));
			string typeName = static_cast<VuoCompilerPublishedPort *>(publishedInputs[1]->getCompiler())->getDataVuoType()->getModuleKey();
			QCOMPARE(QString(typeName.c_str()), QString("VuoInteger"));
		}

		vector<VuoPublishedPort *> publishedOutputs = parser->getPublishedOutputPorts();
		QCOMPARE(publishedOutputs.size(), (size_t)1);
		{
			QCOMPARE(QString(publishedOutputs[0]->getClass()->getName().c_str()), QString("publishedSum"));
			string typeName = static_cast<VuoCompilerPublishedPort *>(publishedOutputs[0]->getCompiler())->getDataVuoType()->getModuleKey();
			QCOMPARE(QString(typeName.c_str()), QString("VuoInteger"));
		}

		delete parser;
	}

	void testPublishedPortDetails_data()
	{
		QTest::addColumn< QString >("publishedPortName");
		QTest::addColumn< bool >("isInput");
		QTest::addColumn< QString >("expectedDetails");

		QTest::newRow("all from published input port") << "hue" << true << "{\"default\":0.3,\"suggestedMin\":0.2,\"suggestedMax\":1.1,\"suggestedStep\":0.1}";
		QTest::newRow("all from published output port") << "color" << false << "{}";
		QTest::newRow("some from published port, some from connected ports") << "saturation" << true << "{\"default\":0.500000,\"suggestedMin\":0,\"suggestedMax\":1,\"suggestedStep\":1}";
		QTest::newRow("none from published port, no connected ports") << "image" << true << "{}";
	}
	void testPublishedPortDetails()
	{
		QFETCH(QString, publishedPortName);
		QFETCH(bool, isInput);
		QFETCH(QString, expectedDetails);

		string compositionPath = getCompositionPath("PublishedPorts.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		bool foundPublishedPort = false;
		foreach (VuoPublishedPort *publishedPort, (isInput ? parser->getPublishedInputPorts() : parser->getPublishedOutputPorts()))
		{
			if (publishedPort->getClass()->getName() == publishedPortName.toStdString())
			{
				json_object *detailsObj = static_cast<VuoCompilerPublishedPort *>(publishedPort->getCompiler())->getDetails(isInput);
				json_object *expectedDetailsObj = json_tokener_parse(expectedDetails.toUtf8().constData());

				{
					json_object_object_foreach(expectedDetailsObj, key, val)
					{
						json_object *o;
						bool foundKey = json_object_object_get_ex(detailsObj, key, &o);
						QVERIFY2(foundKey, key);
						QCOMPARE(QString(json_object_to_json_string(o)), QString(json_object_to_json_string(val)));
					}
				}
				{
					json_object_object_foreach(detailsObj, key, val)
					{
						json_object *o;
						bool foundKey = json_object_object_get_ex(expectedDetailsObj, key, &o);
						QVERIFY2(foundKey, key);
						QCOMPARE(QString(json_object_to_json_string(o)), QString(json_object_to_json_string(val)));
					}
				}

				foundPublishedPort = true;
			}
		}
		QVERIFY(foundPublishedPort);

		delete parser;
	}

	void testMakeListNodes()
	{
		string compositionPath = getCompositionPath("MakeList.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		int makeListNodeCount = 0;
		foreach (VuoNode *node, parser->getNodes())
		{
			if (node->getTitle() == "Make List")
			{
				++makeListNodeCount;
				QVERIFY(node->getCompiler() != NULL);

				QCOMPARE(QString(node->getNodeClass()->getDefaultTitle().c_str()), QString("Make List"));
				QVERIFY(! node->getNodeClass()->getDescription().empty());
				QCOMPARE(QString(node->getNodeClass()->getVersion().c_str()), QString("2.0.0"));

				vector<VuoPort *> inputPorts = node->getInputPorts();
				QCOMPARE(inputPorts.size(), (size_t)4);
				foreach (VuoPort *port, inputPorts)
				{
					if (port == node->getRefreshPort())
						continue;

					VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler());
					QVERIFY(eventPort != NULL);
					VuoCompilerInputData *data = eventPort->getData();
					QVERIFY(data != NULL);
					VuoCompilerInputDataClass *dataClass = dynamic_cast<VuoCompilerInputDataClass *>(data->getBase()->getClass()->getCompiler());
					QVERIFY(dataClass != NULL);
					VuoType *type = dataClass->getVuoType();
					QVERIFY(type != NULL);
					QCOMPARE(QString(type->getModuleKey().c_str()), QString("VuoInteger"));
					QVERIFY(type->getCompiler() != NULL);
					QVERIFY(type->getCompiler() == compiler->getType("VuoInteger"));
				}

				vector<VuoPort *> outputPorts = node->getOutputPorts();
				QCOMPARE(outputPorts.size(), (size_t)(1 + VuoNodeClass::unreservedOutputPortStartIndex));
				{
					VuoPort *port = outputPorts.at(VuoNodeClass::unreservedOutputPortStartIndex);
					VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>(port->getCompiler());
					QVERIFY(eventPort != NULL);
					VuoCompilerOutputData *data = eventPort->getData();
					QVERIFY(data != NULL);
					VuoCompilerOutputDataClass *dataClass = dynamic_cast<VuoCompilerOutputDataClass *>(data->getBase()->getClass()->getCompiler());
					QVERIFY(dataClass != NULL);
					VuoType *type = dataClass->getVuoType();
					QVERIFY(type != NULL);
					QCOMPARE(QString(type->getModuleKey().c_str()), QString("VuoList_VuoInteger"));
					QVERIFY(type->getCompiler() != NULL);
					QVERIFY(type->getCompiler() == compiler->getType("VuoList_VuoInteger"));
				}
			}
		}
		QCOMPARE(makeListNodeCount, 1);

		delete parser;
	}

	void testUnknownNodeClass()
	{
		string compositionPath = getCompositionPath("UnknownNodeClass.vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);

		int numUnknownNodeClasses = 0;
		foreach (VuoNode *node, parser->getNodes())
		{
			if (! node->hasCompiler())
			{
				QCOMPARE(QString::fromStdString(node->getTitle()), QString("FirePeriodically1"));
				QCOMPARE(QString::fromStdString(node->getNodeClass()->getClassName()), QString("UnknownNodeClass"));
				++numUnknownNodeClasses;
			}
		}

		QCOMPARE(numUnknownNodeClasses, 1);

		delete parser;
	}

	void testParsingDescription_data()
	{
		QTest::addColumn< QString >("compositionPath");
		QTest::addColumn< QString >("expectedDescription");

		string descriptionPath = getCompositionPath("Description.vuo");
		QTest::newRow("Composition with description") << QString::fromStdString(descriptionPath) << "This is the description.";

		string noDescriptionPath = VuoFileUtilities::makeTmpFile("NoDescription", "vuo");
		ifstream fin(descriptionPath.c_str());
		ofstream fout(noDescriptionPath.c_str());
		string line;
		int lineNum = 0;
		while (getline(fin, line))
			if (++lineNum >= 10)
				fout << line << endl;
		fin.close();
		fout.close();
		QTest::newRow("Composition without Doxygen header") << QString::fromStdString(noDescriptionPath) << "";
	}
	void testParsingDescription()
	{
		QFETCH(QString, compositionPath);
		QFETCH(QString, expectedDescription);

		{
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath.toStdString(), compiler);
			QCOMPARE(QString::fromStdString( parser->getDescription() ), expectedDescription);
			delete parser;
		}

		{
			string compositionAsString;
			ifstream fin(compositionPath.toStdString().c_str());
			string line;
			while (getline(fin, line))
				compositionAsString += line + "\n";
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionAsString, compiler);
			QCOMPARE(QString::fromStdString( parser->getDescription() ), expectedDescription);
			delete parser;
		}
	}

};

QTEST_APPLESS_MAIN(TestVuoCompilerGraphvizParser)
#include "TestVuoCompilerGraphvizParser.moc"
