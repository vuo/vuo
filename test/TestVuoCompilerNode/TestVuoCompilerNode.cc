/**
 * @file
 * TestVuoCompilerNode interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <libgen.h>
#include <fcntl.h>
#include "TestVuoCompiler.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoPort.hh"
#include "../../type/VuoInteger.h"


class TestVuoCompilerNode;
typedef Module * (TestVuoCompilerNode::*moduleFunction_t)(void);  ///< A function that creates a @c Module.

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(moduleFunction_t);


/**
 * Tests for using nodes.
 */
class TestVuoCompilerNode : public TestVuoCompiler
{
	Q_OBJECT

private:

	Module * makeSumTestModule();
	Module * makeCounterTestModule();
	Module * makeStartTestModule();
	Module * makeTwoCountersTestModule();
	Module * makeTimerTestModule();

	int numBitsInVuoInteger(void)
	{
		return sizeof(VuoInteger) * CHAR_BIT;
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

	void testPortInitialValue_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("inputPortName");
		QTest::addColumn< QString >("expectedDefaultValue");

		QTest::newRow("non-zero integer") << "vuo.math.divide.VuoInteger" << "b" << "1";
		QTest::newRow("UTF-8 string") << "vuo.test.unicodeDefaultString" << "string" << "\"画\"";
		QTest::newRow("JSON object") << "vuo.color.get.hsl" << "color" << "{\"r\":1,\"g\":1,\"b\":1,\"a\":1}";
	}
	void testPortInitialValue()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, inputPortName);
		QFETCH(QString, expectedDefaultValue);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		VuoNode *node = nodeClass->newNode();
		VuoPort *basePort = node->getInputPortWithName(inputPortName.toStdString());
		VuoCompilerInputEventPort *compilerPort = dynamic_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
		QCOMPARE(QString(compilerPort->getData()->getInitialValue().c_str()), expectedDefaultValue);
	}

	void testGraphvizIdentifierPrefix_data()
	{
		QTest::addColumn< QString >("title");
		QTest::addColumn< QString >("graphvizIdentifierPrefix");

		QTest::newRow("force UpperCamelCase") << "Fire on Start" << "FireOnStart";
		QTest::newRow("leading and trailing spaces") << "    Fire on Start    " << "FireOnStart";
		QTest::newRow("non-alphanumeric characters") << "Fire on Start 42!#@%*(!&" << "FireOnStart42";
		QTest::newRow("leading non-alpha characters") << " !@#^!42 Fire on Start 42!" << "FireOnStart42";
		QTest::newRow("no alpha characters") << " !@#^!42!  " << "Node";
	}
	void testGraphvizIdentifierPrefix()
	{
		QFETCH(QString, title);
		QFETCH(QString, graphvizIdentifierPrefix);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.math.divide.VuoInteger");
		VuoNode *node = nodeClass->newNode(title.toStdString());
		QCOMPARE(QString(node->getCompiler()->getGraphvizIdentifierPrefix().c_str()), graphvizIdentifierPrefix);
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerNode)
#include "TestVuoCompilerNode.moc"
