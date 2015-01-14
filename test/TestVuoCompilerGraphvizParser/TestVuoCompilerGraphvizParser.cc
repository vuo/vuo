/**
 * @file
 * TestVuoCompilerGraphvizParser interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <libgen.h>
#include <fcntl.h>
#include "TestVuoCompiler.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoPort.hh"


class TestVuoCompilerGraphvizParser;
typedef Module * (TestVuoCompilerGraphvizParser::*moduleFunction_t)();  ///< A function that creates a @c Module.

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

	void testParsingConstants()
	{
		string compositionPath = getCompositionPath("DifferenceOfConstants.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		VuoNode * n = parser.getNodes()[0];
		QCOMPARE(n->getInputPorts().size(), (size_t)3);

		VuoCompilerInputEventPort *a = dynamic_cast<VuoCompilerInputEventPort *>(n->getInputPortWithName("a")->getCompiler());
		QCOMPARE(a->getData()->getInitialValue().c_str(), "4444");
		VuoCompilerInputEventPort *b = dynamic_cast<VuoCompilerInputEventPort *>(n->getInputPortWithName("b")->getCompiler());
		QCOMPARE(b->getData()->getInitialValue().c_str(), "5555");
	}

	void testParsingUnicodeConstant()
	{
		string compositionPath = getCompositionPath("LengthOfConstantString.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		VuoNode * n = parser.getNodes()[0];
		VuoCompilerInputEventPort *stringPort = dynamic_cast<VuoCompilerInputEventPort *>(n->getInputPortWithName("text")->getCompiler());

		string valueAsString = stringPort->getData()->getInitialValue();
		QCOMPARE(valueAsString.c_str(), "流");
	}

	void testParsingRendererAttributes()
	{
		string compositionPath = getCompositionPath("RenderingAttributes.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		{
			VuoNode * n = parser.getNodes()[0];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddTinted"));
			QCOMPARE(n->getX(), 22);
			QCOMPARE(n->getY(), 42);
			QCOMPARE(n->getTintColor(), VuoNode::TintYellow);
			QVERIFY(! n->isCollapsed());
		}

		{
			VuoNode * n = parser.getNodes()[1];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddCollapsed"));
			QVERIFY(n->isCollapsed());
		}

		{
			VuoNode * n = parser.getNodes()[2];
			QCOMPARE(QString::fromStdString(n->getTitle()), QString("AddUncollapsed"));
			QVERIFY(!n->isCollapsed());
		}
	}

	void testEmptyStringConstantOverridingNonEmptyDefaultInputPortValue()
	{
		string compositionPath = getCompositionPath("NonEmptyDefaultString.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		VuoNode *n = parser.getNodes()[0];
		VuoCompilerInputEventPort *port = dynamic_cast<VuoCompilerInputEventPort *>(n->getInputPortWithName("string")->getCompiler());
//		QEXPECT_FAIL("", "Known bug: https://b33p.net/kosada/node/3175", Continue);  /// @todo Is this bug fixed?
		QCOMPARE(port->getData()->getInitialValue().c_str(), "");
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
			outputs.push_back("done");
			outputs.push_back("fired");
			QTest::newRow("Fire Periodically node") << "FirePeriodically1" << "vuo.time.firePeriodically" << inputs << outputs;
		}

		{
			vector<string> inputs;
			inputs.push_back("refresh");
			inputs.push_back("increment");
			inputs.push_back("decrement");
			vector<string> outputs;
			outputs.push_back("done");
			outputs.push_back("count");
			QTest::newRow("Count node") << "Count1" << "vuo.math.count.integer" << inputs << outputs;
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
		VuoCompilerGraphvizParser parser(compositionPath);

		vector<VuoNode *> nodes = parser.getNodes();
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
		QVERIFY(node->getDonePort() != NULL);
	}

	void testPublishedPorts()
	{
		string compositionPath = getCompositionPath("Recur_Subtract_published.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		vector<VuoCompilerPublishedPort *> publishedInputs = parser.getPublishedInputPorts();
		QCOMPARE(publishedInputs.size(), (size_t)2);
		foreach (VuoCompilerPublishedPort *input, publishedInputs)
		{
			QVERIFY2(input->getBase()->getName() == "publishedA" ||
					 input->getBase()->getName() == "publishedB",
					 input->getBase()->getName().c_str());
			QCOMPARE(QString(input->getBase()->getType()->getModuleKey().c_str()), QString("VuoInteger"));
		}

		vector<VuoCompilerPublishedPort *> publishedOutputs = parser.getPublishedOutputPorts();
		QCOMPARE(publishedOutputs.size(), (size_t)1);
		QCOMPARE(QString(publishedOutputs.at(0)->getBase()->getName().c_str()), QString("publishedSum"));
		QCOMPARE(QString(publishedOutputs.at(0)->getBase()->getType()->getModuleKey().c_str()), QString("VuoInteger"));

		VuoNode *publishedInputNode = parser.getPublishedInputNode();
		QCOMPARE(publishedInputNode->getOutputPorts().size(), (size_t)3);
		QVERIFY(publishedInputNode->getOutputPortWithName("publishedA") != NULL);
		QVERIFY(publishedInputNode->getOutputPortWithName("publishedB") != NULL);
	}

	void testParsingConstantsForPublishedPorts()
	{
		string compositionPath = getCompositionPath("Recur_Subtract_published.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		bool foundNode = false;
		foreach (VuoNode *node, parser.getNodes())
		{
			if (node->getTitle() == "Subtract1")
			{
				foundNode = true;

				{
					VuoPort *port = node->getInputPortWithName("a");
					VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>(port->getCompiler());
					QCOMPARE(QString::fromStdString(eventPort->getData()->getInitialValue()), QString("10"));
				}

				{
					VuoPort *port = node->getInputPortWithName("b");
					VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>(port->getCompiler());
					QCOMPARE(QString::fromStdString(eventPort->getData()->getInitialValue()), QString("0"));
				}
			}
		}
		QVERIFY(foundNode);
	}

	void testMakeListNodes()
	{
		string compositionPath = getCompositionPath("MakeList.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);

		int makeListNodeCount = 0;
		foreach (VuoNode *node, parser.getNodes())
		{
			if (node->getTitle() == "Make List")
			{
				++makeListNodeCount;
				QVERIFY(node->getCompiler() != NULL);

				QCOMPARE(QString(node->getNodeClass()->getDefaultTitle().c_str()), QString("Make List"));
				QVERIFY(! node->getNodeClass()->getDescription().empty());
				QCOMPARE(QString(node->getNodeClass()->getVersion().c_str()), QString("1.0.0"));

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
				QCOMPARE(outputPorts.size(), (size_t)2);
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
	}

	void testUnknownNodeClass()
	{
		string compositionPath = getCompositionPath("UnknownNodeClass.vuo");
		VuoCompilerGraphvizParser parser(compositionPath, compiler);
		vector<string> unknownNodeClasses = parser.getUnknownNodeClasses();

		QCOMPARE(unknownNodeClasses.size(), (size_t)1);

		string unknownNodeClass = unknownNodeClasses.at(0);
		QCOMPARE(QString(unknownNodeClass.c_str()), QString("UnknownNodeClass"));
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerGraphvizParser)
#include "TestVuoCompilerGraphvizParser.moc"
