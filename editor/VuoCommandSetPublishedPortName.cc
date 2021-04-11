/**
 * @file
 * VuoCommandSetPublishedPortName implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetPublishedPortName.hh"
#include "VuoCompilerPort.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for setting the name of a published port.
 */
VuoCommandSetPublishedPortName::VuoCommandSetPublishedPortName(VuoRendererPublishedPort *port, string name, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Published Port Name"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	string oldPortName = port->getBase()->getClass()->getName();

	// Start of command content.
	{
		window->getComposition()->setPublishedPortName(port, name);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Rename published port %s to %s",
		oldPortName.c_str(),
		name.c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetPublishedPortName::id() const
{
	return VuoCommandCommon::setPublishedPortNameCommandID;
}

/**
 * Restores the published port's old name.
 */
void VuoCommandSetPublishedPortName::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);

	setText(QApplication::translate("VuoEditorWindow", "Set Published Port Name"));
}

/**
 * Assigns the published port its new name.
 */
void VuoCommandSetPublishedPortName::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);

	setText(QApplication::translate("VuoEditorWindow", "Set Published Port Name"));
}
