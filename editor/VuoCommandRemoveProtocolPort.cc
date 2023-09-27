/**
 * @file
 * VuoCommandRemoveProtocolPort implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandRemoveProtocolPort.hh"
#include "VuoCommandCommon.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererPublishedPort.hh"

/**
 * Creates a command for removing a published protocol port from a composition.
 */
VuoCommandRemoveProtocolPort::VuoCommandRemoveProtocolPort(VuoRendererPublishedPort *publishedPort, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Remove Protocol Port"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		bool isPublishedInput = !publishedPort->getInput();
		window->getComposition()->removePublishedPort(dynamic_cast<VuoPublishedPort *>(publishedPort->getBase()), isPublishedInput);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Remove protocol port %s", publishedPort->getBase()->getClass()->getName().c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandRemoveProtocolPort::id() const
{
	return VuoCommandCommon::removeProtocolPortCommandID;
}

/**
 * Undoes removal of the protocol port.
 */
void VuoCommandRemoveProtocolPort::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Removes the protocol port.
 */
void VuoCommandRemoveProtocolPort::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}
