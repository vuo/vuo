/**
 * @file
 * VuoCommandSetPublishedPortDetails implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetPublishedPortDetails.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for setting the details (suggestedMin, suggestedMax, suggestedStep)
 * associated with a published port.
 */
VuoCommandSetPublishedPortDetails::VuoCommandSetPublishedPortDetails(VuoRendererPublishedPort *port,
																	 json_object *details,
																	 VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Published Port Details"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		static_cast<VuoCompilerPublishedPortClass *>(port->getBase()->getClass()->getCompiler())->updateDetails(details);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Set published port '%s' details to %s",
		port->getBase()->getClass()->getName().c_str(),
		json_object_to_json_string(details));
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetPublishedPortDetails::id() const
{
	return VuoCommandCommon::setPublishedPortDetailsCommandID;
}

/**
 * Restores the published port's old details.
 */
void VuoCommandSetPublishedPortDetails::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	window->getComposition()->updateCompositionsThatContainThisSubcomposition(revertedSnapshot);
}

/**
 * Assigns the published port its new details.
 */
void VuoCommandSetPublishedPortDetails::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	window->getComposition()->updateCompositionsThatContainThisSubcomposition(updatedSnapshot);
}
