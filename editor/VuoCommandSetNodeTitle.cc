/**
 * @file
 * VuoCommandSetNodeTitle implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetNodeTitle.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for setting the title of a node.
 */
VuoCommandSetNodeTitle::VuoCommandSetNodeTitle(VuoCompilerNode *node, string title, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Node Title"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		node->getBase()->getRenderer()->setTitle(title);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Set node %s title to '%s'",
		node->getIdentifier().c_str(),
		title.c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetNodeTitle::id() const
{
	return VuoCommandCommon::setNodeTitleCommandID;
}

/**
 * Restores the node to its old title.
 */
void VuoCommandSetNodeTitle::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->getComposition()->updatePortPopovers();
}

/**
 * Sets the node to its new title.
 */
void VuoCommandSetNodeTitle::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->getComposition()->updatePortPopovers();
}
