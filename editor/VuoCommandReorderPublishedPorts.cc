/**
 * @file
 * VuoCommandReorderPublishedPorts implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandReorderPublishedPorts.hh"
#include "VuoCommandCommon.hh"
#include "VuoComposition.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for reordering a composition's published input or output ports.
 */
VuoCommandReorderPublishedPorts::VuoCommandReorderPublishedPorts(vector<VuoPublishedPort *> ports, bool isInput, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Reorder Published Ports"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();
	vector<string> portNames;

	// Start of command content.
	{
		vector<VuoPublishedPort *> existingPortList = (isInput? window->getComposition()->getBase()->getPublishedInputPorts() :
																window->getComposition()->getBase()->getPublishedOutputPorts());
		foreach (VuoPublishedPort *port, existingPortList)
			window->getComposition()->removePublishedPort(port, isInput, false);

		foreach (VuoPublishedPort *port, ports)
		{
			window->getComposition()->addPublishedPort(port, isInput, false);
			portNames.push_back(port->getClass()->getName());
		}
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Reorder published %s ports to %s",
		isInput ? "input" : "output",
		VuoStringUtilities::join(portNames, ", ").c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandReorderPublishedPorts::id() const
{
	return VuoCommandCommon::reorderPublishedPortsCommandID;
}

/**
 * Undoes the reordering of published ports.
 */
void VuoCommandReorderPublishedPorts::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Reorders the published ports.
 */
void VuoCommandReorderPublishedPorts::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}
