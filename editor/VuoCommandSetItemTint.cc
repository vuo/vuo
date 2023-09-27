/**
 * @file
 * VuoCommandSetItemTint implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetItemTint.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerNode.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererComment.hh"
#include "VuoComment.hh"

/**
 * Creates a command for setting the tint color of a node or comment.
 */
VuoCommandSetItemTint::VuoCommandSetItemTint(QGraphicsItem *item, VuoNode::TintColor tintColor, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Tint"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();
	string itemIdentifier = "?";

	// Start of command content.
	{
		VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(item);
		VuoRendererComment *comment = dynamic_cast<VuoRendererComment *>(item);

		if (node)
		{
			QGraphicsItem::CacheMode normalCacheMode = node->cacheMode();
			node->setCacheModeForNodeAndPorts(QGraphicsItem::NoCache);

			set<VuoCable *> connectedCables = node->getConnectedCables(true);
			foreach (VuoCable *cable, connectedCables)
				cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);

			node->updateGeometry();
			node->getBase()->setTintColor(tintColor);

			node->setCacheModeForNodeAndPorts(normalCacheMode);

			foreach (VuoCable *cable, connectedCables)
				cable->getRenderer()->setCacheModeForCableAndConnectedPorts(normalCacheMode);

			if (node->getBase()->hasCompiler())
				itemIdentifier = node->getBase()->getCompiler()->getIdentifier().c_str();
		}
		else if (comment)
		{
			QGraphicsItem::CacheMode normalCacheMode = comment->cacheMode();
			comment->setCacheMode(QGraphicsItem::NoCache);
			comment->updateGeometry();

			comment->getBase()->setTintColor(tintColor);

			comment->setCacheMode(normalCacheMode);

			itemIdentifier = comment->getBase()->getCompiler()->getGraphvizIdentifier().c_str();
		}
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Tint %s to %s",
				   itemIdentifier.c_str(),
				   tintColor == VuoNode::TintNone ? "none" : VuoNode::getGraphvizNameForTint(tintColor).c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetItemTint::id() const
{
	return VuoCommandCommon::setItemTintCommandID;
}

/**
 * Restores the node to its old tint.
 */
void VuoCommandSetItemTint::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
}

/**
 * Sets the node to its new tint.
 */
void VuoCommandSetItemTint::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
}
