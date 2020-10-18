/**
 * @file
 * TestVuoEditor implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>
#include "VuoEditorWindow.hh"
#include "VuoEditorComposition.hh"
#include "VuoExampleMenu.hh"
#include <QQuickWindow>
#include <QtNetwork/QNetworkReply>
#include "VuoEditor.hh"
#include "VuoEditorUtilities.hh"
#include "VuoExampleMenu.hh"
#include "VuoCodeWindow.hh"
#include "VuoModuleManager.hh"
#include "VuoSearchBox.hh"
#include <sstream>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(set<QString>);
Q_DECLARE_METATYPE(set<string>);
Q_DECLARE_METATYPE(VuoNode::TintColor);

/**
 * Tests for the VuoEditor* classes.
 */
class TestVuoEditor : public QObject
{
	Q_OBJECT

public:
	TestVuoEditor()
	{
		window = NULL;
		composition = NULL;
		compiler = NULL;
	}

private:
	VuoEditorWindow *window;
	VuoEditorComposition *composition;
	VuoCompiler *compiler;

	VuoCompiler * initCompiler()
	{
		VuoCompiler *c = new VuoCompiler();
		return c;
	}

	string getCompositionPath(string compositionFileName)
	{
		QDir compositionDir = QDir::current();
		compositionDir.cd("composition");
		return compositionDir.filePath(QString(compositionFileName.c_str())).toStdString();
	}

	void initTestCaseWithCompositionFile(QString compositionFile)
	{
		// Initialize a VuoEditorWindow.
		if (compositionFile.isEmpty())
		{
			window = new VuoEditorWindow();
		}
		else
		{
			string compositionPath = getCompositionPath(compositionFile.toStdString());
			string compositionString = VuoFileUtilities::readFileToString(compositionPath);
			window = new VuoEditorWindow(compositionPath.c_str(), compositionPath.c_str(), compositionString);
		}

		composition = window->composition;
	}

	void waitWithoutBlockingMainThread(float seconds)
	{
		for (int i = 0; i < seconds * 1000; ++i)
		{
			VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
			struct timespec t;
			t.tv_sec = 0;
			t.tv_nsec = 1000000;
			nanosleep(&t, nullptr);
		}
	}

private slots:
	void initTestCase()
	{
		compiler = initCompiler();
		initTestCaseWithCompositionFile("");
	}

	void cleanupTestCase()
	{
		delete window;
		window = NULL;
	}

	void testMultipleInputCables_data()
	{
		QTest::addColumn<QString>("toNode");
		QTest::addColumn<QString>("toPort");

		QTest::addColumn<QString>("fromNode");
		QTest::addColumn<QString>("fromPort");

		QTest::addColumn<QString>("fromNode2");
		QTest::addColumn<QString>("fromPort2");

		QTest::addColumn<QString>("fromNode3");
		QTest::addColumn<QString>("fromPort3");

		QTest::addColumn<int>("expectedFinalNumDataCables");
		QTest::addColumn<int>("expectedFinalNumEventCables");

		// OK: 2 event-only cables -> event-only port
		QTest::newRow("2 event cables to event port")               << "vuo.event.allowFirst"       << "event"      << "vuo.event.fireOnStart"  << "started"    << "vuo.event.fireOnStart"          << "started"            << ""                       << ""       << 0    << 2;

		// OK: 1 event-only cable + constant -> data-event port
		QTest::newRow("1 event cable + constant to data port")		<< "vuo.math.count.VuoInteger"	<< "increment"	<< "vuo.event.fireOnStart"	<< "started"	<< ""								<< ""					<< ""						<< ""		<< 0	<< 1;

		// OK: 1 event-only cable + 1 data-event cable -> data-event port
		QTest::newRow("1 event cable + 1 data cable to data port")	<< "vuo.math.count.VuoInteger"	<< "increment"	<< "vuo.event.fireOnStart"	<< "started"	<< "vuo.math.subtract.VuoInteger"	<< "difference"			<< ""						<< ""		<< 1	<< 1;

		// OK: 1 event-only cable + 1 data-event cable -> event-only port
		QTest::newRow("1 event cable + 1 data cable to event port") << "vuo.event.allowFirst"       << "event"      << "vuo.event.fireOnStart"  << "started"    << "vuo.math.subtract.VuoInteger"   << "difference"         << ""                       << ""       << 0    << 2;

		// NOT OK: 1 event-only cable + 2 data-event cables -> data-event port
		// Connection of second data+event cable should displace the previous data+event cable.
		QTest::newRow("1 event cable + 2 data cables to data port")	<< "vuo.math.count.VuoInteger"	<< "increment"	<< "vuo.event.fireOnStart"	<< "started"	<< "vuo.math.subtract.VuoInteger"	<< "difference"			<< "vuo.math.subtract.VuoInteger"	<< "difference"	<< 1	<< 1;
	}
	void testMultipleInputCables()
	{
		QFETCH(QString, toNode);
		QFETCH(QString, toPort);
		QFETCH(QString, fromNode);
		QFETCH(QString, fromPort);
		QFETCH(QString, fromNode2);
		QFETCH(QString, fromPort2);
		QFETCH(QString, fromNode3);
		QFETCH(QString, fromPort3);

		QFETCH(int, expectedFinalNumDataCables);
		QFETCH(int, expectedFinalNumEventCables);

		// Create mousePress and mouseRelease events to simulate cable creation by dragging between ports.
		QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
		pressEvent.setButton(Qt::LeftButton);

		QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
		moveEvent.setButtons(Qt::LeftButton);
		// Make sure that the simulated move event covers enough distance that it is not ignored as mouse jitter.
		moveEvent.setButtonDownScreenPos(Qt::LeftButton, QPoint(0,0));
		moveEvent.setScreenPos(QPoint(100,100));

		QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
		releaseEvent.setButton(Qt::LeftButton);

		VuoNode *toN, *fromN, *fromN2, *fromN3;
		VuoPort *toP, *fromP, *fromP2, *fromP3;

		toN = compiler->getNodeClass(toNode.toStdString())->newNode();
		toP = toN->getInputPortWithName(toPort.toStdString());
		toN->setY(100);
		composition->addNode(toN);

		fromN = compiler->getNodeClass(fromNode.toStdString())->newNode();
		fromP = fromN->getOutputPortWithName(fromPort.toStdString());
		fromN->setY(200);
		composition->addNode(fromN);

		// Create cable: fromN:fromP -> toN:toP
		pressEvent.setScenePos(fromP->getRenderer()->scenePos());
		QApplication::sendEvent(composition, &pressEvent);

		QApplication::sendEvent(composition, &moveEvent);

		releaseEvent.setScenePos(toP->getRenderer()->scenePos());
		QApplication::sendEvent(composition, &releaseEvent);

		// Connect second cable, if applicable.
		if (! (fromNode2.isEmpty() || fromPort2.isEmpty()))
		{
			fromN2 = compiler->getNodeClass(fromNode2.toStdString())->newNode();
			fromP2 = fromN2->getOutputPortWithName(fromPort2.toStdString());
			fromN2->setY(300);
			composition->addNode(fromN2);

			// Create cable: fromN2:fromP2 -> toN:toP
			pressEvent.setScenePos(fromP2->getRenderer()->scenePos());
			QApplication::sendEvent(composition, &pressEvent);

			QApplication::sendEvent(composition, &moveEvent);

			releaseEvent.setScenePos(toP->getRenderer()->scenePos());
			QApplication::sendEvent(composition, &releaseEvent);
		}

		// Connect third cable, if applicable.
		if (! (fromNode3.isEmpty() || fromPort3.isEmpty()))
		{
			fromN3 = compiler->getNodeClass(fromNode3.toStdString())->newNode();
			fromP3 = fromN3->getOutputPortWithName(fromPort3.toStdString());
			fromN3->setY(400);
			composition->addNode(fromN3);

			// Create cable: fromN3:fromP3 -> toN:toP
			pressEvent.setScenePos(fromP3->getRenderer()->scenePos());
			QApplication::sendEvent(composition, &pressEvent);

			QApplication::sendEvent(composition, &moveEvent);

			releaseEvent.setScenePos(toP->getRenderer()->scenePos());
			QApplication::sendEvent(composition, &releaseEvent);
		}

		// Check how many cables of each type were successfully connected.
		vector<VuoCable *> connectedCables = toP->getConnectedCables();
		int numConnectedDataCables = 0;
		for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		{
			if ((*cable)->getCompiler()->carriesData())
				numConnectedDataCables++;
		}

		QCOMPARE(numConnectedDataCables, expectedFinalNumDataCables);
		QCOMPARE(((int)connectedCables.size() - numConnectedDataCables), expectedFinalNumEventCables);
	}

	void testDeleteChainReaction_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< set<QString> >("componentsToExplicitlyDelete");
		QTest::addColumn< set<QString> >("expectedSurvivingNodes");
		QTest::addColumn< set<QString> >("expectedSurvivingCables");

		/**
		 * Tests on composition "DeleteChainTest.vuo"
		 */
		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract:difference -> ConvertIntegertoText:integer;");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("Subtract");
			expectedSurvivingNodes.insert("SelectInput");

			set<QString> expectedSurvivingCables;

			QTest::newRow("Data+event cable deletion triggering deletion of connected collapsed typecast (including typecast's hidden output cable)")	<< "DeleteChainTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("SelectInput");

			set<QString> expectedSurvivingCables;

			QTest::newRow("Node deletion triggering deletion of output data+event cable and cable's connected typecast (including typecast's hidden output cable)")	<< "DeleteChainTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("SelectInput");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("Subtract");

			set<QString> expectedSurvivingCables;

			QTest::newRow("Node deletion triggering deletion of attached collapsed typecast and its connected data+event input cable and (hidden) output cable")	<< "DeleteChainTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		/**
		 * Tests on composition "DeleteChainTestWithRerouting.vuo"
		 */
		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("FireonStart:started -> ConvertIntegertoText:integer;");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("Subtract");
			expectedSurvivingNodes.insert("SelectInput");
			expectedSurvivingNodes.insert("FireonStart");
			expectedSurvivingNodes.insert("FireonStart2");
			expectedSurvivingNodes.insert("ConvertIntegertoText");

			set<QString> expectedSurvivingCables;
			expectedSurvivingCables.insert("Subtract:difference -> ConvertIntegertoText:integer;");
			expectedSurvivingCables.insert("FireonStart2:started -> ConvertIntegertoText:integer;");
			expectedSurvivingCables.insert("ConvertIntegertoText:text -> SelectInput:option1;");

			QTest::newRow("Collapsed typecast enduring deletion of incoming event-only cable")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract:difference -> ConvertIntegertoText:integer;");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("Subtract");
			expectedSurvivingNodes.insert("SelectInput");
			expectedSurvivingNodes.insert("FireonStart");
			expectedSurvivingNodes.insert("FireonStart2");

			set<QString> expectedSurvivingCables;
			expectedSurvivingCables.insert("FireonStart:started -> SelectInput:option1;");
			expectedSurvivingCables.insert("FireonStart2:started -> SelectInput:option1;");

			QTest::newRow("Data+event cable deletion triggering deletion of connected collapsed typecast (including typecast's hidden output cable) and re-routing of event cables")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("SelectInput");
			expectedSurvivingNodes.insert("FireonStart");
			expectedSurvivingNodes.insert("FireonStart2");

			set<QString> expectedSurvivingCables;
			expectedSurvivingCables.insert("FireonStart:started -> SelectInput:option1;");
			expectedSurvivingCables.insert("FireonStart2:started -> SelectInput:option1;");

			QTest::newRow("Node deletion triggering deletion of output data+event cable and cable's connected typecast (including typecast's hidden output cable) and re-routing of event cables")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract");
			componentsToExplicitlyDelete.insert("FireonStart:started -> ConvertIntegertoText:integer;");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("SelectInput");
			expectedSurvivingNodes.insert("FireonStart");
			expectedSurvivingNodes.insert("FireonStart2");

			set<QString> expectedSurvivingCables;
			expectedSurvivingCables.insert("FireonStart2:started -> SelectInput:option1;");

			QTest::newRow("Cable marked for both, direct deletion and re-routing in compound delete operation is correctly deleted")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("Subtract");
			componentsToExplicitlyDelete.insert("FireonStart");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("SelectInput");
			expectedSurvivingNodes.insert("FireonStart2");

			set<QString> expectedSurvivingCables;
			expectedSurvivingCables.insert("FireonStart2:started -> SelectInput:option1;");

			QTest::newRow("Cable marked for both, indirect deletion and re-routing in compound delete operation is correctly deleted")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}

		{
			set<QString> componentsToExplicitlyDelete;
			componentsToExplicitlyDelete.insert("SelectInput");

			set<QString> expectedSurvivingNodes;
			expectedSurvivingNodes.insert("Subtract");
			expectedSurvivingNodes.insert("FireonStart");
			expectedSurvivingNodes.insert("FireonStart2");

			set<QString> expectedSurvivingCables;

			QTest::newRow("Node deletion triggering deletion of attached collapsed typecast and its connected event-only/data+event input cables and (hidden) output cable")	<< "DeleteChainWithReroutingTest.vuo"	<< componentsToExplicitlyDelete	<< expectedSurvivingNodes	<< expectedSurvivingCables;
		}
	}
	void testDeleteChainReaction()
	{
		QFETCH(QString, compositionFile);
		QFETCH(set<QString>, componentsToExplicitlyDelete);
		QFETCH(set<QString>, expectedSurvivingNodes);
		QFETCH(set<QString>, expectedSurvivingCables);

		cleanupTestCase();
		initTestCaseWithCompositionFile(compositionFile);

		// Select the specified composition components.
		QList<QGraphicsItem *> initialComponents = composition->items();

		for (QList<QGraphicsItem *>::iterator i = initialComponents.begin(); i != initialComponents.end(); ++i)
		{
			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);
			VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(*i);

			if (rn)
			{
				QString nodeName = rn->getBase()->getCompiler()->getGraphvizIdentifier().c_str();
				if (componentsToExplicitlyDelete.find(nodeName) != componentsToExplicitlyDelete.end())
					rn->setSelected(true);
			}

			else if (rc)
			{
				QString cableName = rc->getBase()->getCompiler()->getGraphvizDeclaration().c_str();
				if (componentsToExplicitlyDelete.find(cableName) != componentsToExplicitlyDelete.end())
					rc->setSelected(true);
			}
		}

		// Create a keyPress event to simulate pressing 'Backspace' to delete the selected components.
		QKeyEvent keyPressEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
		QApplication::sendEvent(composition, &keyPressEvent);

		// Now see what items remain in the scene.
		set<QString> actualSurvivingNodes;
		set<QString> actualSurvivingCables;
		QList<QGraphicsItem *> survivingComponents = composition->items();

		for (QList<QGraphicsItem *>::iterator i = survivingComponents.begin(); i != survivingComponents.end(); ++i)
		{
			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);
			VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(*i);

			if (rn)
			{
				QString survivingNodeName = rn->getBase()->getCompiler()->getGraphvizIdentifier().c_str();
				actualSurvivingNodes.insert(survivingNodeName);

			}
			else if (rc)
			{
				QString survivingCableName = rc->getBase()->getCompiler()->getGraphvizDeclaration().c_str();
				actualSurvivingCables.insert(survivingCableName);
			}
		}

		QCOMPARE(actualSurvivingNodes, expectedSurvivingNodes);
		QCOMPARE(actualSurvivingCables, expectedSurvivingCables);
	}

	void testTypecastCollapsing_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< QString >("typecast");
		QTest::addColumn< bool >("collapseExpected");

		QTest::newRow("Typecast that has no output cables")							<< "UncollapsibleTypecasts.vuo"	<< "ConvertTexttoInteger3"		<< false;
		QTest::newRow("Typecast that has multiple output cables")					<< "UncollapsibleTypecasts.vuo"	<< "ConvertTexttoInteger2"		<< false;
		QTest::newRow("Typecast that outputs to another typecast")					<< "UncollapsibleTypecasts.vuo"	<< "ConvertTexttoInteger"		<< false;
		QTest::newRow("Typecast that outputs to a port with multiple input cables")	<< "UncollapsibleTypecasts.vuo"	<< "ConvertIntegertoText"		<< false;
		QTest::newRow("Typecast that has no input data+event cables")				<< "UncollapsibleTypecasts.vuo"	<< "ConvertIntegertoText2"		<< false;
		QTest::newRow("Typecast with an incoming cable to its 'refresh' port")				<< "UncollapsibleTypecasts.vuo"	<< "ConvertTexttoInteger5"		<< false;

		QTest::newRow("Final typecast in a typecast chain")							<< "TypecastChain.vuo"			<< "ConvertIntegertoText"		<< true;
		QTest::newRow("Non-final typecast in a typecast chain")						<< "TypecastChain.vuo"			<< "ConvertTexttoInteger"		<< false;
		QTest::newRow("Non-final typecast 2 in a typecast chain")					<< "TypecastChain.vuo"			<< "ConvertIntegertoText2"		<< false;
		QTest::newRow("Non-final typecast 3 in a typecast chain")					<< "TypecastChain.vuo"			<< "ConvertTexttoInteger2"		<< false;
	}
	void testTypecastCollapsing()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, typecast);
		QFETCH(bool, collapseExpected);

		cleanupTestCase();
		initTestCaseWithCompositionFile(compositionFile);

		// Check whether the target typecast was collapsed.
		QList<QGraphicsItem *> components = composition->items();
		bool targetFound = false;
		bool targetCollapsed = false;
		for (QList<QGraphicsItem *>::iterator i = components.begin(); (! targetFound) && (i != components.end()); ++i)
		{
			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);

			if (rn)
			{
				QString nodeName = rn->getBase()->getCompiler()->getGraphvizIdentifier().c_str();
				if (nodeName == typecast)
				{
					targetFound = true;
					targetCollapsed = rn->getProxyNode();
				}
			}
		}

		QVERIFY2(targetFound, "The target typecast was not found.");
		QCOMPARE(targetCollapsed, collapseExpected);
	}

	void testUnspecializeGenericPort_data()
	{
		QTest::addColumn<QString>("compositionFile");
		QTest::addColumn<QString>("nodeTitle");
		QTest::addColumn<QString>("portName");
		QTest::addColumn< set<string> >("expectedNodesToReplace");
		QTest::addColumn< set<string> >("expectedCablesToDelete");

		{
			set<string> expectedNodesToReplace;
			expectedNodesToReplace.insert("HoldValue1 vuo.data.hold.VuoGenericType1");
			set<string> expectedCablesToDelete;
			QTest::newRow("Replace one node") << "Generics.vuo" << "HoldValue1" << "heldValue" << expectedNodesToReplace << expectedCablesToDelete;
		}

		{
			set<string> expectedNodesToReplace;
			expectedNodesToReplace.insert("Hold1 vuo.data.hold.VuoGenericType1");
			expectedNodesToReplace.insert("MakeList1 vuo.list.make.2.VuoGenericType1");
			expectedNodesToReplace.insert("Add1 vuo.math.add.VuoGenericType1");
			set<string> expectedCablesToDelete;
			expectedCablesToDelete.insert("Add1:sum -> ConvertIntegertoText1:integer");
			QTest::newRow("Replace multiple nodes and remove cables") << "Recur_Hold_Add_Write_loop.vuo" << "Hold1" << "heldValue" << expectedNodesToReplace << expectedCablesToDelete;
		}

		{
			set<string> expectedNodesToReplace;
			expectedNodesToReplace.insert("MakeList vuo.list.make.2.VuoGenericType1");
			expectedNodesToReplace.insert("CountItemsInList vuo.list.count.VuoGenericType1");
			set<string> expectedCablesToDelete;
			QTest::newRow("Replace node with VuoList and no singleton") << "CountItemsInList.vuo" << "CountItemsInList" << "list" << expectedNodesToReplace << expectedCablesToDelete;
		}

		{
			set<string> expectedNodesToReplace;
			expectedNodesToReplace.insert("HoldValue1 vuo.data.hold.VuoGenericType1");
			set<string> expectedCablesToDelete;
			expectedCablesToDelete.insert("PublishedInputs:newValue -> HoldValue1:newValue");
			expectedCablesToDelete.insert("HoldValue1:heldValue -> PublishedOutputs:heldValue");
			QTest::newRow("Replace node that has published ports") << "HoldValuePublished.vuo" << "HoldValue1" << "heldValue" << expectedNodesToReplace << expectedCablesToDelete;
		}
	}
	void testUnspecializeGenericPort()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, nodeTitle);
		QFETCH(QString, portName);
		QFETCH(set<string>, expectedNodesToReplace);
		QFETCH(set<string>, expectedCablesToDelete);

		cleanupTestCase();
		initTestCaseWithCompositionFile(compositionFile);

		VuoPort *portToUnspecialize = NULL;
		set<VuoNode *> nodes = composition->getBase()->getNodes();
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

				portToUnspecialize = port;
			}
		}
		QVERIFY(portToUnspecialize != NULL);

		map<VuoNode *, string> nodesToReplace;
		set<VuoCable *> cablesToDelete;
		composition->createReplacementsToUnspecializePort(portToUnspecialize, true, nodesToReplace, cablesToDelete);

		for (map<VuoNode *, string>::iterator i = nodesToReplace.begin(); i != nodesToReplace.end(); ++i)
		{
			string nodeAndNodeClassName = i->first->getTitle() + " " + i->second;
			QVERIFY2(expectedNodesToReplace.find(nodeAndNodeClassName) != expectedNodesToReplace.end(), nodeAndNodeClassName.c_str());
		}
		QCOMPARE(nodesToReplace.size(), expectedNodesToReplace.size());

		for (set<VuoCable *>::iterator i = cablesToDelete.begin(); i != cablesToDelete.end(); ++i)
		{
			string cable = (*i)->getFromNode()->getTitle() + ":" + (*i)->getFromPort()->getClass()->getName() + " -> " +
						   (*i)->getToNode()->getTitle() + ":" + (*i)->getToPort()->getClass()->getName();
			QVERIFY2(expectedCablesToDelete.find(cable) != expectedCablesToDelete.end(), cable.c_str());
		}
		QCOMPARE(cablesToDelete.size(), expectedCablesToDelete.size());
	}

	/**
	 * Ensure that only the most commonly-used types appear at the root of the Set Data Type menu.
	 */
	void testRootLevelTypeSpecializationMenu()
	{
		set<VuoCompilerType *> actualRootLevelTypes = composition->moduleManager->getLoadedTypesForNodeSet()[""];
		for (set<VuoCompilerType *>::iterator it = actualRootLevelTypes.begin(); it != actualRootLevelTypes.end(); )
			if (VuoType::isDictionaryTypeName((*it)->getBase()->getModuleKey())
				   || VuoType::isListTypeName((*it)->getBase()->getModuleKey()))
				actualRootLevelTypes.erase(it++);
			else
				++it;

		set<VuoCompilerType *> allowedRootLevelTypes;
		allowedRootLevelTypes.insert(compiler->getType("VuoPoint2d"));
		allowedRootLevelTypes.insert(compiler->getType("VuoPoint3d"));
		allowedRootLevelTypes.insert(compiler->getType("VuoPoint4d"));
		allowedRootLevelTypes.insert(compiler->getType("VuoBoolean"));
		allowedRootLevelTypes.insert(compiler->getType("VuoColor"));
		allowedRootLevelTypes.insert(compiler->getType("VuoImage"));
		allowedRootLevelTypes.insert(compiler->getType("VuoInteger"));
		allowedRootLevelTypes.insert(compiler->getType("VuoLayer"));
		allowedRootLevelTypes.insert(compiler->getType("VuoReal"));
		allowedRootLevelTypes.insert(compiler->getType("VuoSceneObject"));
		allowedRootLevelTypes.insert(compiler->getType("VuoText"));

		set<VuoCompilerType *> extraRootLevelTypes;
		std::set_difference(actualRootLevelTypes.begin(), actualRootLevelTypes.end(),
							allowedRootLevelTypes.begin(), allowedRootLevelTypes.end(),
							std::inserter(extraRootLevelTypes, extraRootLevelTypes.end()));

		ostringstream oss;
		oss << "Unexpected root-level types found: ";
		for (set<VuoCompilerType *>::iterator it = extraRootLevelTypes.begin(); it != extraRootLevelTypes.end(); ++it)
			oss << (*it)->getBase()->getModuleKey() << " ";
		oss << "— add to VuoModuleManager::getPrimaryAffiliatedNodeSetForType";
		QVERIFY2(extraRootLevelTypes.empty(), oss.str().c_str());
	}

	void testCableTint_data()
	{
		QTest::addColumn< QString >("cableID");
		QTest::addColumn< VuoNode::TintColor >("expectedTint");

		QTest::newRow("Internal cable") << "SubtractYellow:difference -> SubtractGrey:a;" << VuoNode::TintYellow;
		QTest::newRow("Published input cable from non-protocol port") << "PublishedInputs:Text -> CutGrey:text;" << VuoNode::TintNone;
		QTest::newRow("Published input cable from protocol port") << "PublishedInputs:time -> SubtractYellow:a;" << VuoRendererColors::getActiveProtocolTint(0, false);
	}

	void testCableTint()
	{
		QFETCH(QString, cableID);
		QFETCH(VuoNode::TintColor, expectedTint);

		cleanupTestCase();
		initTestCaseWithCompositionFile("CableTint.vuo");

		QList<QGraphicsItem *> components = composition->items();

		for (QList<QGraphicsItem *>::iterator i = components.begin(); i != components.end(); ++i)
		{
			VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(*i);
			if (rc)
			{
				QString currentCableID = rc->getBase()->getCompiler()->getGraphvizDeclaration().c_str();
				if (currentCableID == cableID)
					QCOMPARE(rc->getTintColor(), expectedTint);
			}
		}
	}

	void testSavingCompositions_data()
	{
		QTest::addColumn< QString >("savePath");
		QTest::addColumn< QString >("chmodCommand");
		QTest::addColumn< bool >("saveShouldSucceed");
		QTest::addColumn< QString >("dirToRemove");

		/* @todo Re-enable the following tests once dialog boxes can be suppressed during testing (https://b33p.net/kosada/node/4283):

		QString dir1 = QString(VuoFileUtilities::makeTmpDir("readOnlyDir").c_str());
		QTest::newRow("Save within a read-only directory")	<< QString(dir1).append("/").append("Test.vuo")	<< QString("chmod a-w ").append(dir1)	<< false	<< QString(dir1);

		QString file2 = QString(VuoFileUtilities::makeTmpFile("readOnlyFile", "vuo").c_str());
		QTest::newRow("Modification of a read-only composition")		<< file2										<< QString("chmod a-w ").append(file2)	<< false	<< "";
		*/

		QString file3 = QString(VuoFileUtilities::makeTmpFile("writableFile", "vuo").c_str());
		QTest::newRow("Modification of a writable composition")		<< file3										<< ""									<< true		<< "";

	}
	void testSavingCompositions()
	{
		QFETCH(QString, savePath);
		QFETCH(QString, chmodCommand);
		QFETCH(bool, saveShouldSucceed);
		QFETCH(QString, dirToRemove);

		cleanupTestCase();
		initTestCase();

		// Set the appropriate write permissions for the test.
		if (! chmodCommand.isEmpty())
			system(string(chmodCommand.toUtf8().data()).c_str());

		// The specific composition opened here is irrelevant.
		initTestCaseWithCompositionFile("TypecastChain.vuo");

		// Mark the composition as having unsaved changes.
		window->setCompositionModified(true);

		// Attempt to save the composition at the new path.
		window->saveFile(savePath);

		// The composition should now be considered to have unsaved changes
		// if and only if the "Save As..." operation failed.
		QCOMPARE(saveShouldSucceed, (! window->isWindowModified()));

		// Clean up temporary files.
		remove(string(savePath.toUtf8().data()).c_str());
		if (! dirToRemove.isEmpty())
			remove(string(dirToRemove.toUtf8().data()).c_str());
	}

	void testSavingNewShader_data()
	{
		QTest::addColumn<QString>("protocol");

		QTest::newRow("Image Filter") << "Image Filter";
		QTest::newRow("Image Generator") << "Image Generator";
		QTest::newRow("Image Transition") << "Image Transition";
	}
	void testSavingNewShader()
	{
		QFETCH(QString, protocol);

		initTestCaseWithCompositionFile("");

		// File > New Shader > (protocol)
		QAction *newShaderAction = nullptr;
		for (QAction *a : static_cast<VuoEditor *>(qApp)->menuFile->actions())
		{
			if (a->menu() && a->text() == "New Shader")
			{
				for (QAction *aa : a->menu()->actions())
				{
					if (aa->text() == protocol)
					{
						newShaderAction = aa;
						break;
					}
				}
				break;
			}
		}
		QVERIFY(newShaderAction);
		newShaderAction->trigger();

		// Wait for modules to finish loading
		waitWithoutBlockingMainThread(0.1);

		// Save file
		QList<QMainWindow *> openWindows = VuoEditorUtilities::getOpenEditingWindows();
		QCOMPARE(openWindows.size(), 1);
		VuoCodeWindow *shaderWindow = dynamic_cast<VuoCodeWindow *>(openWindows.front());
		QVERIFY(shaderWindow);
		string saveDir = VuoFileUtilities::makeTmpDir("testSavingNewShader");
		string savePath = saveDir + "/" + protocol.toStdString() + ".fs";
		shaderWindow->saveToPath(QString::fromStdString(savePath));
		QVERIFY(VuoFileUtilities::fileExists(savePath));

		// Wait for modules to finish loading
		waitWithoutBlockingMainThread(0.1);

		// Clean up
		VuoFileUtilities::deleteDir(saveDir);
		delete shaderWindow;
	}

	void testValidBuildDate_data()
	{
		QTest::addColumn< QDate >("editorBuildDate");

		QDate buildDate = ((VuoEditor *)qApp)->getBuildDate();
		QTest::newRow(QString("Build date: ").append(buildDate.toString()).toUtf8().data()) << buildDate;
	}
	void testValidBuildDate()
	{
		QFETCH(QDate, editorBuildDate);
		QVERIFY2(editorBuildDate.isValid(), "Invalid build date.");

		QString RFC2822BuildDateTimeString = QDateTime(editorBuildDate).toString(Qt::RFC2822Date);
		QDate RFC2822ParsedBuildDate = ((VuoEditor *)qApp)->getDateFromRFC2822String(RFC2822BuildDateTimeString);
		QVERIFY2(RFC2822ParsedBuildDate.isValid(), QString("Invalid RFC2822-formatted build date: %1.").arg(RFC2822BuildDateTimeString).toUtf8().constData());
		QVERIFY2(editorBuildDate == RFC2822ParsedBuildDate, QString("Build date differs after RFC2822 formatting: %1").arg(RFC2822BuildDateTimeString).toUtf8().constData());
	}

	void testValidVersion_data()
	{
		QTest::addColumn< QString >("version");
		QTest::addColumn< bool >("expectedValid");

		QString editorVersion = VUO_VERSION_STRING;
		QTest::newRow(QString("Current version: ").append(editorVersion).toUtf8().data()) << editorVersion << true;
		QTest::newRow("Multi-digit version") << "12.345.6789" << true;

		QTest::newRow("Major version with illegal (non-numeric) characters") << "a.0.1" << false;
		QTest::newRow("Major version with illegal (non-numeric) characters") << "1a.0.1" << false;
		QTest::newRow("Minor version with illegal (non-numeric) characters") << "1.a.0" << false;
		QTest::newRow("Minor version with illegal (non-numeric) characters") << "1.1a.0" << false;
		QTest::newRow("Patch version with illegal (non-numeric) characters") << "1.0.a" << false;
		QTest::newRow("Patch version with illegal (non-numeric) characters") << "1.0.1a" << false;
		QTest::newRow("Patch version with illegal (non-numeric) characters") << "1.0.1-alpha" << false;
		QTest::newRow("Version with too few fields") << "1.0" << false;
		QTest::newRow("Version with too many fields") << "1.0.0.0" << false;
		QTest::newRow("Empty version string") << "" << false;
	}

	void testValidVersion()
	{
		QFETCH(QString, version);
		QFETCH(bool, expectedValid);

		QVERIFY(((VuoEditor *)qApp)->versionStringIsValid(version) == expectedValid);
	}

	void testVersionComparison_data()
	{
		QTest::addColumn< QString >("version1");
		QTest::addColumn< QString >("version2");
		QTest::addColumn< QDate >("buildDate1");
		QTest::addColumn< QDate >("buildDate2");
		QTest::addColumn< bool >("expectedLessThanResult");

		QTest::newRow("Major version difference") << "1.0.0" << "2.0.0" << QDate() << QDate() << true;
		QTest::newRow("Major version difference") << "2.0.0" << "1.0.0" << QDate() << QDate() << false;

		QTest::newRow("Minor version difference") << "1.1.0" << "1.2.0" << QDate() << QDate() << true;
		QTest::newRow("Minor version difference") << "1.2.0" << "1.1.0" << QDate() << QDate() << false;

		QTest::newRow("Patch version difference") << "1.2.2" << "1.2.3" << QDate() << QDate() << true;
		QTest::newRow("Patch version difference") << "1.2.3" << "1.2.2" << QDate() << QDate() << false;

		QString version = VUO_VERSION_STRING;
		QDate buildDate = ((VuoEditor *)qApp)->getBuildDate();

		QTest::newRow("Matching versions, no dates") << version << version << QDate() << QDate() << false;
		QTest::newRow("Matching versions, matching dates") << version << version << buildDate << buildDate << false;

		QDate futureBuildDate(buildDate.year()+1, buildDate.month(), buildDate.day());
		QDate pastBuildDate(buildDate.year()-1, buildDate.month(), buildDate.day());

		QTest::newRow("Matching versions, different build dates") << version << version << buildDate << futureBuildDate << true;
		QTest::newRow("Matching versions, different build dates") << version << version << futureBuildDate << buildDate << false;
		QTest::newRow("Matching versions, different build dates") << version << version << buildDate << pastBuildDate << false;
		QTest::newRow("Matching versions, different build dates") << version << version << pastBuildDate << buildDate << true;

		QTest::newRow("Invalid version") << "badVersion" << version << QDate() << QDate() << false;
		QTest::newRow("Invalid version") << version << "badVersion" << QDate() << QDate() << false;
	}
	void testVersionComparison()
	{
		QFETCH(QString, version1);
		QFETCH(QString, version2);
		QFETCH(QDate, buildDate1);
		QFETCH(QDate, buildDate2);
		QFETCH(bool, expectedLessThanResult);

		bool lessThanResult = ((VuoEditor *)qApp)->versionLessThan(version1, version2, buildDate1, buildDate2);

		QVERIFY(lessThanResult == expectedLessThanResult);
	}

	void testSearch_data()
	{
		QTest::addColumn<QString>("compositionFile");
		QTest::addColumn<QString>("searchText");
		QTest::addColumn< vector<QString> >("expectedSelectedNodes");

		vector<QString> empty;

		{
			// https://b33p.net/kosada/node/10737#comment-57139
			vector<QString> expectedSelectedNodes;
			expectedSelectedNodes.push_back("ArrangeLayersInGrid");
			QTest::newRow("Find rendered constant values, but not raw constant data") << "RenderedTextInvisibleConstant.vuo" << "6" << expectedSelectedNodes;
		}

		// https://b33p.net/kosada/node/10737#comment-57538
		QTest::newRow("When a cable is connected, don't find the port's default value") << "HideDefaultWhenCableConnected.vuo" << "1024" << empty;

		{
			// https://b33p.net/kosada/node/10737#comment-57660
			vector<QString> expectedSelectedNodes;
			expectedSelectedNodes.push_back("MakeList3");
			QTest::newRow("Find constant values in lists") << "ListConstant.vuo" << "42" << expectedSelectedNodes;
		}

		{
			// https://b33p.net/kosada/node/10737#comment-57660
			vector<QString> expectedSelectedNodes;
			expectedSelectedNodes.push_back("FireOnStart");
			expectedSelectedNodes.push_back("ListFiles");
			expectedSelectedNodes.push_back("ProcessList3");
			expectedSelectedNodes.push_back("ArrangeLayersInGrid");
			expectedSelectedNodes.push_back("MakeScaledLayer");
			expectedSelectedNodes.push_back("GetFileURLValues");
			QTest::newRow("Ensure results are in left-right, top-bottom order") << "RenderedTextInvisibleConstant.vuo" << "fi" << expectedSelectedNodes;
		}

		{
			// https://b33p.net/kosada/node/10737#comment-57660
			vector<QString> expectedSelectedNodes;
			expectedSelectedNodes.push_back("PixellateImage");
			QTest::newRow("Find port display names")        << "PortDisplayName.vuo" << "width" << expectedSelectedNodes;
			QTest::newRow("Don't find port internal names") << "PortDisplayName.vuo" << "size"  << empty;
		}

		{
			// https://b33p.net/kosada/node/10737#comment-57660
			vector<QString> expectedSelectedNodes;
			expectedSelectedNodes.push_back("MakeColorLayer");
			QTest::newRow("Find type converter display names") << "TypeconverterName.vuo" << "summary" << expectedSelectedNodes;
		}

		// https://b33p.net/kosada/node/10737#comment-57660
		QTest::newRow("Match beginnings (not middles) of words") << "WordBeginsWith.vuo" << "all" << empty;
	}
	void testSearch()
	{
		QFETCH(QString, compositionFile);
		QFETCH(QString, searchText);
		QFETCH(vector<QString>, expectedSelectedNodes);

		cleanupTestCase();
		initTestCaseWithCompositionFile(compositionFile);

		VuoSearchBox searchBox(composition, NULL, 0);

		vector<QGraphicsItem *> selectedNodes = searchBox.getCurrentSearchResults(searchText);
		QCOMPARE(selectedNodes.size(), expectedSelectedNodes.size());

		size_t count = selectedNodes.size();
		for (int i = 0; i < count; ++i)
			QCOMPARE(static_cast<VuoRendererNode *>(selectedNodes[i])->getBase()->getRawGraphvizIdentifier().c_str(), expectedSelectedNodes[i].toUtf8().constData());
	}

	void testNodeLibrarySearch_data()
	{
		QTest::addColumn<QStringList>({"filterTokens"});
		QTest::addColumn<QString>("expectedTopNodeClass");

		{
			QTest::newRow("Special handling for arithmetic operator '+'") << (QStringList() << "+") << "vuo.math.add";
			QTest::newRow("Special handling for arithmetic operator '-'") << (QStringList() << "-") << "vuo.math.subtract";
			QTest::newRow("Special handling for arithmetic operator '/'") << (QStringList() << "/") << "vuo.math.divide.VuoReal";
			QTest::newRow("Special handling for arithmetic operator '*'") << (QStringList() << "*") << "vuo.math.multiply";
			QTest::newRow("Special handling for arithmetic operator '•'") << (QStringList() << "•") << "vuo.math.multiply";
			QTest::newRow("Special handling for arithmetic operator '×'") << (QStringList() << "×") << "vuo.math.multiply";
			QTest::newRow("Special handling for arithmetic operator '|'") << (QStringList() << "|") << "vuo.math.absolute";
			QTest::newRow("Special handling for arithmetic operator '%'") << (QStringList() << "%") << "vuo.math.limitToRange";
			QTest::newRow("Special handling for arithmetic operator '^'") << (QStringList() << "^") << "vuo.math.exponentiate";
			QTest::newRow("Special handling for comparison operator '<'") << (QStringList() << "<") << "vuo.data.isLessThan";
			QTest::newRow("Special handling for comparison operator '>'") << (QStringList() << ">") << "vuo.data.isGreaterThan";
			QTest::newRow("Special handling for comparison operator '=='") << (QStringList() << "==") << "vuo.data.areEqual";
			QTest::newRow("Special handling for comparison operator '='") << (QStringList() << "=") << "vuo.data.areEqual";
			QTest::newRow("Special handling for comparison operator '!='") << (QStringList() << "!=") << "vuo.math.areNotEqual";
			QTest::newRow("Special handling for comparison operator '<>'") << (QStringList() << "<>") << "vuo.math.areNotEqual";
			QTest::newRow("Special handling for comparison operator '≠'") << (QStringList() << "≠") << "vuo.math.areNotEqual";
			QTest::newRow("Special handling for comparison operator '<='") << (QStringList() << "<=") << "vuo.math.compareNumbers";
			QTest::newRow("Special handling for comparison operator '>='") << (QStringList() << ">=") << "vuo.math.compareNumbers";
			QTest::newRow("Special handling for comparison operator '=<'") << (QStringList() << "=<") << "vuo.math.compareNumbers";
			QTest::newRow("Special handling for comparison operator '=>'") << (QStringList() << "=>") << "vuo.math.compareNumbers";
		}
	}

	void testNodeLibrarySearch()
	{
		QFETCH(QStringList, filterTokens);
		QFETCH(QString, expectedTopNodeClass);

		cleanupTestCase();
		initTestCase();

		VuoNodeLibrary nodeLibrary(compiler);
		composition->moduleManager->setNodeLibrary(&nodeLibrary);
		composition->moduleManager->updateWithAlreadyLoadedModules();
		vector<VuoCompilerNodeClass *> returnedNodeClasses = nodeLibrary.getMatchingNodeClassesForSearchTerms(filterTokens);

		QVERIFY2(returnedNodeClasses.size() >= 1, "No search results were returned.");
		QCOMPARE(QString(returnedNodeClasses[0]->getBase()->getClassName().c_str()), expectedTopNodeClass);
	}


	void testLaunchPerformance_data()
	{
		QTest::addColumn<QString>("args");

		QTest::newRow("Open empty composition and quit")           << "--quit";
		QTest::newRow("Open and run empty composition, then quit") << "--run --quit";
		QTest::newRow("Open AddNoiseToClay and quit")              << "vuo-example://vuo.scene/AddNoiseToClay.vuo --quit";
		QTest::newRow("Open and run AddNoiseToClay, then quit")    << "vuo-example://vuo.scene/AddNoiseToClay.vuo --run --quit";
		QTest::newRow("Open random and quit")                      << "--open-random --quit";
		QTest::newRow("Open recent and quit")                      << "--open-recent --quit";
	}
	void testLaunchPerformance()
	{
		QFETCH(QString, args);

		QBENCHMARK {
			QString vuoApp = "../../build/package/Vuo.app";
			if (!QFileInfo(vuoApp).exists())
				vuoApp = "../../build/bin/Vuo.app";
			int ret = QProcess::execute("/usr/bin/open --wait-apps " + vuoApp + " --args " + args);
			QCOMPARE(ret, 0);
		}
	}

	void testNodeClassTransliteration_data()
	{
		QTest::addColumn<QString>("category");
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<QString>("expectedOutput");

		QTest::newRow("empty")                       << ""                       << ""                    << "defaultCategory.defaultCompositionName";
		QTest::newRow("reduces to empty")            << " ./ "                   << "-\\'\"✅"            << "defaultCategory.defaultCompositionName";
		QTest::newRow("English")                     << "some user"              << "some composition"    << "someUser.someComposition";
		QTest::newRow("English, category")           << "magneson.util"          << "object.addVelocity"  << "magneson.util.object.addVelocity";
		QTest::newRow("English, category, more dots")<< "..magneson..util.."     << "..add..velocity.."   << "magneson.util.add.velocity";
		if (QSysInfo::productVersion() > "10.10")  // OS X 10.10 converts "ß" to emptystring, but macOS 10.14 properly converts it to "ss".
		{
		QTest::newRow("German")                      << "Österreich Ärger"       << "Rückstoß Gondoliere" << "osterreichArger.ruckstossGondoliere";
		QTest::newRow("German, combining diaeresis") << "Österreich Ärger"       << "Rückstoß Gondoliere" << "osterreichArger.ruckstossGondoliere";
		}
		QTest::newRow("Chinese")                     << "張中山"                  << "我迷路了"             << "zhangZhongShan.woMiLuLe";
		QTest::newRow("Japanese")                    << "ミクダリハンアッパーカット" << "生け花"               << "mikudarihanAppakatto.shengkeHua";
	}
	void testNodeClassTransliteration()
	{
		QFETCH(QString, category);
		QFETCH(QString, compositionName);
		QFETCH(QString, expectedOutput);

		QString output = VuoEditorWindow::getNodeClassNameForDisplayNameAndCategory(compositionName, category, "default composition name", "default category");
		QCOMPARE(output, expectedOutput);
	}

	void testRemoveVuoLinks_data()
	{
		QTest::addColumn<QString>("markdown");
		QTest::addColumn<QString>("expectedOutput");

		QTest::newRow("node")       << "[Make HSL Color](vuo-node://vuo.color.make.hsl)"                                                      << "`Make HSL Color`";
		QTest::newRow("2 nodes")    << "[Make HSL Color](vuo-node://vuo.color.make.hsl) text [Make RGB Color](vuo-node://vuo.color.make.rgb)" << "`Make HSL Color` text `Make RGB Color`";
		QTest::newRow("nodeset")    << "[vuo.osc](vuo-nodeset://vuo.osc)"                                                                     << "`vuo.osc`";
		QTest::newRow("2 nodesets") << "[vuo.osc](vuo-nodeset://vuo.osc) text [vuo.audio](vuo-nodeset://vuo.audio)"                           << "`vuo.osc` text `vuo.audio`";

		QTest::newRow("link + node")
			<< "[sRGB colorspace](https://en.wikipedia.org/wiki/SRGB) text [Make HSL Color](vuo-node://vuo.color.make.hsl)"
			<< "[sRGB colorspace](https://en.wikipedia.org/wiki/SRGB) text `Make HSL Color`";

		QTest::newRow("list of 2 links")
			<< "   - [Make Action Button Theme (Rounded)](vuo-node://vuo.ui.make.theme.button.rounded)\n"
			   "   - [Make Toggle Button Theme (Rounded)](vuo-node://vuo.ui.make.theme.toggle.rounded)\n"
			<< "   - `Make Action Button Theme (Rounded)`\n"
			   "   - `Make Toggle Button Theme (Rounded)`\n";
	}
	void testRemoveVuoLinks()
	{
		QFETCH(QString, markdown);
		QFETCH(QString, expectedOutput);

		QString actualOutput = QString::fromStdString(VuoEditor::removeVuoLinks(markdown.toStdString()));
		QCOMPARE(actualOutput, expectedOutput);
	}

	/**
	 * Returns the indentation depth of the specified action.
	 * 0 = not indented (normal).
	 */
	int getIndentationLevel(QAction *action)
	{
		int indentationLevel = 0;

		if (!action->icon().isNull())
		{
			QPixmap emptyPixmap(32, 32);
			emptyPixmap.fill(Qt::transparent);
			if (action->icon().pixmap(32, 32).toImage() == emptyPixmap.toImage())
				++indentationLevel;
		}
		if (action->text()[0] == ' ')
			++indentationLevel;

		return indentationLevel;
	}

	/**
	 * Recursively adds the menu items in `menu` to `menuPaths`.
	 */
	void addMenuPaths(QSet<QString> &menuPaths, QMenu *menu, QString path)
	{
		foreach (QAction *action, menu->actions())
		{
			if (action->isSeparator())
				continue;

			if (action->menu())
			{
				QString actionPath = path + " > " + action->menu()->title();
				menuPaths.insert(actionPath.remove('&'));
				addMenuPaths(menuPaths, action->menu(), actionPath);
			}
			else
			{
				QString leaf = action->text()
					.remove('&')
					.remove(" (requires Image Generator, Filter, or Transition protocol)")
					.remove(" (requires Image Generator protocol)")
					.trimmed();

				// Treat indented menu items as submenus.
				// E.g., what is actually `File > New Composition from Template > Blend Mode`
				// can be referenced as   `File > New Composition from Template > Export > FFGL > Blend Mode`.
				int indentationLevel = getIndentationLevel(action);
				while (indentationLevel--)
					for (int i = menu->actions().indexOf(action); i >= 0; --i)
						if (getIndentationLevel(menu->actions()[i]) <= indentationLevel)
						{
							leaf = menu->actions()[i]->text() + " > " + leaf;
							break;
						}

				menuPaths.insert(path.remove('&') + " > " + leaf);
			}
		}
	}

	/**
	 * Verifies that `\menu{…}` references in the manual actually exist in the editor GUI.
	 */
	void testManualMenuReferences()
	{
		cleanupTestCase();
		initTestCase();

		// VuoExampleMenu populates itself using dispatch_async; give it a chance to run that.
		VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
		VuoEventLoop_processEvent(VuoEventLoop_RunOnce);

		// Collect the editor's actual menu item paths/names.
		QSet<QString> allowedMenuPaths;
		foreach (QMenu *menu, window->menuBar()->findChildren<QMenu *>())
			addMenuPaths(allowedMenuPaths, menu, menu->title());

		// Non-Vuo menu items:
		allowedMenuPaths.insert("Desktop \\& Screen Saver > Screen Saver");
		allowedMenuPaths.insert("Draft");
		allowedMenuPaths.insert("File > Take Screenshot");
		allowedMenuPaths.insert("Go > Home");
		allowedMenuPaths.insert("Go > Library");
		allowedMenuPaths.insert("Language \\& Region > Apps");
		allowedMenuPaths.insert("Language \\& Region > General");
		allowedMenuPaths.insert("Render > Best");
		allowedMenuPaths.insert("Render > Normal");
		allowedMenuPaths.insert("Show Package Contents");
		allowedMenuPaths.insert("System Preferences > Language \\& Region");
		allowedMenuPaths.insert("Video > Vuo");

		// Vuo non-global menu items:
		allowedMenuPaths.insert("Add Input Port");
		allowedMenuPaths.insert("Delete Port");
		allowedMenuPaths.insert("Drop Events");
		allowedMenuPaths.insert("Edit Composition…");
		allowedMenuPaths.insert("Edit Details…");
		allowedMenuPaths.insert("Edit Shader…");
		allowedMenuPaths.insert("Edit Value…");
		allowedMenuPaths.insert("Edit…");
		allowedMenuPaths.insert("Enqueue Events");
		allowedMenuPaths.insert("Fire Event");
		allowedMenuPaths.insert("Generic");
		allowedMenuPaths.insert("Hide");
		allowedMenuPaths.insert("Insert Node");
		allowedMenuPaths.insert("Publish Port");
		allowedMenuPaths.insert("Publish");
		allowedMenuPaths.insert("Remove Input Port");
		allowedMenuPaths.insert("Rename Port…");
		allowedMenuPaths.insert("Set Data Type > Generic");
		allowedMenuPaths.insert("Set Data Type");
		allowedMenuPaths.insert("Set Event Throttling");
		allowedMenuPaths.insert("Show in Finder");

		// Vuo modal menu items:
		allowedMenuPaths.insert("File > Move to User Library");
		allowedMenuPaths.insert("Run > Hide Events");
		allowedMenuPaths.insert("View > Show/Hide Published Ports");
		allowedMenuPaths.insert("View > Show/Hide Toolbar Labels");

		// VuoCompositionLoader/VuoApp menu items:
		allowedMenuPaths.insert("File > Start Recording");
		allowedMenuPaths.insert("File > Stop Recording…");

		// vuo.org menu items:
		allowedMenuPaths.insert("Download");
		allowedMenuPaths.insert("Vuo SDK");

		QFile file(VUO_SOURCE_DIR "/documentation/VuoManual.txt");
		QVERIFY(file.open(QFile::ReadOnly));
		QTextStream stream(&file);
		QString line;
		QRegularExpression re("\\menu{(.*?)}");
		int lineNumber = 1;
		while (stream.readLineInto(&line))
		{
			QRegularExpressionMatchIterator i = re.globalMatch(line);
			while (i.hasNext())
			{
				QRegularExpressionMatch match = i.next();
				QString referencedMenuPath = match.captured(1);

				// Allow the manual to reference menu items by just the leaf, or by the full path.
				bool matchFound = false;
				foreach (QString actualMenuPath, allowedMenuPaths)
					if (actualMenuPath.endsWith(referencedMenuPath))
					{
						matchFound = true;
						break;
					}

				QVERIFY2(matchFound, QString("VuoManual.txt:%1: no such menu item: %2")
					.arg(lineNumber)
					.arg(referencedMenuPath).toUtf8().data());
			}
			++lineNumber;
		}
	}

	void testCableDragPerformance_data()
	{
		QTest::addColumn<QString>("port");

		QTest::newRow("nodes-200-cables-100-list-text.vuo")  << "GetItemFromList:item";
		QTest::newRow("nodes-1000-cables-2000.vuo")          << "GetRGBColorValues:red";
		QTest::newRow("nodes-1000-cables-500-list-text.vuo") << "GetItemFromList:item";
		QTest::newRow("nodes-1000-hold-real.vuo")            << "HoldValue:heldValue";
		QTest::newRow("nodes-1200-cables-1200-generic.vuo")  << "AddLists97:summedList";
		QTest::newRow("nodes-2000-cables-4000.vuo")          << "GetRGBColorValues:red";
	}
	void testCableDragPerformance()
	{
		QFETCH(QString, port);

		cleanupTestCase();
		initTestCaseWithCompositionFile(QTest::currentDataTag());

		VuoPort *p = composition->getPortWithStaticIdentifier(port.toStdString());
		QVERIFY(p);
		VuoRendererPort *rp = p->getRenderer();
		QVERIFY(rp);

		QGraphicsSceneMouseEvent e;
		QBENCHMARK {
			composition->initiateCableDrag(rp, nullptr, &e);
		}
	}

	void testProExampleCompositions_data()
	{
		QTest::addColumn<QString>("compositionFile");

		map<string, VuoNodeSet *> nodeSets = compiler->getNodeSets();
		for (map<string, VuoNodeSet *>::iterator i = nodeSets.begin(); i != nodeSets.end(); ++i)
		{
			vector<string> examples = i->second->getExampleCompositionFileNames();
			for (vector<string>::iterator j = examples.begin(); j != examples.end(); ++j)
				QTest::newRow((i->second->getName() + ":" + *j).c_str()) << ("../../node/" + i->second->getName() + "/examples/" + *j).c_str();
		}
	}
	/**
	 * Verifies that example compositions containing Pro nodes, and _only_ example compositions containing Pro nodes,
	 * are designated as Pro in the menu.
	 */
	void testProExampleCompositions()
	{
		QFETCH(QString, compositionFile);

		QString filename = QFileInfo(compositionFile).fileName();
		bool compositionListedAsPro = static_cast<VuoEditor *>(qApp)->menuOpenExample->isCompositionPro(filename);

		VuoCompilerGraphvizParser *graphParser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionFile.toStdString(), compiler);
		bool foundProNode = false;
		foreach (VuoNode *node, graphParser->getNodes())
			if (node->getNodeClass()->isPro())
			{
				foundProNode = true;
				QVERIFY2(compositionListedAsPro, QString("Example composition '%1' contains Pro node '%2', but isn't listed as a Pro example composition.")
					.arg(filename.toUtf8().data(), node->getNodeClass()->getClassName().c_str()).toUtf8().data());
			}

		if (compositionListedAsPro && !foundProNode)
			QFAIL(QString("Example composition '%1' is listed as a Pro example composition, but doesn't contain any Pro nodes.")
				.arg(filename).toUtf8().data());
	}

};


int main(int argc, char *argv[])
{
	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../QtPlugins").c_str()));

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	VuoEditor app(argc, argv);
	TestVuoEditor tc;

	// VuoModuleManager::loadedModules needs to run a block on the main thread.
	QTest::qWait(1000);

	return QTest::qExec(&tc, argc, argv);
}

#include "TestVuoEditor.moc"
