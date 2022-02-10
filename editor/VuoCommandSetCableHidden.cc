/**
 * @file
 * VuoCommandSetCableHidden implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetCableHidden.hh"
#include "VuoCompilerNode.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererCable.hh"

/**
 * Creates a command for setting the wireless (hidden) status of a cable.
 */
VuoCommandSetCableHidden::VuoCommandSetCableHidden(VuoRendererCable *cable, bool hidden, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Hide"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		cable->setWireless(hidden);
		cable->getBase()->getFromPort()->getRenderer()->updateGeometry();
		cable->getBase()->getToPort()->getRenderer()->updateGeometry();
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("%s cable %s:%s -> %s:%s",
		hidden ? "Hide" : "Unhide",
		cable->getBase()->getFromNode()->hasCompiler() ? cable->getBase()->getFromNode()->getCompiler()->getIdentifier().c_str() : "?",
		cable->getBase()->getFromPort()->getClass()->getName().c_str(),
		cable->getBase()->getToNode()->hasCompiler() ? cable->getBase()->getToNode()->getCompiler()->getIdentifier().c_str() : "?",
		cable->getBase()->getToPort()->getClass()->getName().c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetCableHidden::id() const
{
	return VuoCommandCommon::setCableHiddenCommandID;
}

/**
 * Sets the new "hidden" status of the cable.
 */
void VuoCommandSetCableHidden::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
}

/**
 * Restores the cable to its old "hidden" status.
 */
void VuoCommandSetCableHidden::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
}
