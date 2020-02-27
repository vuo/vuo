/**
 * @file
 * VuoCommandSetPortConstant implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetPortConstant.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoEditorComposition.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for setting the constant value of a port.
 */
VuoCommandSetPortConstant::VuoCommandSetPortConstant(VuoCompilerPort *port, string constant, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Port Constant"));
	this->window = window;
	this->portIsPublished = dynamic_cast<VuoCompilerPublishedPort *>(port);
	this->portID = (portIsPublished? dynamic_cast<VuoCompilerPublishedPort *>(port)->getBase()->getClass()->getName() :
									 window->getComposition()->getIdentifierForStaticPort(port->getBase()));

	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		window->getComposition()->updatePortConstant(port, constant, false);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Set port %s to %s",
		port->getIdentifier().c_str(),
		constant.c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetPortConstant::id() const
{
	return VuoCommandCommon::setPortConstantCommandID;
}

/**
 * Restores the port to its old constant value.
 */
void VuoCommandSetPortConstant::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (portIsPublished)
	{
		window->coalescePublishedPortConstantsToSync(portID);
		window->getComposition()->updateCompositionsThatContainThisSubcomposition(revertedSnapshot);
	}
	else
		window->coalesceInternalPortConstantsToSync(portID);
}

/**
 * Sets the port to its new constant value.
 */
void VuoCommandSetPortConstant::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	if (portIsPublished)
	{
		window->coalescePublishedPortConstantsToSync(portID);
		window->getComposition()->updateCompositionsThatContainThisSubcomposition(updatedSnapshot);
	}
	else
		window->coalesceInternalPortConstantsToSync(portID);
}
