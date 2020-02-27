/**
 * @file
 * VuoCommandPublishPort implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandPublishPort.hh"
#include "VuoCommandCommon.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for publishing a port.
 */
VuoCommandPublishPort::VuoCommandPublishPort(VuoPort *port,
											 VuoRendererCable *displacedCable,
											 VuoEditorWindow *window,
											 bool forceEventOnlyPublication,
											 string publishedPortName,
											 bool attemptMerge)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Publish Port"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	VuoPublishedPort *externalPort = NULL;
	{
		// Take inventory of required changes.
		pair<VuoPort *, VuoPublishedPort *> unpublishedPortCombination;
		map<VuoRendererCable *, VuoPort *> updatedFromPortForCable;
		map<VuoRendererCable *, VuoPort *> updatedToPortForCable;

		VuoPort *internalPort = port;
		bool displacedCableWasPublishedCable = (displacedCable && displacedCable->getBase()->isPublished());

		if (displacedCable)
		{
			if (displacedCableWasPublishedCable)
			{
				bool inputCable = displacedCable->getBase()->isPublishedInputCable();
				VuoPort *toPort = displacedCable->getBase()->getToPort();
				VuoPort *fromPort = displacedCable->getBase()->getFromPort();

				VuoPort *internalPublishedPort = (inputCable? toPort : fromPort);
				VuoPublishedPort *externalPublishedPort = (inputCable?
															   dynamic_cast<VuoPublishedPort *>(fromPort) :
															   dynamic_cast<VuoPublishedPort *>(toPort));

				unpublishedPortCombination = make_pair(internalPublishedPort, externalPublishedPort);
			}
			else
			{
				updatedFromPortForCable[displacedCable] = NULL;
				updatedToPortForCable[displacedCable] = NULL;
			}
		}

		// Now apply required changes.
		internalPort->getRenderer()->updateGeometry();

		// Disconnect the displaced cable, if applicable.
		if (displacedCable)
		{
			if (displacedCableWasPublishedCable)
			{
				bool unpublishIsolatedExternalPort = false;
				VuoCommandCommon::unpublishInternalExternalPortCombination(unpublishedPortCombination.first, unpublishedPortCombination.second, window->getComposition(), unpublishIsolatedExternalPort);
			}
			else
				VuoCommandCommon::updateCable(displacedCable, updatedFromPortForCable[displacedCable], updatedToPortForCable[displacedCable], window->getComposition());
		}

		// Case: Publishing the port for the first time.
		if (! externalPort)
			externalPort = VuoCommandCommon::publishInternalPort(internalPort, forceEventOnlyPublication, publishedPortName, window->getComposition(), attemptMerge);

		// Case: Re-doing port publication.
		else
			VuoCommandCommon::publishInternalExternalPortCombination(internalPort, externalPort, forceEventOnlyPublication, window->getComposition());

		// Collapse any typecasts possible.
		window->getComposition()->collapseTypecastNodes();
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Publish %s port '%s'",
		port->getRenderer()->getInput() ? "input" : "output",
		externalPort->getClass()->getName().c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandPublishPort::id() const
{
	return VuoCommandCommon::publishPortCommandID;
}

/**
 * Undoes publication of the port.
 */
void VuoCommandPublishPort::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Publishes the port.
 */
void VuoCommandPublishPort::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}

