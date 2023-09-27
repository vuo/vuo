/**
 * @file
 * VuoCommandSetMetadata implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetMetadata.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoCompositionMetadataPanel.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindow.hh"
#include "VuoEditor.hh"
#include "VuoCompiler.hh"
#include "VuoSubcompositionMessageRouter.hh"

/**
 * Creates a command for setting a composition's metadata.
 */
VuoCommandSetMetadata::VuoCommandSetMetadata(VuoCompositionMetadata *metadata, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Metadata"));
	this->window = window;

	this->revertedMetadata = new VuoCompositionMetadata(*window->getComposition()->getBase()->getMetadata());
	this->updatedMetadata = metadata;

	setDescription("Set metadata to:\n%s", metadata->toCompositionHeader().c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetMetadata::id() const
{
	return VuoCommandCommon::setMetadataCommandID;
}

/**
 * The composition's save state (and therefore its default name) may change outside of the Undo stack,
 * so the name needs to be re-generated when undo/redo is actually called,
 * not when the command is first pushed onto the Undo stack.
 */
void VuoCommandSetMetadata::updateDefaultCompositionName(VuoCompositionMetadata *metadata)
{
	if (! window->windowFilePath().isEmpty())
	{
		string defaultName = VuoEditorComposition::getDefaultNameForPath( window->windowFilePath().toStdString() );
		metadata->setDefaultName(defaultName);
	}
}

/**
 * Restores the composition to its old metadata.
 */
void VuoCommandSetMetadata::undo()
{
	VuoCommandCommon_undo;

	updateDefaultCompositionName(revertedMetadata);

	window->getComposition()->getBase()->setMetadata(revertedMetadata, false);

	window->getMetadataPanel()->update();
	window->getCurrentNodeLibrary()->displayPopoverInPane(window->getMetadataPanel());

	window->getComposition()->updateCompositionsThatContainThisSubcomposition(window->getComposition()->takeSnapshot());
}

/**
 * Sets the composition to its new metadata.
 */
void VuoCommandSetMetadata::redo()
{
	VuoCommandCommon_redo;

	updateDefaultCompositionName(updatedMetadata);

	window->getComposition()->getBase()->setMetadata(updatedMetadata, false);

	window->getMetadataPanel()->update();
	window->getCurrentNodeLibrary()->displayPopoverInPane(window->getMetadataPanel());

	window->getComposition()->updateCompositionsThatContainThisSubcomposition(window->getComposition()->takeSnapshot());
}
