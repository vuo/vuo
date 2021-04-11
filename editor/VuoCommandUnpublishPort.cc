/**
 * @file
 * VuoCommandUnpublishPort implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandUnpublishPort.hh"

#include "VuoCable.hh"
#include "VuoCommandCommon.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoPublishedPort.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoRendererPort.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for unpublishing an externally visible published @c port.
 */
VuoCommandUnpublishPort::VuoCommandUnpublishPort(VuoPublishedPort *externalPort, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	VuoPublishedPort *explicitExternalPort = externalPort;
	bool isPublishedInput = !externalPort->getRenderer()->getInput();

	vector<pair<VuoPort *, VuoPublishedPort *> > internalExternalPortCombinations;
	foreach (VuoCable *cable, externalPort->getConnectedCables(true))
	{
		VuoPort *internalPort = (isPublishedInput? cable->getToPort() : cable->getFromPort());
		internalExternalPortCombinations.push_back(make_pair(internalPort, externalPort));
	}

	initialize(explicitExternalPort, isPublishedInput, internalExternalPortCombinations, window);
}

/**
 * Creates a command for unpublishing an internal @c port connected to an externally visible published port.
 */
VuoCommandUnpublishPort::VuoCommandUnpublishPort(VuoPort *internalPort, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	VuoPublishedPort *explicitExternalPort = NULL;
	bool isPublishedInput = internalPort->getRenderer()->getInput();

	vector<pair<VuoPort *, VuoPublishedPort *> > internalExternalPortCombinations;
	foreach (VuoRendererPublishedPort *externalPort, internalPort->getRenderer()->getPublishedPorts())
		internalExternalPortCombinations.push_back(make_pair(internalPort, dynamic_cast<VuoPublishedPort *>(externalPort->getBase())));

	initialize(explicitExternalPort, isPublishedInput, internalExternalPortCombinations, window);
}

/**
 * Helper function for VuoCommandUnpublishPort constructors.
 */
void VuoCommandUnpublishPort::initialize(VuoPublishedPort *explicitExternalPort,
									bool isPublishedInput,
									vector<pair<VuoPort *, VuoPublishedPort *> > internalExternalPortCombinations,
									VuoEditorWindow *window)
{
	setText(QApplication::translate("VuoEditorWindow", "Delete"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	vector<string> portNames;
	{
		if (explicitExternalPort && explicitExternalPort->getConnectedCables(true).empty() &&
				!explicitExternalPort->isProtocolPort())
		{
			window->getComposition()->removePublishedPort(explicitExternalPort, isPublishedInput);
			portNames.push_back(explicitExternalPort->getClass()->getName());
		}

		for (vector<pair<VuoPort *, VuoPublishedPort *> >::iterator i = internalExternalPortCombinations.begin(); i != internalExternalPortCombinations.end(); ++i)
		{
			bool unpublishIsolatedExternalPort = !(*i).second->isProtocolPort();
			VuoCommandCommon::unpublishInternalExternalPortCombination((*i).first, (*i).second, window->getComposition(), unpublishIsolatedExternalPort);
			portNames.push_back((*i).second->getClass()->getName());
		}
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Unpublish %s port '%s'",
		isPublishedInput ? "input" : "output",
		VuoStringUtilities::join(portNames, ", ").c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandUnpublishPort::id() const
{
	return VuoCommandCommon::unpublishPortCommandID;
}

/**
 * Undoes unpublication of the port.
 */
void VuoCommandUnpublishPort::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Unpublishes the port.
 */
void VuoCommandUnpublishPort::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}
