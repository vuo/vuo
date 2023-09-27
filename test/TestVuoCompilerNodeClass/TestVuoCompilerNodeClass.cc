﻿/**
 * @file
 * TestVuoCompilerNodeClass interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <fcntl.h>
#include "TestVuoCompiler.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(vector<string>);
Q_DECLARE_METATYPE(set<string>);
Q_DECLARE_METATYPE(VuoPortClass::EventBlocking);
Q_DECLARE_METATYPE(VuoGenericType::Compatibility);


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
			outputNames.push_back("heldValue");
			QTest::newRow("Node class with explicit event blocking") << "vuo.data.hold.VuoInteger" << inputNames << outputNames;
		}

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");  // Refresh should always be the 0th port of getInputPortClasses(), even though it's not specified at all in Add's nodeEvent method.
			inputNames.push_back("a");
			inputNames.push_back("b");
			vector<string> outputNames;
			outputNames.push_back("difference");
			QTest::newRow("Node class without explicit event blocking") << "vuo.math.subtract.VuoInteger" << inputNames << outputNames;
		}

		{
			vector<string> inputNames;
			inputNames.push_back("refresh");  // Refresh should always be the 0th port of getInputPortClasses(), even though it's not specified at all in Fire Periodically's nodeInstanceEvent method.
			inputNames.push_back("seconds");
			vector<string> outputNames;
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

	void testPortDetails()
	{
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.details");

		vector<VuoPortClass *> portClasses;
		vector<VuoPortClass *> inputPortClasses = nodeClass->getBase()->getInputPortClasses();
		vector<VuoPortClass *> outputPortClasses = nodeClass->getBase()->getOutputPortClasses();
		portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());

		foreach (VuoPortClass *basePortClass, portClasses)
		{
			if (basePortClass == nodeClass->getBase()->getRefreshPortClass())
				continue;

			string portName = basePortClass->getName();

			VuoCompilerEventPortClass *eventPortClass = dynamic_cast<VuoCompilerEventPortClass *>(basePortClass->getCompiler());
			if (eventPortClass)
			{
				QVERIFY2(eventPortClass->getDetails() != NULL, portName.c_str());
				string eventDetails;
				{
					json_object *o = NULL;
					bool found = json_object_object_get_ex(eventPortClass->getDetails(), "test", &o);
					QVERIFY2(found, portName.c_str());
					eventDetails = json_object_get_string(o);
				}
				string expectedEventDetails = portName + " event";
				QCOMPARE(QString::fromStdString(eventDetails), QString::fromStdString(expectedEventDetails));

				VuoCompilerDataClass *dataClass = eventPortClass->getDataClass();
				if (dataClass)
				{
					QVERIFY2(dataClass->getDetails() != NULL, portName.c_str());
					string dataDetails;
					{
						json_object *o = NULL;
						bool found = json_object_object_get_ex(dataClass->getDetails(), "test", &o);
						QVERIFY2(found, portName.c_str());
						dataDetails = json_object_get_string(o);
					}
					string expectedDataDetails = portName + " data";
					QCOMPARE(QString::fromStdString(dataDetails), QString::fromStdString(expectedDataDetails));
				}
			}
			else
			{
				VuoCompilerTriggerPortClass *triggerPortClass = dynamic_cast<VuoCompilerTriggerPortClass *>(basePortClass->getCompiler());
				QVERIFY2(triggerPortClass->getDetails() != NULL, portName.c_str());
				string details;
				{
					json_object *o = NULL;
					bool found = json_object_object_get_ex(triggerPortClass->getDetails(), "test", &o);
					QVERIFY2(found, portName.c_str());
					details = json_object_get_string(o);
				}
				string expectedDetails = portName + " trigger";
				QCOMPARE(QString::fromStdString(details), QString::fromStdString(expectedDetails));
			}
		}
	}

	void testPortDefaultValue_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("inputPortName");
		QTest::addColumn< QString >("expectedDefaultValue");

		QTest::newRow("non-zero integer") << "vuo.math.divide.VuoInteger" << "b" << "1";
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

		QTest::newRow("event-only door")                    << "vuo.select.in.boolean.event2"<< "falseOption"   << true     << VuoPortClass::EventBlocking_Door;
		QTest::newRow("data-and-event door")                << "vuo.select.in.boolean"       << "falseOption"   << true     << VuoPortClass::EventBlocking_Door;
		QTest::newRow("data-and-event wall")                << "vuo.data.hold"               << "newValue"      << true     << VuoPortClass::EventBlocking_Wall;
		QTest::newRow("data-and-event non-blocking")        << "vuo.text.countCharacters"    << "text"          << true     << VuoPortClass::EventBlocking_None;
		QTest::newRow("refresh")                            << "vuo.text.countCharacters"    << "refresh"       << true     << VuoPortClass::EventBlocking_None;
		QTest::newRow("output port")                        << "vuo.select.in.event2.2"      << "out"           << false    << VuoPortClass::EventBlocking_None;
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

	void testAggregateTypePorts_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("portClassName");
		QTest::addColumn< QString >("typeName");
		QTest::addColumn< bool >("isInputPort");

		QTest::newRow("Not an aggregate type") << "vuo.point.make.VuoPoint2d" << "x" << "VuoReal" << true;
		QTest::newRow("VuoPoint2d output port") << "vuo.point.make.VuoPoint2d" << "point" << "VuoPoint2d" << false;
		QTest::newRow("VuoPoint2d input port") << "vuo.point.get.VuoPoint2d" << "point" << "VuoPoint2d" << true;
		QTest::newRow("VuoPoint3d input port") << "vuo.point.get.VuoPoint3d" << "point" << "VuoPoint3d" << true;
		QTest::newRow("VuoPoint4d input port") << "vuo.point.get.VuoPoint4d" << "point" << "VuoPoint4d" << true;
		QTest::newRow("VuoColor input port") << "vuo.color.get.rgb" << "color" << "VuoColor" << true;
		QTest::newRow("VuoAudioSamples input port") << "vuo.audio.analyze.fft2" << "samples" << "VuoAudioSamples" << true;
	}
	void testAggregateTypePorts()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, portClassName);
		QFETCH(QString, typeName);
		QFETCH(bool, isInputPort);

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
	}

	void testGenericPorts_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< vector<string> >("expectedUniqueGenericTypes");
		QTest::addColumn< vector<string> >("expectedPortTypes");

		{
			vector<string> expectedUniqueGenericTypes;
			QTest::newRow("0 generic types") << "vuo.image.blend" << expectedUniqueGenericTypes;
		}

		{
			vector<string> expectedUniqueGenericTypes;
			expectedUniqueGenericTypes.push_back("VuoGenericType1");
			QTest::newRow("1 generic type in multiple ports") << "vuo.data.hold" << expectedUniqueGenericTypes;
		}

		{
			vector<string> expectedUniqueGenericTypes;
			expectedUniqueGenericTypes.push_back("VuoGenericType1");
			QTest::newRow("VuoList and singleton of generic type") << "vuo.list.get" << expectedUniqueGenericTypes;
		}

		{
			vector<string> expectedUniqueGenericTypes;
			expectedUniqueGenericTypes.push_back("VuoGenericType1");
			QTest::newRow("VuoList of generic type (without singleton)") << "vuo.list.count" << expectedUniqueGenericTypes;
		}

		{
			vector<string> expectedUniqueGenericTypes;
			expectedUniqueGenericTypes.push_back("VuoGenericType1");
			expectedUniqueGenericTypes.push_back("VuoGenericType2");
			QTest::newRow("2 generic types") << "vuo.osc.message.get.2" << expectedUniqueGenericTypes;
		}

		{
			vector<string> expectedUniqueGenericTypes;
			expectedUniqueGenericTypes.push_back("VuoGenericType9");
			expectedUniqueGenericTypes.push_back("VuoGenericType10");
			QTest::newRow("Generic type with multiple digits in its suffix") << "vuo.test.multiDigitGenericTypes" << expectedUniqueGenericTypes;
		}
	}
	void testGenericPorts()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(vector<string>, expectedUniqueGenericTypes);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());

		vector<string> actualGenericTypeNames = VuoCompilerSpecializedNodeClass::getGenericTypeNamesFromPorts(nodeClass);
		for (size_t i = 0; i < expectedUniqueGenericTypes.size(); ++i)
		{
			QVERIFY(i < actualGenericTypeNames.size());
			QCOMPARE(QString::fromStdString(expectedUniqueGenericTypes[i]), QString::fromStdString(actualGenericTypeNames[i]));
		}
		QCOMPARE(expectedUniqueGenericTypes.size(), actualGenericTypeNames.size());
	}

	void testGenericPortCompatibility_data()
	{
		QTest::addColumn< QString >("nodeClassName");
		QTest::addColumn< QString >("portName");
		QTest::addColumn< QString >("expectedType");
		QTest::addColumn< VuoGenericType::Compatibility >("expectedCompatibility");
		QTest::addColumn< set<string> >("expectedCompatibleTypes");

		{
			set<string> expectedCompatibleTypes;
			QTest::newRow("Generic singleton compatible with any") << "vuo.list.get" << "item" << "VuoGenericType1" << VuoGenericType::anyType << expectedCompatibleTypes;
		}

		{
			set<string> expectedCompatibleTypes;
			QTest::newRow("Generic list compatible with any") << "vuo.list.get" << "list" << "VuoList_VuoGenericType1" << VuoGenericType::anyListType << expectedCompatibleTypes;
		}

		{
			set<string> expectedCompatibleTypes;
			expectedCompatibleTypes.insert("VuoInteger");
			expectedCompatibleTypes.insert("VuoReal");
			expectedCompatibleTypes.insert("VuoPoint2d");
			expectedCompatibleTypes.insert("VuoPoint3d");
			expectedCompatibleTypes.insert("VuoPoint4d");
			QTest::newRow("Generic singleton compatible with numeric") << "vuo.math.max" << "max" << "VuoGenericType1" << VuoGenericType::whitelistedTypes << expectedCompatibleTypes;
		}

		{
			set<string> expectedCompatibleTypes;
			expectedCompatibleTypes.insert("VuoList_VuoInteger");
			expectedCompatibleTypes.insert("VuoList_VuoReal");
			expectedCompatibleTypes.insert("VuoList_VuoPoint2d");
			expectedCompatibleTypes.insert("VuoList_VuoPoint3d");
			expectedCompatibleTypes.insert("VuoList_VuoPoint4d");
			QTest::newRow("Generic list compatible with numeric") << "vuo.math.max" << "values" << "VuoList_VuoGenericType1" << VuoGenericType::whitelistedTypes << expectedCompatibleTypes;
		}
	}
	void testGenericPortCompatibility()
	{
		QFETCH(QString, nodeClassName);
		QFETCH(QString, portName);
		QFETCH(QString, expectedType);
		QFETCH(VuoGenericType::Compatibility, expectedCompatibility);
		QFETCH(set<string>, expectedCompatibleTypes);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());

		bool foundPort = false;
		vector<VuoPortClass *> inputPortClasses = nodeClass->getBase()->getInputPortClasses();
		vector<VuoPortClass *> outputPortClasses = nodeClass->getBase()->getOutputPortClasses();
		vector<VuoPortClass *> portClasses;
		portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
		for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
		{
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*i)->getCompiler());
			if (portClass->getBase()->getName() != portName.toStdString())
				continue;

			foundPort = true;

			VuoGenericType *type = dynamic_cast<VuoGenericType *>(portClass->getDataVuoType());
			QCOMPARE(QString::fromStdString(type->getModuleKey()), expectedType);

			VuoGenericType::Compatibility compatibility;
			vector<string> compatibleTypes = type->getCompatibleSpecializedTypes(compatibility);

			QCOMPARE(compatibility, expectedCompatibility);

			for (vector<string>::iterator j = compatibleTypes.begin(); j != compatibleTypes.end(); ++j)
			{
				string compatibleType = *j;
				QVERIFY2(expectedCompatibleTypes.find(compatibleType) != expectedCompatibleTypes.end(), compatibleType.c_str());
			}
			QCOMPARE(compatibleTypes.size(), expectedCompatibleTypes.size());
		}

		QVERIFY(foundPort);
	}

private:

	vector<VuoPublishedPort *> makePublishedPorts(QStringList names, QStringList types)
	{
		vector<VuoPublishedPort *> publishedPorts;

		for (int i = 0; i < names.size(); ++i)
		{
			string portName = names[i].toStdString();
			string typeName = types[i].toStdString();

			VuoType *type = (typeName.empty() ? nullptr : compiler->getType(typeName)->getBase());

			VuoCompilerPublishedPort *port = VuoCompilerPublishedPort::newPort(portName, type);
			publishedPorts.push_back(static_cast<VuoPublishedPort *>(port->getBase()));
		}

		return publishedPorts;
	}

private slots:

	void testPublishedInputNodeClass_data()
	{
		QTest::addColumn< QStringList >("publishedPortNames");
		QTest::addColumn< QStringList >("publishedPortTypes");
		QTest::addColumn< QStringList >("expectedInputPortNames");
		QTest::addColumn< QStringList >("expectedOutputPortNames");

		auto appendOut = [] (QStringList list)
		{
			for (int i = 0; i < list.size(); ++i)
				list[i].append("Out");

			return list;
		};

		{
			QTest::newRow("no published ports") << QStringList() << QStringList() << QStringList() << QStringList();
		}
		{
			QStringList names("a");
			QStringList types("");
			QTest::newRow("one event port") << names << types << names << appendOut(names);
		}
		{
			QStringList names("b");
			QStringList types("VuoInteger");
			QTest::newRow("one data+event port") << names << types << names << appendOut(names);
		}
		{
			QStringList names("c");
			QStringList types("VuoGenericType1");
			QTest::newRow("one unspecialized generic port") << names << types << names << appendOut(names);
		}
		{
			QStringList names;
			QStringList types;
			names << "data1" << "event1" << "data2" << "event2";
			types << "VuoText" << "" << "VuoLayer" << "";
			QTest::newRow("multiple event and data+event ports") << names << types << names << appendOut(names);
		}
		{
			QStringList names;
			names << "refresh" << "a" << "aEvent" << "aOut" << "aOutEvent";
			QStringList inNames;
			inNames << "refresh2" << "a" << "aEvent" << "aOut" << "aOutEvent";
			QStringList outNames;
			outNames << "refresh2Out" << "aOut2" << "aEventOut" << "aOutOut" << "aOutEventOut";

			QStringList specTypes;
			specTypes << "VuoReal" << "VuoSceneObject" << "" << "" << "";
			QTest::newRow("name collisions, no unspecialized generic ports") << names << specTypes << inNames << outNames;

			QStringList unspecTypes;
			unspecTypes << "VuoGenericType1" << "VuoGenericType2" << "" << "" << "";
			QTest::newRow("name collisions, some unspecialized generic ports") << names << unspecTypes << inNames << outNames;
		}
	}
	void testPublishedInputNodeClass()
	{
		QFETCH(QStringList, publishedPortNames);
		QFETCH(QStringList, publishedPortTypes);
		QFETCH(QStringList, expectedInputPortNames);
		QFETCH(QStringList, expectedOutputPortNames);

		vector<VuoPublishedPort *> publishedPorts = makePublishedPorts(publishedPortNames, publishedPortTypes);

		VuoNode *publishedNode = compiler->createPublishedInputNode(publishedPorts);
		VuoCompilerPublishedInputNodeClass *publishedNodeClass = static_cast<VuoCompilerPublishedInputNodeClass *>(publishedNode->getNodeClass()->getCompiler());

		vector<VuoPortClass *> inputPortClasses = publishedNodeClass->getBase()->getInputPortClasses();
		QCOMPARE(inputPortClasses.size(), publishedPorts.size() + VuoNodeClass::unreservedInputPortStartIndex);

		vector<VuoPortClass *> outputPortClasses = publishedNodeClass->getBase()->getOutputPortClasses();
		QCOMPARE(outputPortClasses.size(), publishedPorts.size() + VuoNodeClass::unreservedOutputPortStartIndex);

		for (size_t i = 0; i < publishedPorts.size(); ++i)
		{
			size_t inputPortIndex = i + VuoNodeClass::unreservedInputPortStartIndex;
			size_t outputPortIndex = i + VuoNodeClass::unreservedOutputPortStartIndex;

			VuoPortClass *inputPortClass = inputPortClasses[inputPortIndex];
			VuoPortClass *outputPortClass = outputPortClasses[outputPortIndex];

			QCOMPARE(QString::fromStdString(inputPortClass->getName()), expectedInputPortNames[i]);
			QCOMPARE(QString::fromStdString(outputPortClass->getName()), expectedOutputPortNames[i]);

			QCOMPARE((int)inputPortClass->getEventBlocking(), (int)VuoPortClass::EventBlocking_Door);

			QCOMPARE((int)inputPortClass->getPortType(), (int)(publishedPortTypes[i].isEmpty() ? VuoPortClass::eventOnlyPort : VuoPortClass::dataAndEventPort));
			QCOMPARE((int)outputPortClass->getPortType(), (int)(publishedPortTypes[i].isEmpty() ? VuoPortClass::eventOnlyPort : VuoPortClass::dataAndEventPort));

			QCOMPARE(publishedNodeClass->getInputPortIndexForPublishedInputPort(i), inputPortIndex);
			QCOMPARE(publishedNodeClass->getOutputPortIndexForPublishedInputPort(i), outputPortIndex);
		}
	}

	void testPublishedOutputNodeClass_data()
	{
		QTest::addColumn< QStringList >("publishedPortNames");
		QTest::addColumn< QStringList >("publishedPortTypes");
		QTest::addColumn< QStringList >("expectedInputPortNames");

		{
			QTest::newRow("no published ports") << QStringList() << QStringList() << QStringList();
		}
		{
			QStringList names("a");
			QStringList types("");
			QTest::newRow("one event port") << names << types << names;
		}
		{
			QStringList names("b");
			QStringList types("VuoInteger");
			QTest::newRow("one data+event port") << names << types << names;
		}
		{
			QStringList names("c");
			QStringList types("VuoGenericType1");
			QTest::newRow("one unspecialized generic port") << names << types << names;
		}
		{
			QStringList names;
			QStringList types;
			names << "data1" << "event1" << "data2" << "event2";
			types << "VuoText" << "" << "VuoLayer" << "";
			QTest::newRow("multiple event and data+event ports") << names << types << names;
		}
		{
			QStringList names;
			names << "refresh" << "a" << "aEvent" << "aOut" << "aOutEvent";
			QStringList inNames;
			inNames << "refresh2" << "a" << "aEvent" << "aOut" << "aOutEvent";

			QStringList specTypes;
			specTypes << "VuoReal" << "VuoSceneObject" << "" << "" << "";
			QTest::newRow("name collisions, no unspecialized generic ports") << names << specTypes << inNames;

			QStringList unspecTypes;
			unspecTypes << "VuoGenericType1" << "VuoGenericType2" << "" << "" << "";
			QTest::newRow("name collisions, some unspecialized generic ports") << names << unspecTypes << inNames;
		}
	}
	void testPublishedOutputNodeClass()
	{
		QFETCH(QStringList, publishedPortNames);
		QFETCH(QStringList, publishedPortTypes);
		QFETCH(QStringList, expectedInputPortNames);

		vector<VuoPublishedPort *> publishedPorts = makePublishedPorts(publishedPortNames, publishedPortTypes);

		VuoNodeClass *publishedNodeClass = VuoCompilerPublishedOutputNodeClass::newNodeClass(publishedPorts);
		VuoCompilerPublishedOutputNodeClass *publishedCompilerNodeClass = static_cast<VuoCompilerPublishedOutputNodeClass *>(publishedNodeClass->getCompiler());

		vector<VuoPortClass *> inputPortClasses = publishedNodeClass->getInputPortClasses();
		QCOMPARE(inputPortClasses.size(), publishedPorts.size() + 1 + VuoNodeClass::unreservedInputPortStartIndex);

		vector<VuoPortClass *> outputPortClasses = publishedNodeClass->getOutputPortClasses();
		QCOMPARE(outputPortClasses.size(), (size_t)VuoNodeClass::unreservedOutputPortStartIndex);

		for (size_t i = 0; i < publishedPorts.size(); ++i)
		{
			size_t inputPortIndex = i + VuoNodeClass::unreservedInputPortStartIndex;

			VuoPortClass *inputPortClass = inputPortClasses[inputPortIndex];

			QCOMPARE(QString::fromStdString(inputPortClass->getName()), expectedInputPortNames[i]);

			QCOMPARE((int)inputPortClass->getPortType(), (int)(publishedPortTypes[i].isEmpty() ? VuoPortClass::eventOnlyPort : VuoPortClass::dataAndEventPort));

			QCOMPARE(publishedCompilerNodeClass->getInputPortIndexForPublishedOutputPort(i), inputPortIndex);
		}
	}

};

QTEST_APPLESS_MAIN(TestVuoCompilerNodeClass)
#include "TestVuoCompilerNodeClass.moc"
