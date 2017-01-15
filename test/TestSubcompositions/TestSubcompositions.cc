/**
 * @file
 * TestSubcompositions interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
typedef QList<VuoPortClass::EventBlocking> EventBlockingList;
Q_DECLARE_METATYPE(EventBlockingList);
typedef QList<bool> BoolList;
Q_DECLARE_METATYPE(BoolList);
typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap);


/**
 * Tests compiling, loading, and executing subcompositions.
 */
class TestSubcompositions : public TestCompositionExecution
{
	Q_OBJECT

private:

	VuoCompiler *compiler;

	/**
	 * Copy the provided composition into the "User Modules" directory in preparation for installation as a subcomposition.
	 * Return the path to which it was copied.
	 */
	string copyIntoSubcompositionDir(string compositionPath)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);

		string copiedCompositionFileName = file + "." + ext;
		copiedCompositionFileName[0] = tolower(copiedCompositionFileName[0]);
		copiedCompositionFileName = "vuo.test." + copiedCompositionFileName;
		string installedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + copiedCompositionFileName;
		VuoFileUtilities::copyFile(compositionPath, installedCompositionPath);

		return installedCompositionPath;
	}

	/**
	 * For each .vuo file in the compositions folder, delete the corresponding .vuo file and .vuonode file in the Modules folder.
	 */
	void uninstallSubcompositions()
	{
		QDir compositionDir = getCompositionDir();
		QStringList filter("*.vuo");
		QStringList compositionFileNames = compositionDir.entryList(filter);
		foreach (QString compositionFileName, compositionFileNames)
		{
			string dir, nodeClassName, ext;
			VuoFileUtilities::splitPath(compositionFileName.toStdString(), dir, nodeClassName, ext);
			nodeClassName[0] = tolower(nodeClassName[0]);
			nodeClassName = "vuo.test." + nodeClassName;

			if (compiler)
				compiler->uninstallSubcomposition(nodeClassName);

			string copiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
			remove(copiedCompositionPath.c_str());
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
	}

	void cleanup()
	{
		uninstallSubcompositions();
		delete compiler;
	}

	void testNodeInterface_data(void)
	{
		QTest::addColumn< QString >("compositionName");
		QTest::addColumn< QString >("expectedNodeClassName");
		QTest::addColumn< QString >("expectedDefaultTitle");
		QTest::addColumn< bool >("expectedIsInterface");
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
			QTest::newRow("Empty composition") << "Empty" << "vuo.test.empty" << "Empty" << false << false << inputNamesTemplate << QStringList() << inputTypesTemplate << QStringList() << QStringList() << QStringList() << eventBlockingTemplate << BoolList();
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

			QTest::newRow("All kinds of ports") << "PortKinds" << "vuo.test.portKinds" << "Port Kinds" << true << true << inputNames << inputDisplayNames << inputTypes << outputNames << outputDisplayNames << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In");
			inputTypes.append("VuoPoint2d");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out");
			outputTypes.append("VuoPoint2d");
			isTrigger.append(true);

			QTest::newRow("Trigger joining event flow to output") << "TriggerConfluence" << "vuo.test.triggerConfluence" << "Trigger Confluence" << true << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("Trigger joining data flow to output") << "TriggerBlocked" << "vuo.test.triggerBlocked" << "Trigger Blocked" << true << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("Input event to door") << "AllowAlternating" << "vuo.test.allowAlternating" << "Allow Alternating" << false << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("Input event to wall") << "InputBlocked" << "vuo.test.inputBlocked" << "Input Blocked" << false << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In1");
			inputTypes.append("VuoInteger");
			eventBlocking.append(VuoPortClass::EventBlocking_None);
			inputNames.append("In2");
			inputTypes.append("event");
			eventBlocking.append(VuoPortClass::EventBlocking_None);

			QStringList outputNames;
			QStringList outputTypes;
			BoolList isTrigger;
			outputNames.append("Out1");
			outputTypes.append("VuoInteger");
			isTrigger.append(false);

			QTest::newRow("Input event to wall and output") << "InputBlockedAndNotBlocked" << "vuo.test.inputBlockedAndNotBlocked" << "Input Blocked And Not Blocked" << false << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
		}
		{
			QStringList inputNames = inputNamesTemplate;
			QStringList inputTypes = inputTypesTemplate;
			EventBlockingList eventBlocking = eventBlockingTemplate;
			inputNames.append("In1");
			inputTypes.append("VuoPoint3d");
			eventBlocking.append(VuoPortClass::EventBlocking_Door);
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

			QTest::newRow("Input event to wall and door") << "InputBlockedAndDoored" << "vuo.test.inputBlockedAndDoored" << "Input Blocked And Doored" << false << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("Only trigger output ports") << "FireNPeriodically" << "vuo.test.fireNPeriodically" << "Fire N Periodically" << false << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("No output ports") << "SendCheckerboard" << "vuo.test.sendCheckerboard" << "Send Checkerboard" << true << true << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("Unconnected input port") << "PublishedInputAndOutput" << "vuo.test.publishedInputAndOutput" << "Published Input And Output" << false << false << inputNames << QStringList() << inputTypes << outputNames << QStringList() << outputTypes << eventBlocking << isTrigger;
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

			QTest::newRow("All-caps port name") << "IsHttps" << "vuo.test.isHttps" << "Is Https" << false << false << inputNames << inputDisplayNames << inputTypes << outputNames << outputDisplayNames << outputTypes << eventBlocking << isTrigger;
		}
	}
	void testNodeInterface(void)
	{
		QFETCH(QString, compositionName);
		QFETCH(QString, expectedNodeClassName);
		QFETCH(QString, expectedDefaultTitle);
		QFETCH(bool, expectedIsInterface);
		QFETCH(bool, expectedIsStateful);
		QFETCH(QStringList, expectedInputPortNames);
		QFETCH(QStringList, expectedInputPortDisplayNames);
		QFETCH(QStringList, expectedInputPortTypes);
		QFETCH(QStringList, expectedOutputPortNames);
		QFETCH(QStringList, expectedOutputPortDisplayNames);
		QFETCH(QStringList, expectedOutputPortTypes);
		QFETCH(EventBlockingList, expectedEventBlocking);
		QFETCH(BoolList, expectedIsTrigger);

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string copiedSubcompositionPath = copyIntoSubcompositionDir(compositionPath);
		VuoCompilerNodeClass *subcomposition = compiler->installSubcomposition(copiedSubcompositionPath);

		QVERIFY(subcomposition != NULL);

		QCOMPARE(QString::fromStdString(subcomposition->getBase()->getClassName()), expectedNodeClassName);
		QCOMPARE(QString::fromStdString(subcomposition->getBase()->getDefaultTitle()), expectedDefaultTitle);
		QCOMPARE(subcomposition->getBase()->isInterface(), expectedIsInterface);
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

		void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
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
					runner->fireTriggerPortEvent(fireOnStartIdentifier);
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
			expectedOutputPortData["CompositeText"] = "<code></code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("CompositeText");
			QTest::newRow("Default list input port value") << "AppendWithSpaces" << subcompositionNames << "Texts" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}
		{
			QStringList subcompositionNames;
			QStringMap inputPortData;
			inputPortData["Value"] = "4";
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Sum"] = "5";
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
/*		{
			/// @todo https://b33p.net/kosada/node/11146 Need port actions so we can send an event into 'Go to First' without also hitting 'Go Forward'.
			QStringMap inputPortData;
			QStringMap expectedOutputPortData;
			expectedOutputPortData["Season"] = "<code>Spring</code>";
			QStringList expectedOutputPortEvents;
			expectedOutputPortEvents.append("Season");
			QTest::newRow("Wall on input port") << "CycleSeasons" << "GoToFirst" << inputPortData << expectedOutputPortData << expectedOutputPortEvents << expectedOutputPortEvents;
		}*/
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
			expectedOutputPortData["ExclamationTexts"] = "List containing 2 items: <ul><li><code>It's a test!</code></li><li><code>Whee!</code></li></ul>";
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
			string compositionPath = getCompositionPath(currentCompositionName.toStdString() + ".vuo");
			string copiedSubcompositionPath = copyIntoSubcompositionDir(compositionPath);
			VuoCompilerNodeClass *nodeClass = compiler->installSubcomposition(copiedSubcompositionPath);
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
		compiler->compileComposition(topLevelComposition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
		remove(compiledCompositionPath.c_str());
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
		runner->subscribeToAllTelemetry();
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
			string compositionPath = getCompositionPath(subcompositionNames[i] + ".vuo");
			string copiedSubcompositionPath = copyIntoSubcompositionDir(compositionPath);
			compiler->installSubcomposition(copiedSubcompositionPath);
		}

		string compositionName = "MultipleSubcompositions";
		string compositionPath = getCompositionPath(compositionName + ".vuo");
		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoCompilerComposition *topLevelComposition = new VuoCompilerComposition(new VuoComposition(), parser);
		delete parser;

		// Build and run the top-level composition.

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionName, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(compositionName + "-linked", "");
		compiler->compileComposition(topLevelComposition, compiledCompositionPath);
		compiler->linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_SmallBinary);
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
		runner->subscribeToAllTelemetry();
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

	void testLiveCoding()
	{
		string subcompositionNames[] = { "LengthenEs", "FireAlternating" };
		for (int i = 0; i < 2; ++i)
		{
			string compositionPath = getCompositionPath(subcompositionNames[i] + ".vuo");
			string copiedSubcompositionPath = copyIntoSubcompositionDir(compositionPath);
			compiler->installSubcomposition(copiedSubcompositionPath);
		}

		string compositionPath = getCompositionPath("PublishedInputAndOutput.vuo");

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		vector<string> alreadyLinkedResourcePaths;
		set<string> alreadyLinkedResources;

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		VuoRunner::Port *outPort = NULL;
		string originalCompositionGraphviz;

		// Build and run the original composition.
		{
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			VuoComposition *baseComposition = new VuoComposition();
			composition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;

			originalCompositionGraphviz = composition->getGraphvizDeclaration();

			compiler->compileComposition(composition, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource0", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, resourceDylibPath, compositionDir, false, true);
			runner->start();

			outPort = runner->getPublishedOutputPortWithName("Out");
		}

		// Replace the composition with one in which a stateful subcomposition node and published input and output cables have been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.lengthenEs");
			VuoNode *node = compiler->createNode(nodeClass);
			VuoCompilerPort *lengthenPort = static_cast<VuoCompilerPort *>( node->getInputPortWithName("Lengthen")->getCompiler() );
			VuoCompilerPort *esPort = static_cast<VuoCompilerPort *>( node->getOutputPortWithName("Es")->getCompiler() );
			composition->getBase()->addNode(node);

			VuoPort *basePublishedInputPort = composition->getBase()->getPublishedInputPorts().front();
			VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedInputPort->getCompiler() );
			VuoCompilerCable *inputCable = new VuoCompilerCable(NULL, publishedInputPort, node->getCompiler(), lengthenPort);
			composition->getBase()->addCable(inputCable->getBase());

			VuoPort *basePublishedOutputPort = composition->getBase()->getPublishedOutputPorts().front();
			VuoCompilerPublishedPort *publishedOutputPort = static_cast<VuoCompilerPublishedPort *>( basePublishedOutputPort->getCompiler() );
			VuoCompilerCable *outputCable = new VuoCompilerCable(node->getCompiler(), esPort, NULL, publishedOutputPort);
			composition->getBase()->addCable(outputCable->getBase());

			compiler->compileComposition(composition, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource1", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler, set<VuoCompilerComposition::NodeReplacement>());
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);

			// Fire an event through the published input port.
			runner->firePublishedInputPortEvent();
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("e"));
		}

		// Replace the composition with one in which a subcomposition node that fires events has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			VuoCompilerNodeClass *nodeClass = compiler->getNodeClass("vuo.test.fireAlternating");
			VuoNode *node = compiler->createNode(nodeClass);
			composition->getBase()->addNode(node);

			compiler->compileComposition(composition, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource2", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = composition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler, set<VuoCompilerComposition::NodeReplacement>());
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);

			// Fire an event through the published input port.
			runner->firePublishedInputPortEvent();
			runner->waitForAnyPublishedOutputPortEvent();
			QCOMPARE(QString(VuoText_makeFromJson(runner->getPublishedOutputPortValue(outPort))), QString("ee"));
		}

		// Replace the latest composition with the original composition.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			VuoCompilerComposition *originalComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(originalCompositionGraphviz, compiler);

			compiler->compileComposition(originalComposition, bcPath);
			string resourceDylibPath = VuoFileUtilities::makeTmpFile(file + "-resource3", "dylib");
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, resourceDylibPath, alreadyLinkedResourcePaths, alreadyLinkedResources);
			remove(bcPath.c_str());

			string compositionDiff = originalComposition->diffAgainstOlderComposition(oldCompositionGraphviz, compiler, set<VuoCompilerComposition::NodeReplacement>());
			runner->replaceComposition(dylibPath, resourceDylibPath, compositionDiff);
			delete originalComposition;
		}

		runner->stop();

		for (vector<string>::iterator i = alreadyLinkedResourcePaths.begin(); i != alreadyLinkedResourcePaths.end(); ++i)
			remove((*i).c_str());

		delete runner;
		delete composition;
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

		string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
		string copiedSubcompositionPath = copyIntoSubcompositionDir(compositionPath);
		compiler->installSubcomposition(copiedSubcompositionPath);
	}

	void testLoadingInstalledSubcompositions()
	{
		string nodeClassName = "vuo.test.alternate";
		string origCompositionPath = getCompositionPath("Alternate.vuo");
		string copiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + nodeClassName + ".vuo";
		string compiledCompositionPath = VuoCompiler::getCachedModulesPath() + "/" + nodeClassName + ".vuonode";
		VuoFileUtilities::copyFile(origCompositionPath, copiedCompositionPath);

		// The subcomposition source file is in the Modules folder when the compiler is initialized.
		// When any node class is loaded, the subcomposition gets compiled.
		delete compiler;
		compiler = initCompiler();
		QVERIFY(! VuoFileUtilities::fileExists(compiledCompositionPath));
		compiler->getNodeClass("vuo.event.fireOnStart");
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));

		// The subcomposition source file is newer than the compiled file when the compiler is initialized.
		// When any node class is loaded, the subcomposition gets recompiled.
		unsigned long lastModified1 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		sleep(2);
		VuoFileUtilities::copyFile(origCompositionPath, copiedCompositionPath);
		delete compiler;
		compiler = initCompiler();
		compiler->getNodeClass("vuo.event.fireOnStart");
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
		unsigned long lastModified2 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		QVERIFY(lastModified2 > lastModified1);

		// When the subcomposition is reinstalled, it gets recompiled.
		sleep(2);
		VuoCompilerNodeClass *oldNodeClass = compiler->getNodeClass(nodeClassName);
		VuoCompilerNodeClass *newNodeClass = compiler->installSubcomposition(copiedCompositionPath);
		QVERIFY(VuoFileUtilities::fileExists(compiledCompositionPath));
		unsigned long lastModified3 = VuoFileUtilities::getFileLastModifiedInSeconds(compiledCompositionPath);
		QVERIFY(lastModified3 > lastModified2);
		QVERIFY(oldNodeClass != newNodeClass);

		// The compiler has 'shouldLoadAllModules' turned off.
		// When a node class other than the subcomposition is loaded, the subcomposition does not get compiled.
		VuoFileUtilities::deleteFile(compiledCompositionPath);
		delete compiler;
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
		string outerCompiledCompositionPath = VuoCompiler::getCachedModulesPath() + "/" + outerNodeClassName + ".vuonode";

		string midNodeClassName = "vuo.test.lengthenEsAndDouble";
		string midOrigCompositionPath = getCompositionPath("LengthenEsAndDouble.vuo");
		string midCopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + midNodeClassName + ".vuo";
		string midCompiledCompositionPath = VuoCompiler::getCachedModulesPath() + "/" + midNodeClassName + ".vuonode";

		string inner1NodeClassName = "vuo.test.lengthenEs";
		string inner1OrigCompositionPath = getCompositionPath("LengthenEs.vuo");
		string inner1CopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + inner1NodeClassName + ".vuo";
		string inner1CompiledCompositionPath = VuoCompiler::getCachedModulesPath() + "/" + inner1NodeClassName + ".vuonode";

		string inner2NodeClassName = "vuo.test.appendWithSpaces";
		string inner2OrigCompositionPath = getCompositionPath("AppendWithSpaces.vuo");
		string inner2CopiedCompositionPath = VuoFileUtilities::getUserModulesPath() + "/" + inner2NodeClassName + ".vuo";
		string inner2CompiledCompositionPath = VuoCompiler::getCachedModulesPath() + "/" + inner2NodeClassName + ".vuonode";

		// When a subcomposition containing another subcomposition is loaded, the inner subcomposition gets
		// compiled first so that the outer subcomposition can be compiled — with 'shouldLoadAllModules" turned off...
		VuoFileUtilities::deleteFile(copiedCompositionPath);
		VuoFileUtilities::deleteFile(compiledCompositionPath);
		VuoFileUtilities::copyFile(outerOrigCompositionPath, outerCopiedCompositionPath);
		VuoFileUtilities::copyFile(midOrigCompositionPath, midCopiedCompositionPath);
		VuoFileUtilities::copyFile(inner1OrigCompositionPath, inner1CopiedCompositionPath);
		VuoFileUtilities::copyFile(inner2OrigCompositionPath, inner2CopiedCompositionPath);
		delete compiler;
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
		VuoFileUtilities::deleteFile(outerCompiledCompositionPath);
		VuoFileUtilities::deleteFile(midCompiledCompositionPath);
		VuoFileUtilities::deleteFile(inner1CompiledCompositionPath);
		VuoFileUtilities::deleteFile(inner2CompiledCompositionPath);
		delete compiler;
		compiler = initCompiler();
		compiler->getNodeClass("vuo.event.fireOnStart");
		QVERIFY(VuoFileUtilities::fileExists(outerCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(midCompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner1CompiledCompositionPath));
		QVERIFY(VuoFileUtilities::fileExists(inner2CompiledCompositionPath));

		// When a subcomposition is reinstalled, each subcomposition containing it gets recompiled.
		VuoCompilerNodeClass *oldOuterNodeClass = compiler->getNodeClass(outerNodeClassName);
		VuoCompilerNodeClass *oldMidNodeClass = compiler->getNodeClass(midNodeClassName);
		VuoCompilerNodeClass *oldInner1NodeClass = compiler->getNodeClass(inner1NodeClassName);
		VuoCompilerNodeClass *oldInner2NodeClass = compiler->getNodeClass(inner2NodeClassName);
		compiler->installSubcomposition(inner1CopiedCompositionPath);
		QVERIFY(oldOuterNodeClass != compiler->getNodeClass(outerNodeClassName));
		QVERIFY(oldMidNodeClass != compiler->getNodeClass(midNodeClassName));
		QVERIFY(oldInner1NodeClass != compiler->getNodeClass(inner1NodeClassName));
		QVERIFY(oldInner2NodeClass == compiler->getNodeClass(inner2NodeClassName));
	}

	void testErrors()
	{
		string copiedParseErrorPath = VuoFileUtilities::getUserModulesPath() + "/vuo.test.parseError.vuo";
		string compiledParseErrorPath = VuoCompiler::getCachedModulesPath() + "/vuo.test.parseError.vuonode";

		// Try to install a subcomposition that can't be parsed.
		VuoFileUtilities::writeStringToFile("", copiedParseErrorPath);
		try {
			compiler->installSubcomposition(copiedParseErrorPath);
			QFAIL("");
		} catch (VuoCompilerException &e) {
			QVERIFY(! VuoFileUtilities::fileExists(compiledParseErrorPath));
		}

		// Initialize a compiler when the Modules folder contains a subcomposition that can't be parsed.
		VuoFileUtilities::writeStringToFile("", copiedParseErrorPath);
		delete compiler;
		compiler = initCompiler();
		compiler->getNodeClasses();
		map<string, VuoCompilerException> errors = compiler->flushErrorsLoadingModules();
		QVERIFY(errors.find(copiedParseErrorPath) != errors.end());
		QVERIFY(! VuoFileUtilities::fileExists(compiledParseErrorPath));
		VuoFileUtilities::deleteFile(copiedParseErrorPath);


		string lengthenEsAndDoublePath = getCompositionPath("LengthenEsAndDouble.vuo");
		string lengthenEsAndDoubleAndAddKPath = getCompositionPath("LengthenEsAndDoubleAndAddK.vuo");
		string copiedLengthenEsPath = VuoFileUtilities::getUserModulesPath() + "/vuo.test.lengthenEs.vuo";
		string compiledLengthenEsPath = VuoCompiler::getCachedModulesPath() + "/vuo.test.lengthenEs.vuonode";
		string appendWithSpacesPath = getCompositionPath("AppendWithSpaces.vuo");

		// Initialize a compiler when the Modules folder contains a subcomposition that contains an instance of itself.
		copyIntoSubcompositionDir(appendWithSpacesPath);
		VuoFileUtilities::copyFile(lengthenEsAndDoublePath, copiedLengthenEsPath);
		delete compiler;
		compiler = initCompiler();
		compiler->getNodeClasses();
		errors = compiler->flushErrorsLoadingModules();
		QVERIFY(errors.find(copiedLengthenEsPath) != errors.end());
		QVERIFY(VuoStringUtilities::beginsWith(errors[copiedLengthenEsPath].what(), "Subcomposition contains itself"));
		QVERIFY(! VuoFileUtilities::fileExists(compiledLengthenEsPath));

		// Initialize a compiler when the Modules folder contains a subcomposition that indirectly contains an instance of itself.
		copyIntoSubcompositionDir(lengthenEsAndDoublePath);
		VuoFileUtilities::copyFile(lengthenEsAndDoubleAndAddKPath, copiedLengthenEsPath);
		delete compiler;
		compiler = initCompiler();
		compiler->getNodeClasses();
		errors = compiler->flushErrorsLoadingModules();
		QVERIFY(errors.find(copiedLengthenEsPath) != errors.end());
		QVERIFY(VuoStringUtilities::beginsWith(errors[copiedLengthenEsPath].what(), "Subcomposition contains itself"));
		QVERIFY(! VuoFileUtilities::fileExists(compiledLengthenEsPath));
	}
};


QTEST_APPLESS_MAIN(TestSubcompositions)
#include "TestSubcompositions.moc"
