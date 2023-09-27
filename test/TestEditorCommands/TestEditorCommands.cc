/**
 * @file
 * TestEditorCommands implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>
#include "VuoEditorWindow.hh"
#include "VuoRendererComment.hh"
#include "VuoCommandAdd.hh"
#include "VuoCommandSetPortConstant.hh"
#include "VuoEditor.hh"

/// The composition starts out with a `Fire on Start` node, its hidden Refresh port, and its trigger port, so add 3.
#define VERIFY_ITEM_COUNT(n)                                               \
    if (editorWindow->getComposition()->items().count() != n + 3)          \
    {                                                                      \
        dump();                                                            \
        QFAIL(QString("Expected %1 (%2 + 3) items but found %3 (%4 + 3).") \
              .arg(n + 3)                                                  \
              .arg(n)                                                      \
              .arg(editorWindow->getComposition()->items().count())        \
              .arg(editorWindow->getComposition()->items().count() - 3)    \
              .toUtf8().data());                                           \
    }

/**
 * Tests for the VuoCommand* classes.
 */
class TestEditorCommands : public QObject
{
	Q_OBJECT

	VuoEditorWindow *editorWindow;

	void dump()
	{
		foreach (QGraphicsItem *i, editorWindow->getComposition()->items())
			VUserLog("%s(%p)", typeid(*i).name(), i);
	}

	VuoNode *getNodeByClassName(string className)
	{
		auto nodes = editorWindow->getComposition()->getBase()->getNodes();
		auto node  = std::find_if(nodes.begin(), nodes.end(), [className](VuoNode *i) {
			return i->getNodeClass()->getModuleKey() == className;
		});
		if (node == nodes.end())
			throw std::runtime_error(QString("Couldn't find node \"%1\".").arg(QString::fromStdString(className)).toUtf8().data());
		return *node;
	}

public:
	TestEditorCommands()
	{
		editorWindow = new VuoEditorWindow();
		QVERIFY(editorWindow);
	}
	~TestEditorCommands()
	{
		delete editorWindow;
	}

private slots:
	void testVuoCommandAdd_data()
	{
		QTest::addColumn<QList<QGraphicsItem *>>("itemsToAdd");
		QTest::addColumn<int>("expectedItemCount");

		QTest::newRow("node: vuo.data.share") << QList<QGraphicsItem *>{
			editorWindow->getComposition()->createNode("vuo.data.share")
		} << 4;

		// Adding `Calculate` also adds `Make Dictionary` and its 2 drawers.
		QTest::newRow("node: vuo.math.calculate") << QList<QGraphicsItem *>{
			editorWindow->getComposition()->createNode("vuo.math.calculate")
		} << 23;

		{
			auto fromNode = editorWindow->getComposition()->createNode("vuo.data.share");
			auto toNode = editorWindow->getComposition()->createNode("vuo.data.share");

			QTest::newRow("cable") << QList<QGraphicsItem *>{
				fromNode,
				toNode,
				new VuoRendererCable((new VuoCompilerCable(
					fromNode->getBase()->getCompiler(),
					static_cast<VuoCompilerPort *>(fromNode->getBase()->getOutputPortWithName("sameValue")->getCompiler()),
					toNode->getBase()->getCompiler(),
					static_cast<VuoCompilerPort *>(toNode->getBase()->getInputPortWithName("value")->getCompiler()))
				)->getBase())
			} << 9;
		}

		QTest::newRow("comment") << QList<QGraphicsItem *>{
			editorWindow->getComposition()->createRendererComment((new VuoCompilerComment(new VuoComment()))->getBase())
		} << 2;
	}
	void testVuoCommandAdd()
	{
		QFETCH(QList<QGraphicsItem *>, itemsToAdd);
		QFETCH(int, expectedItemCount);

		VERIFY_ITEM_COUNT(0);

		QUndoCommand *command = new VuoCommandAdd(itemsToAdd, editorWindow);
		QVERIFY(command);

		// Create the items.
		editorWindow->undoStack->push(command);
		QVERIFY(editorWindow->undoStack->canUndo());
		VERIFY_ITEM_COUNT(expectedItemCount);

		// Undo, to remove the items.
		editorWindow->undoStack->undo();
		VERIFY_ITEM_COUNT(0);
		QVERIFY(editorWindow->undoStack->canRedo());

		// Redo, to recreate the items.
		editorWindow->undoStack->redo();
		QVERIFY(editorWindow->undoStack->canUndo());
		VERIFY_ITEM_COUNT(expectedItemCount);

		// Undo, to reremove the items.
		editorWindow->undoStack->undo();
		VERIFY_ITEM_COUNT(0);
	}

	/// @todo VuoCommandAddPublishedPort
	/// @todo VuoCommandChangeNode
	/// @todo VuoCommandConnect
	/// @todo VuoCommandMove
	/// @todo VuoCommandPublishPort
	/// @todo VuoCommandRemove
	/// @todo VuoCommandRemoveProtocolPort
	/// @todo VuoCommandReorderPublishedPorts

	/**
	 * adjustInputPortCountForNode calls VuoCommandReplaceNode.
	 */
	void testVuoCommandReplaceNode_adjustInputPortCountForNode()
	{
		VERIFY_ITEM_COUNT(0);

		editorWindow->on_runComposition_triggered();
		auto composition = editorWindow->getComposition();
		QVERIFY(composition->isRunning());

		auto makeListNode = composition->createNode("vuo.list.make.1.VuoInteger");
		QUndoCommand *command = new VuoCommandAdd(QList<QGraphicsItem *>{ makeListNode }, editorWindow);
		QVERIFY(command);

		// Create a list with 1 item.
		{
			editorWindow->undoStack->push(command);
			QVERIFY(editorWindow->undoStack->canUndo());
			VERIFY_ITEM_COUNT(4);
		}

		// Give the list item a runtime (as opposed to constant) value.
		{
			dispatch_sync(composition->runCompositionQueue, ^{});  // Wait for the above reload to complete.
			composition->runner->setInputPortValue("", static_cast<VuoCompilerPort *>(makeListNode->getBase()->getInputPortWithName("1")->getCompiler())->getIdentifier(), json_object_new_int(42));
		}

		// Add a second item to the list.
		{
			editorWindow->adjustInputPortCountForNode(makeListNode, 1, false);
			VERIFY_ITEM_COUNT(5);
		}

		// Find the newly-replaced list node.
		VuoRendererNode *newMakeListNode = nullptr;
		{
			for (auto i : composition->getBase()->getNodes())
				if (i->getNodeClass()->getClassName() == "vuo.list.make.2.VuoInteger")
					newMakeListNode = i->getRenderer();
			QVERIFY(newMakeListNode);
		}

		// Ensure the first port's runtime value was carried over.
		// https://b33p.net/kosada/vuo/vuo/-/issues/17903
		{
			json_object *o = composition->getPortValueInRunningComposition(newMakeListNode->getBase()->getInputPortWithName("1"));
			QVERIFY(o);
			QCOMPARE(json_object_get_int(o), 42);
		}

		// Remove both items from the list.
		{
			editorWindow->adjustInputPortCountForNode(newMakeListNode, -2, false);
			VERIFY_ITEM_COUNT(3);
		}

		// Undo, to go back to a list with 2 items.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(5);
		}

		// Undo, to go back to a list with 1 item.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(4);
		}

		// Undo, to remove the list.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(0);
		}

		editorWindow->on_stopComposition_triggered();
		QVERIFY(!composition->isRunning());
	}

	/**
	 * Ensure that resizing an unspecialized drawer doesn't crash.
	 *
	 * https://b33p.net/kosada/vuo/vuo/-/issues/19823
	 */
	void testVuoCommandReplaceNode_adjustInputPortCountForNode_generic()
	{
		VERIFY_ITEM_COUNT(0);

		auto composition = editorWindow->getComposition();
		auto multiplyListsNode = composition->createNode("vuo.math.multiply.list.2");
		QUndoCommand *command = new VuoCommandAdd(QList<QGraphicsItem *>{ multiplyListsNode }, editorWindow);
		QVERIFY(command);

		// Create the node, including drawers for its list1 and list2 input ports.
		{
			editorWindow->undoStack->push(command);
			QVERIFY(editorWindow->undoStack->canUndo());
			VERIFY_ITEM_COUNT(17);
		}

		// Add a third item to list1.
		{
			auto drawers = multiplyListsNode->getAttachedInputDrawers();
			QCOMPARE(drawers.size(), 2);

			auto list1MakeListNode = multiplyListsNode->getAttachedInputDrawers()[0];
			editorWindow->adjustInputPortCountForNode(list1MakeListNode, 1, false);
			VERIFY_ITEM_COUNT(18);
		}
	}

	/**
	 * specializePortNetwork and unspecializePortNetwork call VuoCommandReplaceNode.
	 */
	void testVuoCommandReplaceNode_specializePortNetwork()
	{
		VERIFY_ITEM_COUNT(0);

		editorWindow->on_runComposition_triggered();
		auto composition = editorWindow->getComposition();
		QVERIFY(composition->isRunning());

		auto fromNode = editorWindow->getComposition()->createNode("vuo.data.share");
		auto toNode = editorWindow->getComposition()->createNode("vuo.data.share");
		QUndoCommand *command = new VuoCommandAdd(QList<QGraphicsItem *>{
			fromNode,
			toNode,
			new VuoRendererCable((new VuoCompilerCable(
				fromNode->getBase()->getCompiler(),
				static_cast<VuoCompilerPort *>(fromNode->getBase()->getOutputPortWithName("sameValue")->getCompiler()),
				toNode->getBase()->getCompiler(),
				static_cast<VuoCompilerPort *>(toNode->getBase()->getInputPortWithName("value")->getCompiler()))
			)->getBase())
		}, editorWindow);
		QVERIFY(command);

		// Create a simple unspecialized network.
		{
			editorWindow->undoStack->push(command);
			QVERIFY(editorWindow->undoStack->canUndo());
			VERIFY_ITEM_COUNT(9);
		}

		// Specialize the network to VuoPoint4d.
		VuoRendererNode *newFromNode = nullptr;
		{
			newFromNode = editorWindow->specializePortNetwork(fromNode->getBase()->getInputPortWithName("value")->getRenderer(), "VuoPoint4d");
			VERIFY_ITEM_COUNT(9);
			QVERIFY(newFromNode);
		}

		// Unspecialize the network.
		VuoRendererNode *newNewFromNode = nullptr;
		{
			newNewFromNode = editorWindow->unspecializePortNetwork(newFromNode->getBase()->getInputPortWithName("value")->getRenderer());
			VERIFY_ITEM_COUNT(9);
			QVERIFY(newNewFromNode);
		}

		// Undo, to revert the network to specialized-to-VuoPoint4d.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(9);
		}

		// Undo, to revert the network to unspecialized.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(9);
		}

		// Undo, to remove the unspecialized network.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(0);
		}

		editorWindow->on_stopComposition_triggered();
		QVERIFY(!composition->isRunning());
	}

	/**
	 * showInputEditor calls VuoCommandReplaceNode when changing the `Calculate` node's expression.
	 */
	void testVuoCommandReplaceNode_showInputEditor()
	{
		VERIFY_ITEM_COUNT(0);

		editorWindow->on_runComposition_triggered();
		auto composition = editorWindow->getComposition();
		QVERIFY(composition->isRunning());

		auto calculateNode = composition->createNode("vuo.math.calculate");
		auto expressionPort = calculateNode->getBase()->getInputPortWithName("expression");
		QUndoCommand *command = new VuoCommandAdd(QList<QGraphicsItem *>{ calculateNode }, editorWindow);
		QVERIFY(command);

		// Create `Calculate` and its bucket of parts for its default expression `A + B`.
		{
			editorWindow->undoStack->push(command);
			QVERIFY(editorWindow->undoStack->canUndo());
			VERIFY_ITEM_COUNT(23);

			QVERIFY(composition->requiresStructuralChangesAfterValueChangeAtPort(expressionPort->getRenderer()));
		}

		// Verify the default expression.
		{
			// Wait for the composition to reload.
			QVERIFY(composition->isRunning());

			VuoNode *keyList = getNodeByClassName("vuo.list.make.2.VuoText");
			QVERIFY(keyList);

			VuoPort *firstPort = keyList->getInputPortWithName("1");
			QVERIFY(firstPort);
			json_object *firstKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(firstPort->getCompiler())->getIdentifier());
			QCOMPARE(json_object_get_string(firstKey), "A");

			VuoPort *secondPort = keyList->getInputPortWithName("2");
			QVERIFY(secondPort);
			json_object *secondKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(secondPort->getCompiler())->getIdentifier());
			QCOMPARE(json_object_get_string(secondKey), "B");
		}

		// Initialize the input values
		{
			VuoNode *valueList = getNodeByClassName("vuo.list.make.2.VuoReal");
			QVERIFY(valueList);

			VuoPort *firstPort = valueList->getInputPortWithName("1");
			QVERIFY(firstPort);
			composition->updatePortConstant(static_cast<VuoCompilerPort *>(firstPort->getCompiler()), "42", true);

			VuoPort *secondPort = valueList->getInputPortWithName("2");
			QVERIFY(secondPort);
			composition->updatePortConstant(static_cast<VuoCompilerPort *>(secondPort->getCompiler()), "22", true);
		}

		// Verify eventless transmission from the list input ports to the Calculate node's dictionary input port.
		{
			// Wait for the async updates to apply.
			QVERIFY(composition->isRunning());

			VuoPort *dictionaryPort = calculateNode->getBase()->getInputPortWithName("values");
			QVERIFY(dictionaryPort);
			json_object *dictionary = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(dictionaryPort->getCompiler())->getIdentifier());
			json_object *dictionaryValues;
			json_object_object_get_ex(dictionary, "values", &dictionaryValues);
			QCOMPARE(json_object_get_int(json_object_array_get_idx(dictionaryValues, 0)), 42);
			QCOMPARE(json_object_get_int(json_object_array_get_idx(dictionaryValues, 1)), 22);
		}

		// Change the expression and update the bucket of parts.
		{
			editorWindow->undoStack->beginMacro("Change Calculate expression");
			string originalEditingSessionValue = VUO_STRINGIFY({"expressions":["A + B"],"inputVariables":["A","B"],"outputVariables":["result"]});
			// `inputVariables` is sorted, so AA appears before B.
			string    finalEditingSessionValue = VUO_STRINGIFY({"expressions":["A + B + AA"],"inputVariables":["A","AA","B"],"outputVariables":["result"]});
			editorWindow->undoStack->push(new VuoCommandSetPortConstant(
				static_cast<VuoCompilerPort *>(expressionPort->getCompiler()),
				finalEditingSessionValue,
				editorWindow));

			composition->performStructuralChangesAfterValueChangeAtPort(editorWindow, editorWindow->undoStack, expressionPort->getRenderer(), originalEditingSessionValue, finalEditingSessionValue);
			editorWindow->undoStack->endMacro();
			VERIFY_ITEM_COUNT(25);
		}

		// Verify that the running composition's list inputs were correctly adapted.
		{
			// Wait for the composition to reload.
			QVERIFY(composition->isRunning());

			VuoNode *keyList = getNodeByClassName("vuo.list.make.3.VuoText");
			QVERIFY(keyList);
			{
				VuoPort *firstPort = keyList->getInputPortWithName("1");
				QVERIFY(firstPort);
				json_object *firstKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(firstPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_string(firstKey), "A");

				// Key "B" was formerly 2nd, but "AA" pushed it back to 3rd.
				VuoPort *secondPort = keyList->getInputPortWithName("2");
				QVERIFY(secondPort);
				json_object *secondKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(secondPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_string(secondKey), "AA");

				VuoPort *thirdPort = keyList->getInputPortWithName("3");
				QVERIFY(thirdPort);
				json_object *thirdKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(thirdPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_string(thirdKey), "B");
			}

			VuoNode *valueList = getNodeByClassName("vuo.list.make.3.VuoReal");
			QVERIFY(valueList);
			{
				VuoPort *firstPort = valueList->getInputPortWithName("1");
				QVERIFY(firstPort);
				json_object *firstKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(firstPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_int(firstKey), 42);

				// The value for "B" should move to the 3rd port, since the key "B" moved to the 3rd port.
				VuoPort *secondPort = valueList->getInputPortWithName("2");
				QVERIFY(secondPort);
				json_object *secondKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(secondPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_int(secondKey), 0);

				VuoPort *thirdPort = valueList->getInputPortWithName("3");
				QVERIFY(thirdPort);
				json_object *thirdKey = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(thirdPort->getCompiler())->getIdentifier());
				QCOMPARE(json_object_get_int(thirdKey), 22);
			}

		}

		// Verify eventless transmission from the list input ports to the Calculate node's dictionary input port.
		{
			VuoPort *dictionaryPort = calculateNode->getBase()->getInputPortWithName("values");
			QVERIFY(dictionaryPort);
			json_object *dictionary = composition->runner->getInputPortValue("", static_cast<VuoCompilerPort *>(dictionaryPort->getCompiler())->getIdentifier());

			json_object *dictionaryValues;
			json_object_object_get_ex(dictionary, "values", &dictionaryValues);
			QCOMPARE(json_object_get_int(json_object_array_get_idx(dictionaryValues, 0)), 42);
			QCOMPARE(json_object_get_int(json_object_array_get_idx(dictionaryValues, 1)), 0);
			QCOMPARE(json_object_get_int(json_object_array_get_idx(dictionaryValues, 2)), 22);
		}

		// Undo, to revert to the original expression.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(23);
		}

		// Undo, to remove the `Calculate` node.
		{
			editorWindow->undoStack->undo();
			VERIFY_ITEM_COUNT(0);
		}

		editorWindow->on_stopComposition_triggered();
		QVERIFY(!composition->isRunning());
	}

	/// @todo VuoCommandResizeComment
	/// @todo VuoCommandSetCableHidden
	/// @todo VuoCommandSetCommentText
	/// @todo VuoCommandSetItemTint
	/// @todo VuoCommandSetMetadata
	/// @todo VuoCommandSetNodeTitle
	/// @todo VuoCommandSetPortConstant
	/// @todo VuoCommandSetPublishedPortDetails
	/// @todo VuoCommandSetPublishedPortName
	/// @todo VuoCommandSetTriggerThrottling
	/// @todo VuoCommandUnpublishPort
};

int main(int argc, char *argv[])
{
	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	// https://bugreports.qt.io/browse/QTBUG-81370
	qputenv("QT_MAC_WANTS_LAYER", "1");

	VuoEditor app(argc, argv);
	TestEditorCommands t;

	// VuoModuleManager::loadedModules needs to run a block on the main thread.
	QTest::qWait(10000);

	return QTest::qExec(&t, argc, argv);
}

#include "TestEditorCommands.moc"
