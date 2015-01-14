/**
 * @file
 * TestVuoCompilerNodeClass interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <fcntl.h>
#include "TestVuoCompiler.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(vector<string>);
Q_DECLARE_METATYPE(VuoPortClass::EventBlocking);


/**
 * Tests for using node classes.
 */
class TestVuoCompilerNodeClass : public TestVuoCompiler
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

	void testPortOrder_data()
	{
		QTest::addColumn< QString >("nodeClass");
		QTest::addColumn< vector<string> >("expectedInputNames");
		QTest::addColumn< vector<string> >("expectedOutputNames");

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");  // Refresh should always be the 0th port of getInputPortClasses(), even though it's the last input port specified in Hold's nodeInstanceEvent method.
			inputNames.push_back("initialValue");
			inputNames.push_back("newValue");
			vector<string> outputNames;
			outputNames.push_back("done");
			outputNames.push_back("heldValue");
			QTest::newRow("Node class with explicit event blocking") << "vuo.hold.VuoInteger" << inputNames << outputNames;
		}

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");  // Refresh should always be the 0th port of getInputPortClasses(), even though it's not specified at all in Add's nodeEvent method.
			inputNames.push_back("a");
			inputNames.push_back("b");
			vector<string> outputNames;
			outputNames.push_back("done");
			outputNames.push_back("difference");
			QTest::newRow("Node class without explicit event blocking") << "vuo.math.subtract.integer" << inputNames << outputNames;
		}

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");  // Refresh should always be the 0th port of getInputPortClasses(), even though it's not specified at all in Fire Periodically's nodeInstanceEvent method.
			inputNames.push_back("seconds");
			vector<string> outputNames;
			outputNames.push_back("done");
			outputNames.push_back("fired");
			QTest::newRow("Node class with a trigger in both the callbackStart and event functions") << "vuo.time.firePeriodically" << inputNames << outputNames;
		}

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");
			inputNames.push_back("inputData0");
			inputNames.push_back("inputData1");
			inputNames.push_back("inputData2");
			inputNames.push_back("inputData3");
			vector<string> outputNames;
			outputNames.push_back("done");
			QTest::newRow("Node class with input data in the init, callbackStart, and event functions") << "vuo.test.inputDataPortOrder" << inputNames << outputNames;
		}
	}
	void testPortOrder()
	{
		QFETCH(QString, nodeClass);
		QFETCH(vector<string>, expectedInputNames);
		QFETCH(vector<string>, expectedOutputNames);

		VuoNodeClass *nc = compiler->getNodeClass( qPrintable(nodeClass) )->getBase();

		vector<VuoPortClass *> actualInputs = nc->getInputPortClasses();
		QCOMPARE(actualInputs.size(), expectedInputNames.size());
		for (uint i = 0; i < actualInputs.size(); ++i)
		{
			string actualName = actualInputs.at(i)->getName();
			string expectedName = expectedInputNames.at(i);
			QCOMPARE(QString(actualName.c_str()), QString(expectedName.c_str()));
		}

		vector<VuoPortClass *> actualOutputs = nc->getOutputPortClasses();
		QCOMPARE(actualOutputs.size(), expectedOutputNames.size());
		for (uint i = 0; i < actualOutputs.size(); ++i)
		{
			string actualName = actualOutputs.at(i)->getName();
			string expectedName = expectedOutputNames.at(i);
			QCOMPARE(QString(actualName.c_str()), QString(expectedName.c_str()));
		}
	}

	void testPortDefaultValue_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("inputPortName");
		QTest::addColumn< QString >("expectedDefaultValue");

		QTest::newRow("non-zero integer") << "vuo.math.divide.integer" << "b" << "1";
		QTest::newRow("UTF-8 string") << "vuo.test.unicodeDefaultString" << "string" << "\"画\"";
		QTest::newRow("JSON object") << "vuo.color.get.hsl" << "color" << "{\"r\":1,\"g\":1,\"b\":1,\"a\":1}";
	}
	void testPortDefaultValue()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, inputPortName);
		QFETCH(QString, expectedDefaultValue);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		VuoPortClass *basePortClass = nodeClass->getInputPortClassWithName(inputPortName.toStdString());
		VuoCompilerInputEventPortClass *compilerPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>(basePortClass->getCompiler());
		QCOMPARE(QString(compilerPortClass->getDataClass()->getDefaultValue().c_str()), expectedDefaultValue);
	}

	void testPortEventBlocking_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("portClassName");
		QTest::addColumn< bool >("isInput");
		QTest::addColumn< VuoPortClass::EventBlocking >("expectedEventBlocking");

		QTest::newRow("VuoInputEvent")					<< "vuo.select.in.2.event"		<< "option1"	<< true		<< VuoPortClass::EventBlocking_Door;
		QTest::newRow("VuoInputEvent and VuoInputData") << "vuo.select.in.2.event"		<< "which"		<< true		<< VuoPortClass::EventBlocking_Wall;
		QTest::newRow("VuoInputData")						<< "vuo.text.countCharacters"	<< "text"		<< true		<< VuoPortClass::EventBlocking_None;
		QTest::newRow("refresh")							<< "vuo.text.countCharacters"	<< "refresh"	<< true		<< VuoPortClass::EventBlocking_None;
		QTest::newRow("output port")						<< "vuo.select.in.2.event"		<< "out"		<< false	<< VuoPortClass::EventBlocking_None;
	}
	void testPortEventBlocking()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, portClassName);
		QFETCH(bool, isInput);
		QFETCH(VuoPortClass::EventBlocking, expectedEventBlocking);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toUtf8().constData());
		VuoPortClass *portClass = (isInput ?
									   nodeClass->getInputPortClassWithName(portClassName.toUtf8().constData()) :
									   nodeClass->getOutputPortClassWithName(portClassName.toUtf8().constData()));
		QCOMPARE(portClass->getEventBlocking(), expectedEventBlocking);
	}

	void testStructPorts_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("portClassName");
		QTest::addColumn< QString >("typeName");
		QTest::addColumn< bool >("isInputPort");
		QTest::addColumn< bool >("isLoweredToTwoParameters");

		QTest::newRow("Not a struct port") << "vuo.point.make.2d" << "x" << "VuoReal" << true << false;
		QTest::newRow("VuoPoint2d output port") << "vuo.point.make.2d" << "point" << "VuoPoint2d" << false << false;
		QTest::newRow("VuoPoint2d input port") << "vuo.point.get.2d" << "point" << "VuoPoint2d" << true << false;
		QTest::newRow("VuoPoint3d input port") << "vuo.point.get.3d" << "point" << "VuoPoint3d" << true << true;
		QTest::newRow("VuoPoint4d input port") << "vuo.point.get.4d" << "point" << "VuoPoint4d" << true << true;
	}
	void testStructPorts()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, portClassName);
		QFETCH(QString, typeName);
		QFETCH(bool, isInputPort);
		QFETCH(bool, isLoweredToTwoParameters);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toUtf8().constData());
		VuoPortClass *portClass = (isInputPort ?
									   nodeClass->getInputPortClassWithName(portClassName.toUtf8().constData()) :
									   nodeClass->getOutputPortClassWithName(portClassName.toUtf8().constData()));
		QVERIFY(portClass != NULL);

		VuoType *expectedType = compiler->getType(typeName.toUtf8().constData())->getBase();
		VuoType *actualType = static_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDataVuoType();
		QCOMPARE(QString(actualType->getModuleKey().c_str()), QString(expectedType->getModuleKey().c_str()));
		QCOMPARE(actualType, expectedType);
		QVERIFY(actualType->getCompiler() != NULL);

		if (isInputPort)
		{
			VuoCompilerInputDataClass *dataClass = static_cast<VuoCompilerInputEventPortClass *>(portClass->getCompiler())->getDataClass();
			QVERIFY(dataClass->isLoweredToTwoParameters() == isLoweredToTwoParameters);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerNodeClass)
#include "TestVuoCompilerNodeClass.moc"
