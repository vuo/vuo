/**
 * @file
 * VuoInputEditorSession implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorSession.hh"

#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoComposition.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoInputEditor.hh"
#include "VuoInputEditorManager.hh"
#include "VuoPublishedPortSidebar.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererPublishedPort.hh"

#include <json-c/json.h>

/**
 * Creates a session that is ready to @ref execute.
 */
VuoInputEditorSession::VuoInputEditorSession(VuoInputEditorManager *inputEditorManager, VuoEditorComposition *composition,
											 VuoPublishedPortSidebar *sidebar, QMainWindow *window)
{
	this->inputEditorManager = inputEditorManager;
	this->composition = composition;
	this->sidebar = sidebar;
	this->window = window;
	portWithOpenInputEditor = nullptr;
}

/**
 * Starts the session by displaying an input editor for @a port.
 * The session continues until the user has finished editing and tabbing through input editors.
 * When the session has ended, returns the edited input values as they were before and after the session.
 */
map<VuoRendererPort *, pair<string, string> > VuoInputEditorSession::execute(VuoRendererPort *port, bool forwardTabTraversal)
{
	// Show the initial input editor, plus any that are opened by tabbing from it.
	// Tab-cycled input editor executions are stacked.

	showInputEditor(port, forwardTabTraversal);

	// Now that we're back to the bottom of the stack, collect all the changes that were made to input values.
	// This is a temporary workaround required because of the stacking. It enables the caller to
	// merge the entire tab cycle's worth of editing into a single Undo command to prevent individual
	// Undo commands from being issued out of order.

	composition->setIgnoreApplicationStateChangeEvents(false);

	map<VuoRendererPort *, pair<string, string> > startAndFinalValues;
	for (auto i : startValueForPort)
		startAndFinalValues[i.first].first = i.second;
	for (auto i : finalValueForPort)
		startAndFinalValues[i.first].second = i.second;

	// Simulate a mouse release event, since the input editor might have stolen it.
	{
		QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
		releaseEvent.setScenePos(firstPortLeftCenterGlobal);
		releaseEvent.setButton(Qt::LeftButton);
		QApplication::sendEvent(this, &releaseEvent);

		QCoreApplication::processEvents();
	}

	return startAndFinalValues;
}

/**
 * Displays an input editor for @a port.
 */
void VuoInputEditorSession::showInputEditor(VuoRendererPort *port, bool forwardTabTraversal)
{
	VuoInputEditor *inputEditor = nullptr;
	VuoType *type = port->getDataType();

	string originalValueAsString;
	json_object *details = nullptr;

	VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(port);
	if (publishedPort)
	{
		VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( publishedPort->getBase()->getCompiler() );
		originalValueAsString = publishedInputPort->getInitialValue();

		bool isPublishedInput = !port->getInput();
		details = static_cast<VuoCompilerPublishedPort *>(publishedPort->getBase()->getCompiler())->getDetails(isPublishedInput);
	}
	else
	{
		originalValueAsString = port->getConstantAsString();
		VuoCompilerInputEventPortClass *portClass = dynamic_cast<VuoCompilerInputEventPortClass *>(port->getBase()->getClass()->getCompiler());
		if (portClass)
			details = portClass->getDataClass()->getDetails();
	}

	inputEditor = inputEditorManager->newInputEditor(type, details);
	if (! inputEditor)
		return;

	inputEditor->setParent(window);

	inputEditor->installEventFilter(this);

	QPoint portCenterGlobal;
	if (publishedPort)
	{
		portCenterGlobal = sidebar->getGlobalPosOfPublishedPort(publishedPort);
	}
	else
	{
		QPoint portCenterInScene = port->scenePos().toPoint();
		QPoint portCenterInView = composition->views()[0]->mapFromScene(portCenterInScene);
		portCenterGlobal = composition->views()[0]->mapToGlobal(portCenterInView);
	}
	QPoint portLeftCenterGlobal = portCenterGlobal - QPoint(port->getPortRect().width()/2., 0);

	if (startValueForPort.empty())
		firstPortLeftCenterGlobal = portLeftCenterGlobal;

	json_object *originalValue = json_tokener_parse(originalValueAsString.c_str());

	VuoRendererNode *parentNode = port->getRenderedParentNode();
	vector<pair<QString, json_object *> > portConstants;
	if (parentNode)
		portConstants = parentNode->getConstantPortValues();

	composition->disableNondetachedPortPopovers();

	if (!publishedPort)
		composition->enablePopoverForPort(port);

	portWithOpenInputEditor = port;

	connect(inputEditor, SIGNAL(valueChanged(json_object *)), this, SLOT(updateValueForEditedPort(json_object *)));

	// @todo: https://b33p.net/kosada/node/9091
	if (!publishedPort)
	{
		connect(inputEditor, &VuoInputEditor::tabbedBackwardPastFirstWidget, this, &VuoInputEditorSession::showPreviousInputEditor);
		connect(inputEditor, &VuoInputEditor::tabbedPastLastWidget, this, &VuoInputEditorSession::showNextInputEditor);
	}

	// Only the first input editor invoked for a given port within an editing session
	// gets to record the port's initial value.
	if (startValueForPort.find(port) == startValueForPort.end())
	{
		startValueForPort[port] = originalValueAsString;

		// Ignore application state change events delivered as a result of showing the input editor.
		// Otherwise, detached port popovers respond to the change in state and break the tab cycle.
		composition->setIgnoreApplicationStateChangeEvents(true);
	}

	if (!details || json_object_get_type(details) != json_type_object)
		details = json_object_new_object();

	// If we're opening an input editor for a list item, propagate range-related details from the list drawer's host port.
	VuoRendererInputListDrawer *listDrawer = dynamic_cast<VuoRendererInputListDrawer *>(port->getUnderlyingParentNode());
	if (listDrawer)
	{
		json_object *hostDetails = nullptr;
		VuoPort *hostPort = listDrawer->getRenderedHostPort();
		VuoCompilerInputEventPortClass *hostPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>(hostPort->getClass()->getCompiler());
		if (hostPortClass && hostPortClass->getDataClass())
			hostDetails = hostPortClass->getDataClass()->getDetails();

		if (hostDetails)
		{
			// "suggestedMin"
			json_object *suggestedMinValue = nullptr;
			if (json_object_object_get_ex(hostDetails, "suggestedMin", &suggestedMinValue))
				json_object_object_add(details, "suggestedMin", json_object_get(suggestedMinValue));

			// "suggestedMax"
			json_object *suggestedMaxValue = nullptr;
			if (json_object_object_get_ex(hostDetails, "suggestedMax", &suggestedMaxValue))
				json_object_object_add(details, "suggestedMax", json_object_get(suggestedMaxValue));

			// "suggestedStep"
			json_object *suggestedStepValue = nullptr;
			if (json_object_object_get_ex(hostDetails, "suggestedStep", &suggestedStepValue))
				json_object_object_add(details, "suggestedStep", json_object_get(suggestedStepValue));

			// Port details not propagated (future work?): default, auto, autoSupersedesDefault
		}
	}

	// Embed some interface settings in the port details object passed to the input editor.
	VuoEditor *editor = static_cast<VuoEditor *>(qApp);
	json_object_object_add(details, "isDark", json_object_new_boolean(editor->isInterfaceDark()));
	json_object_object_add(details, "forwardTabTraversal", json_object_new_boolean(forwardTabTraversal));

	map<QString, json_object *> portNameToConstantValueMap(portConstants.begin(), portConstants.end());
	json_object *newValue = inputEditor->show(portLeftCenterGlobal, originalValue, details, portNameToConstantValueMap);
	string newValueAsString = json_object_to_json_string_ext(newValue, JSON_C_TO_STRING_PLAIN);

	// Only the final input editor invoked (i.e., the first to finish executing) for a given port
	// within an editing session gets to record the port's final value.
	if (finalValueForPort.find(port) == finalValueForPort.end())
		finalValueForPort[port] = newValueAsString;

	disconnect(inputEditor, 0, this, 0);
	inputEditor->deleteLater();

	composition->disableNondetachedPortPopovers();
}

/**
 * Determines which port has most recently had an open input editor by consulting
 * @ref portWithOpenInputEditor; determines the previous eligible port belonging to the
 * same node, and displays its input editor.
 */
void VuoInputEditorSession::showPreviousInputEditor()
{
	// If we ever allow multiple input editors to be open at once, we will need
	// to do something more sophisticated to determine the reference port.  For now,
	// assume that it is the port that most recently had its input editor invoked.
	if (! portWithOpenInputEditor)
		return;

	VuoRendererPort *referencePort = portWithOpenInputEditor;
	QString portName = referencePort->getBase()->getClass()->getName().c_str();
	VuoRendererNode *parentNode = referencePort->getRenderedParentNode();

	// Exit the tab cycle if the user is tabbing away from a port whose constant value changes
	// trigger modifications to the composition structure.  These changes must be pushed onto the
	// 'Undo' stack at the same time as the constant value change, and that does not currently
	// happen until the input editor tab cycle exits.
	if (composition->requiresStructuralChangesAfterValueChangeAtPort(referencePort))
		return;

	vector<pair<QString, json_object *> > portConstants = parentNode->getConstantPortValues();

	// Filter out ports whose input editors don't emit @c tabbedPastLastWidget() and/or
	// @c tabbedBackwardPastFirstWidget() signals. These input editors break the tab cycle.
	vector<QString> portsWithEligibleInputEditors;
	for (vector<pair<QString, json_object *> >::iterator i = portConstants.begin(); i != portConstants.end(); ++i)
	{
		VuoRendererPort *currentPort = parentNode->getBase()->getInputPortWithName(i->first.toUtf8().constData())->getRenderer();
		json_object *currentDetails = nullptr;
		VuoCompilerInputEventPortClass *currentPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>(currentPort->getBase()->getClass()->getCompiler());
		if (currentPortClass)
			currentDetails = currentPortClass->getDataClass()->getDetails();
		VuoInputEditor *currentPortDataEditor = inputEditorManager->newInputEditor(currentPort->getDataType(), currentDetails);
		if (currentPortDataEditor)
		{
			if (currentPortDataEditor->supportsTabbingBetweenPorts())
				portsWithEligibleInputEditors.push_back(i->first);

			currentPortDataEditor->deleteLater();
		}
	}

	if (portsWithEligibleInputEditors.empty())
		return;

	QString previousPortName = "";
	bool referencePortJustTraversed = false;
	for (vector<QString>::reverse_iterator ri = portsWithEligibleInputEditors.rbegin(); ri != portsWithEligibleInputEditors.rend(); ++ri)
	{
		QString currentPortName = *ri;

		if (referencePortJustTraversed)
		{
			previousPortName = currentPortName;
			referencePortJustTraversed = false;
			break;
		}

		referencePortJustTraversed = (currentPortName == portName);
	}

	if (referencePortJustTraversed)
		previousPortName = portsWithEligibleInputEditors.back();

	VuoRendererPort *previousPort = parentNode->getBase()->getInputPortWithName(previousPortName.toUtf8().constData())->getRenderer();
	showInputEditor(previousPort, false);
}

/**
 * Determines which port has most recently had an open input editor by consulting
 * @ref portWithOpenInputEditor; determines the next eligible port belonging to the
 * same node, and displays its input editor.
 */
void VuoInputEditorSession::showNextInputEditor()
{
	// If we ever allow multiple input editors to be open at once, we will need
	// to do something more sophisticated to determine the reference port.  For now,
	// assume that it is the port that most recently had its input editor invoked.
	if (! portWithOpenInputEditor)
		return;

	VuoRendererPort *referencePort = portWithOpenInputEditor;
	QString portName = referencePort->getBase()->getClass()->getName().c_str();
	VuoRendererNode *parentNode = referencePort->getRenderedParentNode();

	// Exit the tab cycle if the user is tabbing away from a port whose constant value changes
	// trigger modifications to the composition structure.  These changes must be pushed onto the
	// 'Undo' stack at the same time as the constant value change, and that does not currently
	// happen until the input editor tab cycle exits.
	if (composition->requiresStructuralChangesAfterValueChangeAtPort(referencePort))
		return;

	vector<pair<QString, json_object *> > portConstants = parentNode->getConstantPortValues();

	// Filter out ports whose input editors don't emit @c tabbedPastLastWidget() and/or
	// @c tabbedBackwardPastFirstWidget() signals. These input editors break the tab cycle.
	vector<QString> portsWithEligibleInputEditors;
	for (vector<pair<QString, json_object *> >::iterator i = portConstants.begin(); i != portConstants.end(); ++i)
	{
		VuoRendererPort *currentPort = parentNode->getBase()->getInputPortWithName(i->first.toUtf8().constData())->getRenderer();
		json_object *currentDetails = nullptr;
		VuoCompilerInputEventPortClass *currentPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>(currentPort->getBase()->getClass()->getCompiler());
		if (currentPortClass)
			currentDetails = currentPortClass->getDataClass()->getDetails();
		VuoInputEditor *currentPortDataEditor = inputEditorManager->newInputEditor(currentPort->getDataType(), currentDetails);
		if (currentPortDataEditor)
		{
			if (currentPortDataEditor->supportsTabbingBetweenPorts())
				portsWithEligibleInputEditors.push_back(i->first);

			currentPortDataEditor->deleteLater();
		}
	}

	if (portsWithEligibleInputEditors.empty())
		return;

	QString nextPortName = "";
	bool referencePortJustTraversed = false;
	foreach (QString currentPortName, portsWithEligibleInputEditors)
	{
		if (referencePortJustTraversed)
		{
			nextPortName = currentPortName;
			referencePortJustTraversed = false;
			break;
		}

		referencePortJustTraversed = (currentPortName == portName);
	}

	if (referencePortJustTraversed)
		nextPortName = portsWithEligibleInputEditors.front();

	VuoRendererPort *nextPort = parentNode->getBase()->getInputPortWithName(nextPortName.toUtf8().constData())->getRenderer();
	showInputEditor(nextPort, true);
}

/**
 * Updates the current @ref portWithOpenInputEditor with the provided @a newValue
 * without pushing the update onto the Undo stack.
 */
void VuoInputEditorSession::updateValueForEditedPort(json_object *newValue)
{
	if (! portWithOpenInputEditor)
		return;

	// Disable real-time constant value updates while input editing is in progress if changes to this port's
	// constant value trigger modifications to the composition structure.  These changes must be pushed onto the
	// 'Undo' stack at the same time as the constant value change, and that does not currently
	// happen until the input editor tab cycle exits.
	if (composition->requiresStructuralChangesAfterValueChangeAtPort(portWithOpenInputEditor))
		return;

	string newValueAsString = json_object_to_json_string_ext(newValue, JSON_C_TO_STRING_PLAIN);
	if (newValueAsString != portWithOpenInputEditor->getConstantAsString())
	{
		VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(portWithOpenInputEditor->getBase()->getCompiler());
		composition->updatePortConstant(compilerPort, newValueAsString);
	}
}

/**
 * Filters events on input editors.
 */
bool VuoInputEditorSession::eventFilter(QObject *object, QEvent *event)
{
	// Filter drag-and-drop-related events destined for VuoText input editors:
	// Modify dropped URLs so that (in the absence of keyboard modifiers) the paths are relative to
	// the composition's storage directory; pass the modified drop event back to the input editor.
	if (object->metaObject() && !strcmp(object->metaObject()->className(), "VuoInputEditorText"))
	{
		if (event->type() == QEvent::Drop)
		{
			QDropEvent *filteredDropEvent = static_cast<QDropEvent *>(event);

			// Retrieve the composition directory so that file paths may be specified relative to it.
			// Note: Providing the directory's canonical path as the argument to the
			// QDir constructor is necessary in order for QDir::relativeFilePath() to
			// work correctly when the non-canonical path contains symbolic links (e.g.,
			// '/tmp' -> '/private/tmp' for example compositions).
			QDir compositionDir(QDir(composition->getBase()->getDirectory().c_str()).canonicalPath());

			// Use the absolute file path if the "Option" key was pressed.
			bool useAbsoluteFilePaths = VuoEditorUtilities::optionKeyPressedForEvent(event);

			const QMimeData *mimeData = filteredDropEvent->mimeData();
			QList<QUrl> urls = mimeData->urls();
			if (urls.size() == 1)
			{
				QString filePath = (useAbsoluteFilePaths? urls[0].path() : compositionDir.relativeFilePath(urls[0].path()));

				QList<QUrl> modifiedUrls;
				modifiedUrls.append(filePath);
				QMimeData *modifiedMimeData = new QMimeData();
				modifiedMimeData->setUrls(modifiedUrls);

				filteredDropEvent->accept();

				QDropEvent *modifiedDropEvent = new QDropEvent(
							filteredDropEvent->pos(),
							filteredDropEvent->possibleActions(),
							modifiedMimeData,
							filteredDropEvent->mouseButtons(),
							filteredDropEvent->keyboardModifiers(),
							filteredDropEvent->type());

				object->removeEventFilter(this);
				QApplication::sendEvent(object, modifiedDropEvent);
				object->installEventFilter(this);
			}
			else
				event->ignore();

			return true;
		}

		else if (event->type() == QEvent::DragEnter)
		{
			QDropEvent *filteredDragEnterEvent = static_cast<QDragEnterEvent *>(event);
			const QMimeData *mimeData = filteredDragEnterEvent->mimeData();

			// Accept drags of single files.
			if (mimeData->hasFormat("text/uri-list"))
			{
				QList<QUrl> urls = mimeData->urls();
				if (urls.size() == 1)
					event->accept();
				else
					filteredDragEnterEvent->setDropAction(Qt::IgnoreAction);
			}
			else
				filteredDragEnterEvent->setDropAction(Qt::IgnoreAction);

			return true;
		}

		else if (event->type() == QEvent::DragMove)
		{
			event->accept();
			return true;
		}

		else if (event->type() == QEvent::DragLeave)
		{
			(static_cast<QDragLeaveEvent *>(event))->accept();
			return true;
		}
	}

	return QObject::eventFilter(object, event);
}
