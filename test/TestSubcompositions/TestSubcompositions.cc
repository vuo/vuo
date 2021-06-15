/**
 * @file
 * TestSubcompositions interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>

#include "VuoRendererCommon.hh"

// Be able to use these types in QTest::addColumn()
typedef QList<VuoPortClass::EventBlocking> EventBlockingList;
Q_DECLARE_METATYPE(EventBlockingList);
typedef QList<bool> BoolList;
Q_DECLARE_METATYPE(BoolList);
typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap);
Q_DECLARE_METATYPE(VuoCompilerCompositionDiff *);


/**
 * Tests compiling, loading, and executing subcompositions.
 */
class TestSubcompositions : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;
	TestCompilerDelegate *compilerDelegate;

	/**
	 * Returns the path where the composition would be copied to in order to install it,
	 * and outputs the node class name it would have.
	 */
	string getInstalledCompositionPath(string compositionPath, string &nodeClassName)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);

		nodeClassName = file;
		nodeClassName[0] = tolower(nodeClassName[0]);
		nodeClassName = "vuo.test." + nodeClassName;

		string copiedCompositionFileName = nodeClassName + "." + ext;
		return VuoFileUtilities::getUserModulesPath() + "/" + copiedCompositionFileName;
	}

	/**
	 * Returns the path where the compiled composition would be cached.
	 */
	string getCachedCompiledCompositionPath(string nodeClassName)
	{
		string arch = VuoCompiler::getTargetArch(VuoCompiler::getProcessTarget());
		return VuoFileUtilities::getCachePath() + "/User/Modules/" + arch + "/" + nodeClassName + ".vuonode";
	}

	/**
	 * Copy the provided composition into the "User Modules" directory. Returns the node class name.
	 */
	string installSubcomposition(string compositionPath)
	{
		string nodeClassName;
		string installedCompositionPath = getInstalledCompositionPath(compositionPath, nodeClassName);

		VuoFileUtilities::copyFile(compositionPath, installedCompositionPath);

		return nodeClassName;
	}

	/**
	 * For each .vuo file in the compositions folder, delete the corresponding installed composition files.
	 */
	void uninstallSubcompositions()
	{
		QDir compositionDir = getCompositionDir();
		QStringList filter("*.vuo");
		QStringList compositionFileNames = compositionDir.entryList(filter);
		compositionFileNames.append("ParseError.vuo");
		foreach (QString compositionFileName, compositionFileNames)
		{
			string nodeClassName;
			string installedCompositionPath = getInstalledCompositionPath(compositionFileName.toStdString(), nodeClassName);

			VuoFileUtilities::deleteFile(installedCompositionPath);

			if (! compiler)
			{
				string cachedModulePath = getCachedCompiledCompositionPath(nodeClassName);
				VuoFileUtilities::deleteFile(cachedModulePath);
			}
		}
	}

private slots:

	void initTestCase()
	{
		uninstallSubcompositions();  // in case the test crashed last time and cleanup() wasn't called
	}

	void init()
	{
		compiler = initCompiler();
		compilerDelegate = new TestCompilerDelegate();
		compiler->setDelegate(compilerDelegate);
	}

	void cleanup()
	{
		delete compiler;
		delete compilerDelegate;
		VuoCompiler::reset();
		uninstallSubcompositions();
	}

	void testNodeInterface_data(void)
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QString >("expectedNodeClassName");
		QTest::addColumn< QString >("expectedDefaultTitle");
		QTest::addColumn< bool >("expectedIsStateful");
		QTest::addColumn< QStringList >("expectedInputPortNames");
		QTest::addColumn< QStringList >("expectedInputPortDisplayNames");
		QTest::addColumn< QStringList >("expectedInputPortTypes");
		QTest::addColumn< QStringList >("expectedOutputPortNames");
		QTest::addColumn< QStringList >("expectedOutputPortDisplayNames");
		QTest::addColumn< QStringList >("expectedOutputPortTypes");
		QTest::addColumn< EventBlockingList >("expectedEventBlocking");
		QTest::addColumn< BoolList >("expectedIsTrigger");

		QStringList inputNamesTemplate;
		QStringList inputTypesTemplate;
		EventBlockingList eventBlockingTemplate;
		inputNamesTemplate.append("refresh");
		inputTypesTemplate.append("event");
		eventBlockingTemplate.append(VuoPortClass::EventBlocking_None);

		{
			QTest::newRow("Empty composition") << "Empty" << "vuo.test.empty" << "Empty" << false << inputNamesTemplate << QStringList() << inputTypesTemplate << QStringList() << QStringList() << QStringList() << eventBlockingTemplate << BoolList();
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputDisplayNames;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("EventOnlyInput");
			inputDisplayNames.append("Event Only Input");
			inputTypes.append("event");
			eventBlocking.append(VuoPortClass::EventBlocking_None);
			inputNames.append("DataAndEventInput");
			inputDisplayNames.append("Data And Event Input");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputDisplayNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("EventOnlyOutput");
			outputDisplayNames.append("Event Only Output");
			outputTypes.append("event");
			isTrigger.append(false);
			outputNames.append("DataAndEventOutput");
			outputDisplayNames.append("Data And Event Output");
			outputTypes.append("VuoInteger");
			isTrigger.append(false);
			outputNames.append("EventOnlyTrigger");
			outputDisplayNames.append("Event Only Trigger");
			outputTypes.append("event");
			isTrigger.append(true);
			outputNames.append("DataAndEventTrigger");
			outputDisplayNames.append("Data And Event Trigger");
			outputTypes.append("VuoPoint2d");
			isTrigger.append(true);

			QTest::newRow("All kinds of ports") << "PortKinds" << "vuo.test.portKinds" << "Port Kinds" << true << inputNames << inputDisplayNames << inputTypes << outputNames << outputDisplayNames << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In");
			inputTypes.append("event");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out");
			outputTypes.append("VuoPoint2d");
			isTrigger.append(false);

			QTest::newRow("Trigger joining data flow to output") << "TriggerBlocked" << "vuo.test.triggerBlocked" << "Trigger Blocked" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("Value");
			inputTypes.append("VuoReal");
			eventBlocking.append(VuoPortClass::EventBlocking_Door);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("AllowedValue");
			outputTypes.append("VuoReal");
			isTrigger.append(false);

			QTest::newRow("Input event to door") << "AllowAlternating" << "vuo.test.allowAlternating" << "Allow Alternating" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In1");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_Wall);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out1");
			outputTypes.append("VuoInteger");
			isTrigger.append(false);

			QTest::newRow("Input event to wall") << "InputBlocked" << "vuo.test.inputBlocked" << "Input Blocked" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In1");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_Wall);
			inputNames.append("In2");
			inputTypes.append("event");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out1");
			outputTypes.append("VuoInteger");
			isTrigger.append(false);

			QTest::newRow("Input event to wall and output") << "InputBlockedAndNotBlocked" << "vuo.test.inputBlockedAndNotBlocked" << "Input Blocked And Not Blocked" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In1");
			inputTypes.append("VuoPoint3d");
			eventBlocking.append(VuoPortClass::EventBlocking_Wall);
			inputNames.append("In2");
			inputTypes.append("VuoBoolean");
			eventBlocking.append(VuoPortClass::EventBlocking_Door);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out1");
			outputTypes.append("VuoPoint3d");
			isTrigger.append(false);
			outputNames.append("Out2");
			outputTypes.append("VuoPoint3d");
			isTrigger.append(false);

			QTest::newRow("Input event to wall and door") << "InputBlockedAndDoored" << "vuo.test.inputBlockedAndDoored" << "Input Blocked And Doored" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("Value");
			inputTypes.append("VuoColor");
			eventBlocking.append(VuoPortClass::EventBlocking_Door);
			inputNames.append("Value2");
			inputTypes.append("VuoColor");
			eventBlocking.append(VuoPortClass::EventBlocking_Door);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("SameValue");
			outputTypes.append("VuoColor");
			isTrigger.append(false);
			outputNames.append("SameValue2");
			outputTypes.append("VuoColor");
			isTrigger.append(false);

			QTest::newRow("Input event to only some outputs") << "PassThru" << "vuo.test.passThru" << "Pass Thru" << false << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("Seconds");
			inputTypes.append("VuoReal");
			eventBlocking.append(VuoPortClass::EventBlocking_None);
			inputNames.append("N");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Fired");
			outputTypes.append("event");
			isTrigger.append(true);

			QTest::newRow("Only trigger output ports") << "FireNPeriodically" << "vuo.test.fireNPeriodically" << "Fire N Periodically" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("Width");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_None);
			inputNames.append("Height");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;

			QTest::newRow("No output ports") << "SendCheckerboard" << "vuo.test.sendCheckerboard" << "Send Checkerboard" << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In");
			inputTypes.append("event");
			eventBlocking.append(VuoPortClass::EventBlocking_Wall);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out");
			outputTypes.append("VuoText");
			isTrigger.append(false);

			QTest::newRow("Unconnected input port") << "PublishedInputAndOutput" << "vuo.test.publishedInputAndOutput" << "Published Input And Output" << false << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputDisplayNames;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("URL");
			inputDisplayNames.append("URL");
			inputTypes.append("VuoText");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputDisplayNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("IsHTTPS");  /// @todo https://b33p.net/kosada/node/11379
			outputDisplayNames.append("Is H T T P S");
			outputTypes.append("VuoBoolean");
			isTrigger.append(false);

			QTest::newRow("All-caps port name") << "IsHttps" << "vuo.test.isHttps" << "Is Https" << false << inputNames << inputDisplayNames << inputTypes << outputNames << outputDisplayNames << outputTypes << eventBlocking << isTrigger;
		}
	}
	void testNodeInterface(void)
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, expectedNodeClassName);
		QFETCH(QString, expectedDefaultTitle);
		QFETCH(bool, expectedIsStateful);
		QFETCH(QStringList, expectedInputPortNames);
		QFETCH(QStringList, expectedInputPortDisplayNames);
		QFETCH(QStringList, expectedInputPortTypes);
		QFETCH(QStringList, expectedOutputPortNames);
		QFETCH(QStringList, expectedOutputPortDisplayNames);
		QFETCH(QStringList, expectedOutputPortTypes);
		QFETCH(EventBlockingList, expectedEventBlocking);
		QFETCH(BoolList, expectedIsTrigger);

		string originalPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string nodeClassName;
		string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
		compilerDelegate->installModule(originalPath, installedPath);
		VuoCompilerNodeClass *subcomposition = compiler->getNodeClass(nodeClassName);

		QVERIFY(subcomposition != NULL);

		QCOMPARE(QString::fromStdString(subcomposition->getBase()->getClassName()), expectedNodeClassName);
		QCOMPARE(QString::fromStdString(subcomposition->getBase()->getDefaultTitle()), expectedDefaultTitle);
		QCOMPARE(subcomposition->isStateful(), expectedIsStateful);

		vector<VuoPortClass *> actualInputPortClasses = subcomposition->getBase()->getInputPortClasses();
		for (size_t i = 0; i < expectedInputPortNames.size(); ++i)
		{
			QVERIFY2(i < actualInputPortClasses.size(), QString("%1").arg(i).toUtf8().data());

			string actualName = actualInputPortClasses[i]->getName();
			QCOMPARE(QString::fromStdString(actualName), expectedInputPortNames[i]);

			if (i >= VuoNodeClass::unreservedInputPortStartIndex && ! expectedInputPortDisplayNames.empty())
			{
				QString expectedDisplayName = expectedInputPortDisplayNames[i - VuoNodeClass::unreservedInputPortStartIndex];
				string actualDisplayName = static_cast<VuoCompilerPortClass *>( actualInputPortClasses[i]->getCompiler() )->getDisplayName();
				QCOMPARE(QString::fromStdString(actualDisplayName), expectedDisplayName);
			}

			VuoType *actualType = static_cast<VuoCompilerPortClass *>(actualInputPortClasses[i]->getCompiler())->getDataVuoType();
			QCOMPARE(QString::fromStdString(actualType ? actualType->getModuleKey() : "event"), expectedInputPortTypes[i]);

			VuoPortClass::EventBlocking actualEventBlocking = actualInputPortClasses[i]->getEventBlocking();
			QCOMPARE((int)actualEventBlocking, (int)expectedEventBlocking[i]);
		}
		QCOMPARE(actualInputPortClasses.size(), (unsigned long)expectedInputPortNames.size());

		vector<VuoPortClass *> actualOutputPortClasses = subcomposition->getBase()->getOutputPortClasses();
		for (size_t i = 0; i < expectedOutputPortNames.size(); ++i)
		{
			QVERIFY2(i < actualOutputPortClasses.size(), QString("%1").arg(i).toUtf8().data());

			string actualName = actualOutputPortClasses[i]->getName();
			QCOMPARE(QString::fromStdString(actualName), expectedOutputPortNames[i]);

			if (i >= VuoNodeClass::unreservedOutputPortStartIndex && ! expectedOutputPortDisplayNames.empty())
			{
				QString expectedDisplayName = expectedOutputPortDisplayNames[i - VuoNodeClass::unreservedOutputPortStartIndex];
				string actualDisplayName = static_cast<VuoCompilerPortClass *>( actualOutputPortClasses[i]->getCompiler() )->getDisplayName();
				QCOMPARE(QString::fromStdString(actualDisplayName), expectedDisplayName);
			}

			VuoType *actualType = static_cast<VuoCompilerPortClass *>(actualOutputPortClasses[i]->getCompiler())->getDataVuoType();
			QCOMPARE(QString::fromStdString(actualType ? actualType->getModuleKey() : "event"), expectedOutputPortTypes[i]);

			bool actualIsTrigger = (actualOutputPortClasses[i]->getPortType() == VuoPortClass::triggerPort);
			QCOMPARE(actualIsTrigger, expectedIsTrigger[i]);
		}
		QCOMPARE(actualOutputPortClasses.size(), (unsigned long)expectedOutputPortNames.size());
	}

private:

	class TestSubcompositionsRunnerDelegate : public VuoRunnerDelegateAdapter
	{
	public:
		QStringList expectedOutputPortEvents[2];
		QStringMap outputPortData;
		QStringList outputPortEvents[2];
		bool gotAllExpectedEvents[2];
		string fireOnStartIdentifier;
		VuoRunner *runner;

		TestSubcompositionsRunnerDelegate(VuoRunner *runner, QStringList expectedOutputPortEvents[2], string fireOnStartIdentifier)
		{
			this->runner = runner;
			this->expectedOutputPortEvents[0] = expectedOutputPortEvents[0];
			this->expectedOutputPortEvents[1] = expectedOutputPortEvents[1];
			this->fireOnStartIdentifier = fireOnStartIdentifier;
			gotAllExpectedEvents[0] = false;
			gotAllExpectedEvents[1] = false;
		}

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			int index = (gotAllExpectedEvents[0] ? 1 : 0);

			if (sentData)
				outputPortData.insert( QString::fromStdString(portIdentifier), QString::fromStdString(dataSummary) );

			outputPortEvents[index].append( QString::fromStdString(portIdentifier) );

			bool gotAll = true;
			foreach (QString i, expectedOutputPortEvents[index])
			{
				if (! outputPortEvents[index].contains(i))
				{
					gotAll = false;
					break;
				}
			}
			if (gotAll)
			{
				gotAllExpectedEvents[index] = true;

				if (gotAllExpectedEvents[1])
					dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
									   runner->stop();
								   });
				else if (gotAllExpectedEvents[0])
					runner->fireTriggerPortEvent("", fireOnStartIdentifier);
			}
		}
	};

private slots:

	void testExecutingSubcomposition_data(void)
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QStringList >("subcompositionNames");
		QTest::addColumn< QString >("firingPortName");
		QTest::addColumn< QStringMap >("inputPortData");
		QTest::addColumn< QStringMap >("expectedOutputPortData");
		QTest::addColumn< QStringList >("expectedOutputPortEvents0");
		QTest::addColumn< QStringList >("expectedOutputPortEvents1");

		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Value"] = "2";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Sum"] = "3";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Sum");
			QTest::newRow("Data-and-event ports") << "AddOne" << subcompositionNames << "Value" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Sum"] = "1";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Sum");
			QTest::newRow("Default input port value") << "AddOne" << subcompositionNames << "Value" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Texts"] = "[\"first\",\"second\",\"third\"]";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["CompositeText"] = "<code>first second third</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("CompositeText");
			QTest::newRow("List input port") << "AppendWithSpaces" << subcompositionNames << "Texts" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["CompositeText"] = "<code>&#0;</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("CompositeText");
			QTest::newRow("Default list input port value") << "AppendWithSpaces" << subcompositionNames << "Texts" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Value"] = "4";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Sum"] = "0";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Sum");
			QTest::newRow("Event into refresh port") << "AddOne" << subcompositionNames << "refresh" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Es"] = "<code>ee</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Es");
			QTest::newRow("Event-only input port") << "LengthenEs" << subcompositionNames << "Lengthen" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents;
			QTest::newRow("No inputs or outputs") << "Empty" << subcompositionNames << "refresh" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents;
			QTest::newRow("No outputs") << "SendCheckerboard" << subcompositionNames << "refresh" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents;
			QTest::newRow("Wall on input port") << "CycleSeasons" << subcompositionNames << "GoToFirst" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents0;
			expectedOutputPortEvents0.append("Second");
			QStringList expectedOutputPortEvents1;
			expectedOutputPortEvents1.append("First");
			QTest::newRow("Door on input port, event reaches some outputs") << "Alternate" << subcompositionNames << "Alternate" << inputPortData << expectedOutputPortData << expectedOutputPortEvents0 << expectedOutputPortEvents1;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents0;
			expectedOutputPortEvents0.append("AllowedValue");
			QStringList expectedOutputPortEvents1;
			QTest::newRow("Door on input port, event sometimes reaches outputs") << "AllowAlternating" << subcompositionNames << "Value" << inputPortData << expectedOutputPortData << expectedOutputPortEvents0 << expectedOutputPortEvents1;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Count"] = "2";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Count");
			QTest::newRow("Event causes trigger to fire") << "SpinOffCount" << subcompositionNames << "Increment" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Count"] = "20";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Count");
			QTest::newRow("Event causes trigger with no outgoing cables to fire") << "SpinOffDisconnected" << subcompositionNames << "Increment" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Fired");
			QTest::newRow("Trigger fires on its own") << "FireNPeriodically" << subcompositionNames << "" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Seconds"] = "0.1";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["FiredText"] = "<code>my text</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("FiredText");
			QTest::newRow("Trigger uses default input port value") << "FireTextPeriodically" << subcompositionNames << "" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Text"] = "\"abc\"";
			inputPortData["Seconds"] = "0.1";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["FiredText"] = "<code>abc</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("FiredText");
			QTest::newRow("Trigger uses constant input port value") << "FireTextPeriodically" << subcompositionNames << "" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Texts"] = "[\"It's a test\", \"Whee\"]";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["ExclamationTexts"] = "List containing 2 items: <ul>\n<li><code>It's a test!</code></li>\n<li><code>Whee!</code></li></ul>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("ExclamationTexts");
			QTest::newRow("Trigger uses constant list input port value") << "AddExclamations" << subcompositionNames << "" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("First");
			QTest::newRow("No inputs") << "FireAlternating" << subcompositionNames << "" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			subcompositionNames.append("AppendWithSpaces");
			subcompositionNames.append("LengthenEs");
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["CompositeText"] = "<code>ee ee</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("CompositeText");
			QTest::newRow("Events transmitted through sub-subcomposition") << "LengthenEsAndDouble" << subcompositionNames << "Lengthen" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			subcompositionNames.append("SpinOffCount");
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Count"] = "4";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Count");
			QTest::newRow("Events fired from sub-subcomposition and subcomposition") << "SpinOffDoubleCount" << subcompositionNames << "IncrementTwice" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			subcompositionNames.append("AppendWithSpaces");
			subcompositionNames.append("LengthenEs");
			subcompositionNames.append("LengthenEsAndDouble");
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["CompositeText"] = "<code>ee eek</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("CompositeText");
			QTest::newRow("Events transmitted through sub-sub-subcomposition") << "LengthenEsAndDoubleAndAddK" << subcompositionNames << "Lengthen" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
	}
	void testExecutingSubcomposition(void)
	{
		QFETCH(QString, compositionName);
		QFETCH(QStringList, subcompositionNames);
		QFETCH(QString, firingPortName);
		QFETCH(QStringMap, inputPortData);
		QFETCH(QStringMap, expectedOutputPortData);
		QFETCH(QStringList, expectedOutputPortEvents0);
		QFETCH(QStringList, expectedOutputPortEvents1);

		VuoCompilerNodeClass *subcompositionNodeClass = NULL;
		QStringList compositionsToInstall = subcompositionNames;
		compositionsToInstall.append(compositionName);
		foreach (QString currentCompositionName, compositionsToInstall)
		{
			string originalPath = getCompositionPath(currentCompositionName.toStdString() + ".vuo");
			string nodeClassName;
			string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
			compilerDelegate->installModule(originalPath, installedPath);
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName);
			if (currentCompositionName == compositionName)
				subcompositionNodeClass = nodeClass;
		}

		// Create a top-level composition that consists of a `Fire on Start` node connected to the subcomposition.

		VuoNode *subcompositionNode = compiler->createNode(subcompositionNodeClass);
		VuoCompilerPort *firingPort = NULL;
		if (! firingPortName.isEmpty())
		{
			VuoPort *baseFiringPort = subcompositionNode->getInputPortWithName(firingPortName.toStdString());
			QVERIFY(baseFiringPort != NULL);
			firingPort = static_cast<VuoCompilerPort *>( baseFiringPort->getCompiler() );
		}

		VuoCompilerNodeClass *fireOnStartNodeClass = compiler->getNodeClass("vuo.event.fireOnStart");
		VuoNode *fireOnStartNode = compiler->createNode(fireOnStartNodeClass);
		VuoCompilerPort *fireOnStartPort = static_cast<VuoCompilerPort *>( fireOnStartNode->getOutputPortWithName("started")->getCompiler() );

		VuoCompilerComposition *topLevelComposition = new VuoCompilerComposition(new VuoComposition(), NULL);
		topLevelComposition->getBase()->addNode( fireOnStartNode );
		topLevelComposition->getBase()->addNode( subcompositionNode );

		if (! firingPortName.isEmpty())
		{
			VuoCompilerCable *startToSubcomposition = new VuoCompilerCable(fireOnStartNode->getCompiler(), fireOnStartPort, subcompositionNode->getCompiler(), firingPort);
			topLevelComposition->getBase()->addCable( startToSubcomposition->getBase() );
		}

		// Set constant input port values on the subcomposition.

		foreach (QString key, inputPortData.keys())
		{
			VuoPort *basePort = subcompositionNode->getInputPortWithName( key.toStdString() );
			VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>( basePort->getCompiler() );
			VuoCompilerInputData *data = eventPort->getData();
			data->setInitialValue( inputPortData.value(key).toStdString() );
		}

		// Build and run the top-level composition.

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("TopLevelComposition", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("TopLevelComposition-linked", "");
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(topLevelComposition, compiledCompositionPath, true, issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		remove(compiledCompositionPath.c_str());
		delete issues;
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, "", false, true);

		QStringList expectedOutputPortEvents[2] = { expectedOutputPortEvents0, expectedOutputPortEvents1 };
		QStringList expectedOutputPortEventsRenamed[2];
		string fireOnStartIdentifier = fireOnStartPort->getIdentifier();
		for (int i = 0; i < 2; ++i)
		{
			foreach (QString j, expectedOutputPortEvents[i])
			{
				VuoPort *port = subcompositionNode->getOutputPortWithName( j.toStdString() );
				string portIdentifier = static_cast<VuoCompilerPort *>( port->getCompiler() )->getIdentifier();
				expectedOutputPortEventsRenamed[i].append( QString::fromStdString(portIdentifier) );
			}
			expectedOutputPortEventsRenamed[i].append( QString::fromStdString(fireOnStartIdentifier) );
		}

		TestSubcompositionsRunnerDelegate delegate(runner, expectedOutputPortEventsRenamed, fireOnStartIdentifier);
		runner->setDelegate(&delegate);
		runner->startPaused();
		runner->subscribeToAllTelemetry("");
		runner->unpause();
		runner->waitUntilStopped();
		delete runner;

		// Check the output port values on the subcomposition.

		foreach (QString key, expectedOutputPortData.keys())
		{
			VuoPort *port = subcompositionNode->getOutputPortWithName( key.toStdString() );
			string portIdentifier = static_cast<VuoCompilerPort *>( port->getCompiler() )->getIdentifier();
			QStringMap::iterator portDataIter = delegate.outputPortData.find( QString::fromStdString(portIdentifier) );
			QVERIFY(portDataIter != delegate.outputPortData.end());
			QCOMPARE(portDataIter.value(), expectedOutputPortData[key]);
		}
	}

	void testMultipleSubcompositions()
	{
		string subcompositionNames[] = { "SpinOffCount", "AddOne" };
		for (int i = 0; i < 2; ++i)
		{
			string originalPath = getCompositionPath(subcompositionNames[i] + ".vuo");
			string nodeClassName;
			string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
			compilerDelegate->installModule(originalPath, installedPath);
		}

		string compositionName = "MultipleSubcompositions";
		string compositionPath = getCompositionPath(compositionName + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition *topLevelComposition = new VuoCompilerComposition(new VuoComposition(), parser);
		delete parser;

		// Build and run the top-level composition.

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionName, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(compositionName + "-linked", "");
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(topLevelComposition, compiledCompositionPath, true, issues);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		delete issues;
		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, "", false, true);

		map<string, VuoNode *> nodeForIdentifier;
		set<VuoNode *> nodes = topLevelComposition->getBase()->getNodes();
		for (set<VuoNode *>::iterator j = nodes.begin(); j != nodes.end(); ++j)
			nodeForIdentifier[(*j)->getCompiler()->getGraphvizIdentifier()] = *j;

		VuoPort *fireOnStartPort = nodeForIdentifier["FireOnStart"]->getOutputPortWithName("started");
		string fireOnStartIdentifier = static_cast<VuoCompilerPort *>(fireOnStartPort->getCompiler())->getIdentifier();
		VuoPort *addOne1Port = nodeForIdentifier["AddOne1"]->getOutputPortWithName("Sum");
		string addOne1Identifier = static_cast<VuoCompilerPort *>(addOne1Port->getCompiler())->getIdentifier();
		VuoPort *addOne2Port = nodeForIdentifier["AddOne2"]->getOutputPortWithName("Sum");
		string addOne2Identifier = static_cast<VuoCompilerPort *>(addOne2Port->getCompiler())->getIdentifier();
		VuoPort *spinOffCount1Port = nodeForIdentifier["SpinOffCount1"]->getOutputPortWithName("Count");
		string spinOffCount1Identifier = static_cast<VuoCompilerPort *>(spinOffCount1Port->getCompiler())->getIdentifier();
		VuoPort *spinOffCount2Port = nodeForIdentifier["SpinOffCount2"]->getOutputPortWithName("Count");
		string spinOffCount2Identifier = static_cast<VuoCompilerPort *>(spinOffCount2Port->getCompiler())->getIdentifier();

		QStringList expectedOutputPortEvents[2];
		expectedOutputPortEvents[0].append( QString::fromStdString(addOne1Identifier) );
		expectedOutputPortEvents[0].append( QString::fromStdString(addOne2Identifier) );
		expectedOutputPortEvents[1].append( QString::fromStdString(addOne1Identifier) );

		TestSubcompositionsRunnerDelegate delegate(runner, expectedOutputPortEvents, fireOnStartIdentifier);
		runner->setDelegate(&delegate);
		runner->startPaused();
		runner->subscribeToAllTelemetry("");
		runner->unpause();
		runner->waitUntilStopped();
		delete runner;

		// Check the output port values.

		map<string, string> expectedOutputPortData;
		expectedOutputPortData[spinOffCount1Identifier] = "2";
		expectedOutputPortData[spinOffCount2Identifier] = "1";
		expectedOutputPortData[addOne1Identifier] = "3";
		expectedOutputPortData[addOne2Identifier] = "2";

		for (map<string, string>::iterator i = expectedOutputPortData.begin(); i != expectedOutputPortData.end(); ++i)
		{
			QStringMap::iterator portDataIter = delegate.outputPortData.find( QString::fromStdString(i->first) );
			QVERIFY(portDataIter != delegate.outputPortData.end());
			QCOMPARE(portDataIter.value(), QString::fromStdString(i->second));
		}

		delete topLevelComposition;
	}

private:

	class TestNodeExecutionRunnerDelegate : public VuoRunnerDelegateAdapter
	{
	public:
		QStringList started;
		QStringList finished;
		VuoRunner *runner;

		TestNodeExecutionRunnerDelegate(VuoRunner *runner)
		{
			this->runner = runner;
		}

		void receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier)
		{
			started.append(QString::fromStdString(compositionIdentifier + "/" + nodeIdentifier));
		}

		void receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier)
		{
			finished.append(QString::fromStdString(compositionIdentifier + "/" + nodeIdentifier));

			if (started.size() == finished.size())
			{
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	void testNodeExecutionTelemetry()
	{
		// Install the subcompositions.

		QStringList subcompositionNames;
		subcompositionNames << "AddOne" << "AddOneAndDouble";
		for (QString subcompositionName : subcompositionNames)
		{
			string originalPath = getCompositionPath(subcompositionName.toStdString() + ".vuo");
			string nodeClassName;
			string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
			compilerDelegate->installModule(originalPath, installedPath);
		}

		// Build and run the top-level composition.

		string topLevelCompositionPath = getCompositionPath("AddOneAndDoubleOnStart.vuo");
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(topLevelCompositionPath, issues);
		delete issues;

		TestNodeExecutionRunnerDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->startPaused();
		runner->subscribeToEventTelemetry("Top");
		runner->subscribeToEventTelemetry("Top/AddOneAndDouble");
		runner->subscribeToEventTelemetry("Top/AddOneAndDouble/AddOne");
		runner->unpause();
		runner->waitUntilStopped();
		delete runner;

		QStringList expectedStarted;
		expectedStarted << "Top/AddOneAndDouble"
						<< "Top/AddOneAndDouble/AddOne"
						<< "Top/AddOneAndDouble/AddOne/MakeList"
						<< "Top/AddOneAndDouble/AddOne/Add"
						<< "Top/AddOneAndDouble/MakeList"
						<< "Top/AddOneAndDouble/Multiply";

		QStringList expectedFinished;
		expectedFinished << "Top/AddOneAndDouble/AddOne/MakeList"
						 << "Top/AddOneAndDouble/AddOne/Add"
						 << "Top/AddOneAndDouble/AddOne"
						 << "Top/AddOneAndDouble/MakeList"
						 << "Top/AddOneAndDouble/Multiply"
						 << "Top/AddOneAndDouble";

		QCOMPARE(delegate.started, expectedStarted);
		QCOMPARE(delegate.finished, expectedFinished);
	}

	void testLiveCodingInTopLevelComposition_data()
	{
		QTest::addColumn<int>("testNum");

		int testNum = 0;
		QTest::newRow("stateful subcomposition node added") << testNum++;
		QTest::newRow("trigger subcomposition node added") << testNum++;
		QTest::newRow("multi-level subcomposition node added") << testNum++;
		QTest::newRow("subcomposition nodes added and removed") << testNum++;
	}
	void testLiveCodingInTopLevelComposition()
	{
		QFETCH(int, testNum);

		string subcompositionNames[] = { "AppendWithSpaces", "AppendWord", "FireWordOnStart",
										 "LengthenEs", "LengthenEsAndDouble", "LengthenEsAndDoubleAndAddK" };
		for (int i = 0; i < 6; ++i)
		{
			string originalPath = getCompositionPath(subcompositionNames[i] + ".vuo");
			string nodeClassName;
			string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
			compilerDelegate->installModule(originalPath, installedPath);
		}

		string compositionPath = getCompositionPath("PublishedInputAndOutput.vuo");

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();

		VuoCompilerComposition *composition = nullptr;
		VuoRunner *runner = nullptr;
		VuoRunner::Port *inPort = nullptr;
		VuoRunner::Port *outPort = nullptr;
		VuoCompilerIssues *issues = new VuoCompilerIssues();

		{
			// Build and run the original composition.

			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath,
																		   runningCompositionLibraries, compositionDir, false, true);
			runner->start();

			inPort = runner->getPublishedInputPortWithName("In");
			outPort = runner->getPublishedOutputPortWithName("Out");
		}

		VuoPort *basePublishedInputPort = composition->getBase()->getPublishedInputPorts().front();
		VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedInputPort->getCompiler() );

		VuoPort *basePublishedOutputPort = composition->getBase()->getPublishedOutputPorts().front();
		VuoCompilerPublishedPort *publishedOutputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedOutputPort->getCompiler() );

		if (testNum == 0)  // stateful subcomposition node added
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add a stateful subcomposition node.
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.lengthenEs");
			VuoNode *node = compiler->createNode(nodeClass);
			VuoCompilerPort *lengthenPort = static_cast<VuoCompilerPort *>( node->getInputPortWithName("Lengthen")->getCompiler() );
			VuoCompilerPort *esPort = static_cast<VuoCompilerPort *>( node->getOutputPortWithName("Es")->getCompiler() );
			composition->getBase()->addNode(node);

			// Connect a published input cable to the node.
			VuoCompilerCable *inputCable = new VuoCompilerCable(NULL, publishedInputPort, node->getCompiler(), lengthenPort);
			composition->getBase()->addCable(inputCable->getBase());

			// Connect a published output cable to the node.
			VuoCompilerCable *outputCable = new VuoCompilerCable(node->getCompiler(), esPort, NULL, publishedOutputPort);
			composition->getBase()->addCable(outputCable->getBase());

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("e"));
		}

		else if (testNum == 1)  // trigger subcomposition node added
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add a subcomposition node with a trigger port.
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.fireWordOnStart");
			VuoNode *node = compiler->createNode(nodeClass);
			VuoCompilerPort *wordPort = static_cast<VuoCompilerPort *>( node->getOutputPortWithName("Word")->getCompiler() );
			composition->getBase()->addNode(node);

			// Connect a published output cable to the node.
			VuoCompilerCable *outputCable = new VuoCompilerCable(node->getCompiler(), wordPort, NULL, publishedOutputPort);
			composition->getBase()->addCable(outputCable->getBase());

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("word"));
		}

		else if (testNum == 2)  // multi-level subcomposition node added
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add a subcomposition node with other subcompositions inside it.
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.lengthenEsAndDoubleAndAddK");
			VuoNode *node = compiler->createNode(nodeClass);
			VuoCompilerPort *lengthenPort = static_cast<VuoCompilerPort *>( node->getInputPortWithName("Lengthen")->getCompiler() );
			VuoCompilerPort *compositeTextPort = static_cast<VuoCompilerPort *>( node->getOutputPortWithName("CompositeText")->getCompiler() );
			composition->getBase()->addNode(node);

			// Connect a published input cable to the node.
			VuoCompilerCable *inputCable = new VuoCompilerCable(NULL, publishedInputPort, node->getCompiler(), lengthenPort);
			composition->getBase()->addCable(inputCable->getBase());

			// Connect a published output cable to the node.
			VuoCompilerCable *outputCable = new VuoCompilerCable(node->getCompiler(), compositeTextPort, NULL, publishedOutputPort);
			composition->getBase()->addCable(outputCable->getBase());

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("e ek"));
		}

		else if (testNum == 3)  // subcomposition nodes added and removed
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add two instances of a subcomposition node.
			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.appendWord");
			QVERIFY(nodeClass);
			VuoNode *node1 = compiler->createNode(nodeClass);
			VuoNode *node2 = compiler->createNode(nodeClass);
			VuoCompilerPort *textPort1 = static_cast<VuoCompilerPort *>( node1->getInputPortWithName("Text")->getCompiler() );
			VuoCompilerPort *textPort2 = static_cast<VuoCompilerPort *>( node2->getInputPortWithName("Text")->getCompiler() );
			VuoCompilerPort *compositeTextPort1 = static_cast<VuoCompilerPort *>( node1->getOutputPortWithName("CompositeText")->getCompiler() );
			VuoCompilerPort *compositeTextPort2 = static_cast<VuoCompilerPort *>( node2->getOutputPortWithName("CompositeText")->getCompiler() );
			composition->getBase()->addNode(node1);
			composition->getBase()->addNode(node2);
			composition->setUniqueGraphvizIdentifierForNode(node2);

			// Connect a cable between the two nodes.
			VuoCompilerCable *internalCable = new VuoCompilerCable(node1->getCompiler(), compositeTextPort1, node2->getCompiler(), textPort2);
			composition->getBase()->addCable(internalCable->getBase());

			// Connect a published input cable to the first node.
			VuoCompilerCable *inputCable = new VuoCompilerCable(NULL, publishedInputPort, node1->getCompiler(), textPort1);
			composition->getBase()->addCable(inputCable->getBase());

			// Connect a published output cable to the second node.
			VuoCompilerCable *outputCable = new VuoCompilerCable(node2->getCompiler(), compositeTextPort2, NULL, publishedOutputPort);
			composition->getBase()->addCable(outputCable->getBase());

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("blahwordword"));

			oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Remove the second node and its connected cables.
			composition->getBase()->removeNode(node2);
			composition->getBase()->removeCable(internalCable->getBase());
			composition->getBase()->removeCable(outputCable->getBase());

			// Connect a published output cable to the first node.
			VuoCompilerCable *otherOutputCable = new VuoCompilerCable(node1->getCompiler(), compositeTextPort1, NULL, publishedOutputPort);
			composition->getBase()->addCable(otherOutputCable->getBase());

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			diffInfo = new VuoCompilerCompositionDiff();
			compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("blahword"));
		}

		runner->stop();

		delete runner;
		delete composition;
		delete issues;
	}

	void testLiveCodingInSubcomposition_data()
	{
		QTest::addColumn<QString>("subcompositionAtTopLevel");
		QTest::addColumn<QString>("originalSubcomposition");
		QTest::addColumn<QString>("modifiedSubcomposition");
		QTest::addColumn<QString>("otherSubcomposition");
		QTest::addColumn<QString>("firstOutput");
		QTest::addColumn<QString>("secondOutput");

		QTest::newRow("cable added to subcomposition") << "AppendWord.vuo" << "AppendWord.vuo" << "AppendSelf.vuo" << "" << "blahword" << "blahblah";
		QTest::newRow("stateless node added to subcomposition") << "LengthenEs.vuo" << "LengthenEs.vuo" << "LengthenBeeps.vuo" << "" << "e" << "beep";
		QTest::newRow("stateless node removed from subcomposition") << "LengthenBeeps.vuo" << "LengthenBeeps.vuo" << "LengthenEs.vuo" << "" << "bep" << "ee";
		QTest::newRow("stateful node added to subcomposition") << "LengthenEs.vuo" << "LengthenEs.vuo" << "LengthenEsAndCount.vuo" << "" << "e" << "ee1";
		QTest::newRow("stateful node removed from subcomposition") << "LengthenEsAndCount.vuo" << "LengthenEsAndCount.vuo" << "LengthenEs.vuo" << "" << "e1" << "ee";

		QTest::newRow("node added to subcomposition within subcomposition") << "LengthenEsAndDouble.vuo" << "LengthenEs.vuo" << "LengthenBeeps.vuo" << "AppendWithSpaces.vuo" << "e e" << "beep beep";

		QTest::newRow("change to subcomposition with trigger port") << "AppendWordTrigger.vuo" << "AppendWordTrigger.vuo" << "AppendSelfTrigger.vuo" << "" << "blahword" << "blahblah";

		QTest::newRow("drawer expanded in subcomposition") << "AppendWordStateful.vuo" << "AppendWordStateful.vuo" << "AppendWordsStateful.vuo" << "" << "blahword" << "blahwords";
		QTest::newRow("node with data-only transmission added to subcomposition") << "LengthenEs.vuo" << "LengthenEs.vuo" << "LengthenWhees.vuo" << "" << "e" << "whee!";

		// The second output should actually be "ef", but editing constants outside of Vuo Editor is not supported.
		QTest::newRow("constant modified in subcomposition") << "LengthenEs.vuo" << "LengthenEs.vuo" << "LengthenFs.vuo" << "" << "e" << "ee";
	}
	void testLiveCodingInSubcomposition()
	{
		QFETCH(QString, subcompositionAtTopLevel);  // An instance of this node class is added to the top-level composition.
		QFETCH(QString, originalSubcomposition);  // This node class gets replaced with a new implementation. May be the same as subcompositionAtTopLevel or contained within it.
		QFETCH(QString, modifiedSubcomposition);  // The new implementation of the node class being replaced.
		QFETCH(QString, otherSubcomposition);  // Additional subcomposition that needs to be installed in order to compile subcompositionAtTopLevel.
		QFETCH(QString, firstOutput);
		QFETCH(QString, secondOutput);

		string nodeClassName;
		string installedPath;
		{
			string subcompositionPath = getCompositionPath(originalSubcomposition.toStdString());
			installedPath = getInstalledCompositionPath(subcompositionPath, nodeClassName);
			compilerDelegate->installModule(subcompositionPath, installedPath);
		}

		if (! otherSubcomposition.isEmpty())
		{
			string otherNodeClassName;
			string subcompositionPath = getCompositionPath(otherSubcomposition.toStdString());
			string otherInstalledPath = getInstalledCompositionPath(subcompositionPath, otherNodeClassName);
			compilerDelegate->installModule(subcompositionPath, otherInstalledPath);
		}

		string topNodeClassName;
		string topInstalledPath;
		if (subcompositionAtTopLevel == originalSubcomposition)
		{
			topNodeClassName = nodeClassName;
			topInstalledPath = installedPath;
		}
		else
		{
			string subcompositionPath = getCompositionPath(subcompositionAtTopLevel.toStdString());
			topInstalledPath = getInstalledCompositionPath(subcompositionPath, topNodeClassName);
			compilerDelegate->installModule(subcompositionPath, topInstalledPath);
		}

		string compositionPath = getCompositionPath("PublishedInputAndOutput.vuo");

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();

		VuoCompilerComposition *composition = nullptr;
		VuoRunner *runner = nullptr;
		VuoRunner::Port *inPort = nullptr;
		VuoRunner::Port *outPort = nullptr;
		string oldCompositionGraphviz;
		VuoCompilerIssues *issues = new VuoCompilerIssues();

		// Build and run the original composition.
		{
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath,
																		   runningCompositionLibraries, compositionDir, false, true);
			runner->start();

			oldCompositionGraphviz = composition->getGraphvizDeclaration();

			inPort = runner->getPublishedInputPortWithName("In");
			outPort = runner->getPublishedOutputPortWithName("Out");
		}

		VuoPort *basePublishedInputPort = composition->getBase()->getPublishedInputPorts().front();
		VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedInputPort->getCompiler() );

		VuoPort *basePublishedOutputPort = composition->getBase()->getPublishedOutputPorts().front();
		VuoCompilerPublishedPort *publishedOutputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedOutputPort->getCompiler() );

		// Add a subcomposition node.
		VuoCompilerNodeClass *topNodeClass = compiler->getNodeClass(topNodeClassName);
		QVERIFY2(topNodeClass, topNodeClassName.c_str());
		VuoNode *node = compiler->createNode(topNodeClass);
		VuoCompilerPort *inputPort = static_cast<VuoCompilerPort *>( node->getInputPorts().at(VuoNodeClass::unreservedInputPortStartIndex)->getCompiler() );
		VuoCompilerPort *outputPort = static_cast<VuoCompilerPort *>( node->getOutputPorts().at(VuoNodeClass::unreservedOutputPortStartIndex)->getCompiler() );
		composition->getBase()->addNode(node);

		// Connect a published input cable to the node.
		VuoCompilerCable *inputCable = new VuoCompilerCable(NULL, publishedInputPort, node->getCompiler(), inputPort);
		composition->getBase()->addCable(inputCable->getBase());

		// Connect a published output cable to the node.
		VuoCompilerCable *outputCable = new VuoCompilerCable(node->getCompiler(), outputPort, NULL, publishedOutputPort);
		composition->getBase()->addCable(outputCable->getBase());

		// Build the modified composition and replace the running composition with it.
		{
			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);
			runner->replaceComposition(dylibPath, compositionDiff);
			delete diffInfo;

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), firstOutput);

			oldCompositionGraphviz = composition->getGraphvizDeclaration();
		}

		// Simulate editing the subcomposition.
		compilerDelegate->installModule(getCompositionPath(modifiedSubcomposition.toStdString()), installedPath, topNodeClassName);
		VuoCompilerCompositionDiff *diffInfo = compilerDelegate->takeDiffInfo();
		VuoCompilerComposition *composition2 = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(oldCompositionGraphviz, compiler);

		// Build the modified composition and replace the running composition with it.
		{
			runningCompositionLibraries->enqueueLibraryContainingDependencyToUnload(nodeClassName);

			compiler->compileComposition(composition2, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition2, compiler);
			delete diffInfo;

			runner->replaceComposition(dylibPath, compositionDiff);

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), secondOutput);
		}

		runner->stop();

		delete runner;
		delete composition;
		delete composition2;
		delete issues;
	}

private:

	class TestLiveCodingDelegate : public TestCompilerDelegate
	{
	private:
		VuoCompilerComposition *composition;

	public:
		TestLiveCodingDelegate(void)
		{
			composition = nullptr;
		}

		// suppress "hides overloaded virtual function" warning
		void installModule(const string &originalPath, const string &installedPath, const string &moduleToWaitOn)
		{
			TestCompilerDelegate::installModule(originalPath, installedPath, moduleToWaitOn);
		}

		void installModule(const string &originalPath, const string &installedPath, VuoCompilerComposition *composition)
		{
			this->composition = composition;
			TestCompilerDelegate::installModule(originalPath, installedPath);
		}

		void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
						   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
						   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
		{
			if (composition)
			{
				foreach (VuoNode *node, composition->getBase()->getNodes())
				{
					for (auto m : modulesModified)
					{
						if (m.second.first->getPseudoBase() == node->getNodeClass())
						{
							VuoCompilerNodeClass *newNodeClass = static_cast<VuoCompilerNodeClass *>(m.second.second);
							VuoNode *newNode = newNodeClass->newNode(node);
							composition->replaceNode(node, newNode);
							delete node;
						}
					}
				}

				composition = nullptr;
			}

			TestCompilerDelegate::loadedModules(modulesAdded, modulesModified, modulesRemoved, issues);
		}
	};

private slots:

	void testLiveCodingSubcompositionInterface_data()
	{
		QTest::addColumn<int>("testNum");

		int testNum = 0;
		QTest::newRow("published ports added") << testNum++;
		QTest::newRow("published ports removed") << testNum++;
		QTest::newRow("published ports reordered") << testNum++;
		QTest::newRow("published port data types changed") << testNum++;
	}
	void testLiveCodingSubcompositionInterface()
	{
		QFETCH(int, testNum);

		TestLiveCodingDelegate delegate;
		compiler->setDelegate(&delegate);

		string nodeClassName;
		string installedPath;
		{
			string subcompositionPath = getCompositionPath("AddThings.vuo");
			installedPath = getInstalledCompositionPath(subcompositionPath, nodeClassName);
			delegate.installModule(subcompositionPath, installedPath, nullptr);
		}

		string compositionPath = getCompositionPath("AddThingsTest.vuo");

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
		VuoCompilerCompositionDiff *diffInfo = nullptr;

		VuoCompilerComposition *composition = nullptr;
		VuoRunner *runner = nullptr;
		VuoRunner::Port *inPort = nullptr;
		VuoRunner::Port *outPort = nullptr;
		string oldCompositionGraphviz;
		VuoCompilerIssues *issues = new VuoCompilerIssues();

		// Build and run the original composition.
		{
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath,
																		   runningCompositionLibraries, compositionDir, false, true);
			runner->start();

			inPort = runner->getPublishedInputPortWithName("In");
			outPort = runner->getPublishedOutputPortWithName("Out");
			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("5 5  "));
		}

		// Install the new version of the subcomposition.
		{
			string modifiedComposition;
			if (testNum == 0)
				modifiedComposition = "AddThings-added.vuo";
			else if (testNum == 1)
				modifiedComposition = "AddThings-removed.vuo";
			else if (testNum == 2)
				modifiedComposition = "AddThings-reordered.vuo";
			else if (testNum == 3)
				modifiedComposition = "AddThings-typesChanged.vuo";

			delegate.installModule(getCompositionPath(modifiedComposition), installedPath, composition);
			diffInfo = delegate.takeDiffInfo();

			oldCompositionGraphviz = composition->getGraphvizDeclaration();
		}

		VuoNode *subcompositionNode = NULL;
		VuoNode *summarizeValueNode = NULL;
		VuoNode *formatTableNode = NULL;
		VuoNode *convertIntegerRealNode = NULL;
		VuoNode *formatNumberNode = NULL;
		foreach (VuoNode *node, composition->getBase()->getNodes())
		{
			string identifier = node->getCompiler()->getGraphvizIdentifier();
			if (identifier == "AddThings")
				subcompositionNode = node;
			else if (identifier == "SummarizeValue4")
				summarizeValueNode = node;
			else if (identifier == "FormatTable")
				formatTableNode = node;
			else if (identifier == "ConvertIntegerToRealNumber")
				convertIntegerRealNode = node;
			else if (identifier == "FormatNumber")
				formatNumberNode = node;
		}

		if (testNum == 0)
		{
			// Connect cables to the added ports on the subcomposition node.
			{
				VuoPort *baseAddedInput = subcompositionNode->getInputPortWithName("AddColumn");
				VuoCompilerPort *addedInput = static_cast<VuoCompilerPort *>(baseAddedInput->getCompiler());

				VuoPort *baseAddedOutput = subcompositionNode->getOutputPortWithName("ModifiedTable");
				VuoCompilerPort *addedOutput = static_cast<VuoCompilerPort *>(baseAddedOutput->getCompiler());

				VuoPort *baseSummarizeValueOutput = summarizeValueNode->getOutputPortWithName("summary");
				VuoCompilerPort *summarizeValueOutput = static_cast<VuoCompilerPort *>(baseSummarizeValueOutput->getCompiler());

				VuoPort *baseFormatTableInput = formatTableNode->getInputPortWithName("table");
				VuoCompilerPort *formatTableInput = static_cast<VuoCompilerPort *>(baseFormatTableInput->getCompiler());

				VuoCompilerCable *inputCable = new VuoCompilerCable(summarizeValueNode->getCompiler(), summarizeValueOutput, subcompositionNode->getCompiler(), addedInput);
				composition->getBase()->addCable(inputCable->getBase());

				VuoCompilerCable *outputCable = new VuoCompilerCable(subcompositionNode->getCompiler(), addedOutput, formatTableNode->getCompiler(), formatTableInput);
				composition->getBase()->addCable(outputCable->getBase());
			}
		}
		else if (testNum == 1)
		{
			// The cables are removed in the process of installing the new version of the subcomposition.
		}
		else if (testNum == 2)
		{
			// The cables stay the same.
		}
		else if (testNum == 3)
		{
			// Connect cables to the ports whose types have changed on the subcomposition node.
			{
				VuoPort *baseChangedInput = subcompositionNode->getInputPortWithName("Increment");
				VuoCompilerPort *changedInput = static_cast<VuoCompilerPort *>(baseChangedInput->getCompiler());

				VuoPort *baseChangedOutput = subcompositionNode->getOutputPortWithName("Count");
				VuoCompilerPort *changedOutput = static_cast<VuoCompilerPort *>(baseChangedOutput->getCompiler());

				VuoPort *baseConvertIntegerRealOutput = convertIntegerRealNode->getOutputPortWithName("real");
				VuoCompilerPort *convertIntegerRealOutput = static_cast<VuoCompilerPort *>(baseConvertIntegerRealOutput->getCompiler());

				VuoPort *baseFormatNumberInput = formatNumberNode->getInputPortWithName("value");
				VuoCompilerPort *formatNumberInput = static_cast<VuoCompilerPort *>(baseFormatNumberInput->getCompiler());

				VuoCompilerCable *inputCable = new VuoCompilerCable(convertIntegerRealNode->getCompiler(), convertIntegerRealOutput, subcompositionNode->getCompiler(), changedInput);
				composition->getBase()->addCable(inputCable->getBase());

				VuoCompilerCable *outputCable = new VuoCompilerCable(subcompositionNode->getCompiler(), changedOutput, formatNumberNode->getCompiler(), formatNumberInput);
				composition->getBase()->addCable(outputCable->getBase());
			}
		}

		// Build the modified composition and replace the running composition with it.
		{
			runningCompositionLibraries->enqueueLibraryContainingDependencyToUnload(nodeClassName);

			compiler->compileComposition(composition, bcPath, true, issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			string compositionDiff = diffInfo->diff(oldCompositionGraphviz, composition, compiler);

			runner->replaceComposition(dylibPath, compositionDiff);

			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();

			QString expectedOutput;
			if (testNum == 0)
				expectedOutput = "10 55 \"5\" ";
			else if (testNum == 1)
				expectedOutput = "5 55  ";
			else if (testNum == 2)
				expectedOutput = "10 55  ";
			else if (testNum == 3)
				expectedOutput = "5 55  5.0";

			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), expectedOutput);
		}

		runner->stop();

		delete runner;
		delete composition;
		delete issues;
		delete diffInfo;
	}

	void testInstallingWithoutCrashing_data()
	{
		QTest::addColumn< QString >("compositionName");

		QTest::newRow("Specialized generic port") << "SpecializedGenericPort";
		QTest::newRow("Unspecialized generic port") << "UnspecializedGenericPort";
		QTest::newRow("Specialized generic list port") << "SpecializedGenericListPort";
		QTest::newRow("Unspecialized generic list port") << "UnspecializedGenericListPort";
	}
	void testInstallingWithoutCrashing()
	{
		QFETCH(QString, compositionName);

		string originalPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string nodeClassName;
		string installedPath = getInstalledCompositionPath(originalPath, nodeClassName);
		compilerDelegate->installModule(originalPath, installedPath);
	}

	void testLoadingInstalledSubcompositions()
	{
		string nodeClassName = "vuo.test.alternate";
		string origCompositionPath = getCompositionPath("Alternate.vuo");
		string copiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
		string compiledCompositionPath = getCachedCompiledCompositionPath(nodeClassName);

		// The subcomposition source file is in the Modules folder when the compiler is initialized.
		// When all node classes are loaded, the subcomposition gets compiled.
		delete compiler;
		VuoCompiler::reset();
		VuoFileUtilities::copyFile(origCompositionPath, copiedCompositionPath);
		compiler = initCompiler();
		QVERIFY(! VuoFileUtilities::fileExists(compiledCompositionPath));
		compiler->getNodeClasses();
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));

		// The subcomposition source file is newer than the compiled file when the compiler is initialized.
		// When all node classes are loaded, the subcomposition gets recompiled.
		unsigned long lastModified1 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		sleep(2);
		delete compiler;
		VuoCompiler::reset();
		string origCompositionContents = VuoFileUtilities::readFileToString(origCompositionPath);
		VuoFileUtilities::writeStringToFile(origCompositionContents + "/* modified */", copiedCompositionPath);
		compiler = initCompiler();
		compiler->getNodeClasses();
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
		unsigned long lastModified2 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		QVERIFY(lastModified2 > lastModified1);

		// When the subcomposition is reinstalled, it gets recompiled.
		sleep(2);
		VuoCompilerNodeClass *oldNodeClass = compiler->getNodeClass(nodeClassName);
		compiler->setDelegate(compilerDelegate);
		compilerDelegate->installModule(origCompositionPath, copiedCompositionPath);
		VuoCompilerNodeClass *newNodeClass = compiler->getNodeClass(nodeClassName);
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
		unsigned long lastModified3 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		QVERIFY(lastModified3 > lastModified2);
		QVERIFY(oldNodeClass != newNodeClass);

		// The compiler has 'shouldLoadAllModules' turned off.
		// When a node class other than the subcomposition is loaded, the subcomposition does not get compiled.
		delete compiler;
		VuoCompiler::reset();
		VuoFileUtilities::deleteFile(compiledCompositionPath);
		compiler = initCompiler();
		compiler->setLoadAllModules(false);
		compiler->getNodeClass("vuo.event.fireOnStart");
		QVERIFY(! VuoFileUtilities::fileExists(compiledCompositionPath));

		// When the subcomposition node class is loaded, the subcomposition gets compiled.
		compiler->getNodeClass(nodeClassName);
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));


		string outerNodeClassName = "vuo.test.lengthenEsAndDoubleAndAddK";
		string outerOrigCompositionPath = getCompositionPath("LengthenEsAndDoubleAndAddK.vuo");
		string outerCopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + outerNodeClassName + ".vuo";
		string outerCompiledCompositionPath = getCachedCompiledCompositionPath(outerNodeClassName);

		string midNodeClassName = "vuo.test.lengthenEsAndDouble";
		string midOrigCompositionPath = getCompositionPath("LengthenEsAndDouble.vuo");
		string midCopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + midNodeClassName + ".vuo";
		string midCompiledCompositionPath = getCachedCompiledCompositionPath(midNodeClassName);

		string inner1NodeClassName = "vuo.test.lengthenEs";
		string inner1OrigCompositionPath = getCompositionPath("LengthenEs.vuo");
		string inner1CopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + inner1NodeClassName + ".vuo";
		string inner1CompiledCompositionPath = getCachedCompiledCompositionPath(inner1NodeClassName);

		string inner2NodeClassName = "vuo.test.appendWithSpaces";
		string inner2OrigCompositionPath = getCompositionPath("AppendWithSpaces.vuo");
		string inner2CopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + inner2NodeClassName + ".vuo";
		string inner2CompiledCompositionPath = getCachedCompiledCompositionPath(inner2NodeClassName);

		// When a subcomposition containing another subcomposition is loaded, the inner subcomposition gets
		// compiled first so that the outer subcomposition can be compiled — with 'shouldLoadAllModules" turned off...
		delete compiler;
		VuoCompiler::reset();
		VuoFileUtilities::deleteFile(copiedCompositionPath);
		VuoFileUtilities::deleteFile(compiledCompositionPath);
		VuoFileUtilities::copyFile(outerOrigCompositionPath, outerCopiedCompositionPath);
		VuoFileUtilities::copyFile(midOrigCompositionPath, midCopiedCompositionPath);
		VuoFileUtilities::copyFile(inner1OrigCompositionPath, inner1CopiedCompositionPath);
		VuoFileUtilities::copyFile(inner2OrigCompositionPath, inner2CopiedCompositionPath);
		compiler = initCompiler();
		compiler->setLoadAllModules(false);
		QVERIFY(! VuoFileUtilities::fileExists(outerCompiledCompositionPath));
		QVERIFY(! VuoFileUtilities::fileExists(midCompiledCompositionPath));
		QVERIFY(! VuoFileUtilities::fileExists(inner1CompiledCompositionPath));
		QVERIFY(! VuoFileUtilities::fileExists(inner2CompiledCompositionPath));
		VuoCompilerNodeClass *outerNodeClass = compiler->getNodeClass(outerNodeClassName);
		QVERIFY(outerNodeClass != NULL);
		QVERIFY(VuoFileUtilities::fileExists(outerCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(midCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner1CompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner2CompiledCompositionPath));

		// ... and with 'shouldLoadAllModules' turned on.
		delete compiler;
		VuoCompiler::reset();
		VuoFileUtilities::deleteFile(outerCompiledCompositionPath);
		VuoFileUtilities::deleteFile(midCompiledCompositionPath);
		VuoFileUtilities::deleteFile(inner1CompiledCompositionPath);
		VuoFileUtilities::deleteFile(inner2CompiledCompositionPath);
		compiler = initCompiler();
		compiler->getNodeClasses();
		QVERIFY(VuoFileUtilities::fileExists(outerCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(midCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner1CompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner2CompiledCompositionPath));

		// When a subcomposition is reinstalled, each subcomposition containing it gets recompiled.
		sleep(1);
		VuoCompilerNodeClass *oldOuterNodeClass = compiler->getNodeClass(outerNodeClassName);
		VuoCompilerNodeClass *oldMidNodeClass = compiler->getNodeClass(midNodeClassName);
		VuoCompilerNodeClass *oldInner1NodeClass = compiler->getNodeClass(inner1NodeClassName);
		VuoCompilerNodeClass *oldInner2NodeClass = compiler->getNodeClass(inner2NodeClassName);
		compiler->setDelegate(compilerDelegate);
		compilerDelegate->installModuleWithSuperficialChange(inner1OrigCompositionPath, inner1CopiedCompositionPath);
		VuoCompilerNodeClass *newOuterNodeClass = compiler->getNodeClass(outerNodeClassName);
		QVERIFY(newOuterNodeClass);
		QVERIFY(oldOuterNodeClass != newOuterNodeClass);
		VuoCompilerNodeClass *newMidNodeClass = compiler->getNodeClass(midNodeClassName);
		QVERIFY(newMidNodeClass);
		QVERIFY(oldMidNodeClass != newMidNodeClass);
		VuoCompilerNodeClass *newInner1NodeClass = compiler->getNodeClass(inner1NodeClassName);
		QVERIFY(newInner1NodeClass);
		QVERIFY(oldInner1NodeClass != newInner1NodeClass);
		VuoCompilerNodeClass *newInner2NodeClass = compiler->getNodeClass(inner2NodeClassName);
		QVERIFY(newInner2NodeClass);
		QVERIFY(oldInner2NodeClass == newInner2NodeClass);
	}

	void testOverridingInstalledSubcomposition()
	{
		delete compiler;
		compiler = NULL;
		VuoCompiler::reset();

		// Install two different versions of the same subcomposition,
		// one in user modules and the other in composition-local modules.

		string compositionLocalPath = VuoFileUtilities::getTmpDir() + "/TestSubcompositions";
		string nodeClassName = "vuo.test.lengthenEs";
		string origUserSubcompositionPath = getCompositionPath("LengthenEs.vuo");
		string origCompSubcompositionPath = getCompositionPath("LengthenBeeps.vuo");
		string installedUserSubcompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
		string installedCompSubcompositionPath = compositionLocalPath + "/Modules/" + nodeClassName + ".vuo";

		VuoFileUtilities::makeDir(compositionLocalPath + "/Modules/");
		VuoFileUtilities::copyFile(origUserSubcompositionPath, installedUserSubcompositionPath);
		VuoFileUtilities::copyFile(origCompSubcompositionPath, installedCompSubcompositionPath);

		VuoFileUtilities::copyFile(getCompositionPath("AppendWithSpaces.vuo"), VuoFileUtilities::getUserModulesPath() + "/vuo.test.appendWithSpaces.vuo");

		// Build and run a composition that refers to the subcomposition,
		// once with access to only the user modules version (that version should be used)
		// and once with access to both versions (the composition-local version should be used).

		string compositionPath = getCompositionPath("LengthenEsAndDouble.vuo");
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("LengthenEsAndDouble", "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("LengthenEsAndDouble-linked", "");

		VuoCompiler *compilers[4];
		VuoCompilerNodeClass *nodeClasses[4];
		string installDirs[2] = { VuoFileUtilities::getUserModulesPath(), compositionLocalPath };
		string expectedOutputs[2] = { "e e", "bep bep" };
		for (int i = 0; i < 4; ++i)
		{
			compilers[i] = new VuoCompiler(installDirs[i%2] + "/unused");
			nodeClasses[i] = compilers[i]->getNodeClass(nodeClassName);

			VuoCompilerIssues *issues = new VuoCompilerIssues();
			compilers[i]->compileComposition(compositionPath, compiledCompositionPath, true, issues);
			compilers[i]->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
			remove(compiledCompositionPath.c_str());

			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, "", false, true);
			runner->start();

			VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("Lengthen");
			VuoRunner::Port *outPort = runner->getPublishedOutputPortWithName("CompositeText");
			runner->firePublishedInputPortEvent(inPort);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString::fromStdString(expectedOutputs[i%2]));

			runner->stop();
			delete runner;
			delete issues;
		}

		QVERIFY(nodeClasses[0] != nodeClasses[1]);
		QVERIFY(nodeClasses[0] == nodeClasses[2]);
		QVERIFY(nodeClasses[1] == nodeClasses[3]);

		for (int i = 0; i < 4; ++i)
			delete compilers[i];

		VuoFileUtilities::deleteDir(compositionLocalPath);
	}

private:

	class TestErrorsCompilerDelegate : public VuoCompilerDelegate
	{
	private:
		VuoCompilerIssues *issues;
		dispatch_queue_t issuesQueue;

		void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
						   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
						   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
		{
			dispatch_sync(issuesQueue, ^{
							  this->issues->append(issues);
						  });

			loadedModulesCompleted();
		}

	public:
		TestErrorsCompilerDelegate(void)
		{
			issues = new VuoCompilerIssues();
			issuesQueue = dispatch_queue_create("org.vuo.TestSubcompositions", NULL);
		}

		map<string, VuoCompilerIssue> getIssuesByFilePath(VuoCompiler *compiler)
		{
			compiler->getNodeClasses();

			__block map<string, VuoCompilerIssue> issuesByFilePath;
			dispatch_sync(issuesQueue, ^{
							  vector<VuoCompilerIssue> issueList = issues->getList();
							  for (vector<VuoCompilerIssue>::iterator i = issueList.begin(); i != issueList.end(); ++i) {
								  issuesByFilePath[(*i).getFilePath()] = *i;
							  }

							  delete issues;
							  issues = new VuoCompilerIssues();
						  });

			return issuesByFilePath;
		}
	};

private slots:

	void testErrors()
	{
		string parseErrorNodeClassName = "vuo.test.parseError";
		string copiedParseErrorPath = VuoFileUtilities::getUserModulesPath() + "/" + parseErrorNodeClassName + ".vuo";
		string compiledParseErrorPath = getCachedCompiledCompositionPath(parseErrorNodeClassName);

		// Try to install a subcomposition that can't be parsed.
		compiler->setDelegate(NULL);
		VuoFileUtilities::writeStringToFile("", copiedParseErrorPath);
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(parseErrorNodeClassName);
		QVERIFY(! nodeClass);
		QVERIFY(! VuoFileUtilities::fileExists(compiledParseErrorPath));

		TestErrorsCompilerDelegate delegate;

		// Initialize a compiler when the Modules folder contains a subcomposition that can't be parsed.
		delete compiler;
		VuoCompiler::reset();
		VuoFileUtilities::writeStringToFile("", copiedParseErrorPath);
		compiler = initCompiler();
		compiler->setDelegate(&delegate);
		map<string, VuoCompilerIssue> issues = delegate.getIssuesByFilePath(compiler);
		QVERIFY(issues.find(copiedParseErrorPath) != issues.end());
		QVERIFY(! VuoFileUtilities::fileExists(compiledParseErrorPath));
		VuoFileUtilities::deleteFile(copiedParseErrorPath);


		string lengthenEsAndDoublePath = getCompositionPath("LengthenEsAndDouble.vuo");
		string lengthenEsAndDoubleAndAddKPath = getCompositionPath("LengthenEsAndDoubleAndAddK.vuo");
		string lengthenEsNodeClassName = "vuo.test.lengthenEs";
		string copiedLengthenEsPath = VuoFileUtilities::getUserModulesPath() + "/" + lengthenEsNodeClassName + ".vuo";
		string compiledLengthenEsPath = getCachedCompiledCompositionPath(lengthenEsNodeClassName);
		string appendWithSpacesPath = getCompositionPath("AppendWithSpaces.vuo");

		// Initialize a compiler when the Modules folder contains a subcomposition that contains an instance of itself.
		delete compiler;
		VuoCompiler::reset();
		installSubcomposition(appendWithSpacesPath);
		VuoFileUtilities::copyFile(lengthenEsAndDoublePath, copiedLengthenEsPath);
		compiler = initCompiler();
		compiler->setDelegate(&delegate);
		issues = delegate.getIssuesByFilePath(compiler);
		QVERIFY(issues.find(copiedLengthenEsPath) != issues.end());
		QVERIFY(VuoStringUtilities::beginsWith(issues[copiedLengthenEsPath].getShortDescription(false), "Subcomposition contains itself"));
		QVERIFY(! VuoFileUtilities::fileExists(compiledLengthenEsPath));

		// Initialize a compiler when the Modules folder contains a subcomposition that indirectly contains an instance of itself.
		delete compiler;
		VuoCompiler::reset();
		installSubcomposition(lengthenEsAndDoublePath);
		VuoFileUtilities::copyFile(lengthenEsAndDoubleAndAddKPath, copiedLengthenEsPath);
		compiler = initCompiler();
		compiler->setDelegate(&delegate);
		issues = delegate.getIssuesByFilePath(compiler);
		QVERIFY(issues.find(copiedLengthenEsPath) != issues.end());
		QVERIFY(VuoStringUtilities::beginsWith(issues[copiedLengthenEsPath].getShortDescription(false), "Subcomposition contains itself"));
		QVERIFY(! VuoFileUtilities::fileExists(compiledLengthenEsPath));
	}
};

int main(int argc, char *argv[])
{
    qInstallMessageHandler(VuoRendererCommon::messageHandler);
    TestSubcompositions tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "TestSubcompositions.moc"
