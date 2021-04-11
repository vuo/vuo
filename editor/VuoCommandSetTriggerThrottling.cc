/**
 * @file
 * VuoCommandSetTriggerThrottling implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandSetTriggerThrottling.hh"
#include "VuoCommandCommon.hh"
#include "VuoCompilerPort.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for editing a trigger port's event-throttling mode.
 */
VuoCommandSetTriggerThrottling::VuoCommandSetTriggerThrottling(VuoPort *port,
												 enum VuoPortClass::EventThrottling eventThrottling,
												 VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Throttling"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		port->setEventThrottling(eventThrottling);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	VuoCompilerPort *cp = dynamic_cast<VuoCompilerPort *>(port->getCompiler());
	setDescription("Set throttling for %s to %s",
		cp ? cp->getIdentifier().c_str() : "?",
		eventThrottling == VuoPortClass::EventThrottling_Enqueue ? "enqueue" : "drop");
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetTriggerThrottling::id() const
{
	return VuoCommandCommon::setTriggerThrottlingCommandID;
}

/**
 * Reverts the port's event-throttling mode.
 */
void VuoCommandSetTriggerThrottling::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
	window->getComposition()->updatePortPopovers();
}

/**
 * Sets the port's event-throttling mode.
 */
void VuoCommandSetTriggerThrottling::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
	window->getComposition()->updatePortPopovers();
}
