/**
 * @file
 * VuoCommandMove implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>

#include "VuoCommandCommon.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerNode.hh"
#include "VuoCommandMove.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoComment.hh"
#include "VuoRendererComment.hh"
#include "VuoRendererNode.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for moving the given items to a different position.
 */
VuoCommandMove::VuoCommandMove(set<VuoRendererNode *> movedNodes,
							   set<VuoRendererComment *> movedComments,
							   qreal dx,
							   qreal dy,
							   VuoEditorWindow *window,
							   bool movedByDragging)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Move"));
	this->window = window;

	this->dx = dx;
	this->dy = dy;
	this->movedByDragging = movedByDragging;

	foreach (VuoRendererNode *rn, movedNodes)
	{
		if (rn->getBase()->hasCompiler())
			movedItemIDs.insert(rn->getBase()->getCompiler()->getGraphvizIdentifier());
	}

	foreach (VuoRendererComment *rc, movedComments)
	{
		if (rc->getBase()->hasCompiler())
			movedItemIDs.insert(rc->getBase()->getCompiler()->getGraphvizIdentifier());
	}

	bool moveAlreadyMade = movedByDragging;
	if (moveAlreadyMade)
	{
		// If the items' positions have already been updated, reconstruct their original positions
		// for the composition's "Before" snapshot.
		for (set<VuoRendererNode *>::iterator node = movedNodes.begin(); node != movedNodes.end(); ++node)
		{
			(*node)->getBase()->setX((*node)->getBase()->getX() - dx);
			(*node)->getBase()->setY((*node)->getBase()->getY() - dy);
		}

		for (set<VuoRendererComment *>::iterator comment = movedComments.begin(); comment != movedComments.end(); ++comment)
		{
			(*comment)->getBase()->setX((*comment)->getBase()->getX() - dx);
			(*comment)->getBase()->setY((*comment)->getBase()->getY() - dy);
		}

		this->revertedSnapshot = window->getComposition()->takeSnapshot();

		// Now put them back.
		for (set<VuoRendererNode *>::iterator node = movedNodes.begin(); node != movedNodes.end(); ++node)
		{
			(*node)->getBase()->setX((*node)->getBase()->getX() + dx);
			(*node)->getBase()->setY((*node)->getBase()->getY() + dy);
		}

		for (set<VuoRendererComment *>::iterator comment = movedComments.begin(); comment != movedComments.end(); ++comment)
		{
			(*comment)->getBase()->setX((*comment)->getBase()->getX() + dx);
			(*comment)->getBase()->setY((*comment)->getBase()->getY() + dy);
		}
	}
	else
		this->revertedSnapshot = window->getComposition()->takeSnapshot();


	// Start of command content.
	{
		if (!moveAlreadyMade)
		{
			for (set<VuoRendererNode *>::iterator node = movedNodes.begin(); node != movedNodes.end(); ++node)
			{
				(*node)->getBase()->setX((*node)->getBase()->getX() + dx);
				(*node)->getBase()->setY((*node)->getBase()->getY() + dy);
				(*node)->moveBy(dx, dy);

				(*node)->layoutConnectedInputDrawers();
			}

			for (set<VuoRendererComment *>::iterator comment = movedComments.begin(); comment != movedComments.end(); ++comment)
			{
				(*comment)->getBase()->setX((*comment)->getBase()->getX() + dx);
				(*comment)->getBase()->setY((*comment)->getBase()->getY() + dy);
				(*comment)->moveBy(dx, dy);
			}
		}
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	if (dx || dy)
		setDescription("Move %s by %g,%g%s", VuoStringUtilities::join(movedItemIDs, ", ").c_str(), dx, dy, movedByDragging ? " (drag)" : "");
}

/**
 * Returns the ID of this command.
 */
int VuoCommandMove::id() const
{
	return VuoCommandCommon::moveCommandID;
}

/**
 * Moves the items back to their old position.
 */
void VuoCommandMove::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
}

/**
 * Moves the items to their new position.
 */
void VuoCommandMove::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
}

/**
 * Coalesce consecutive moves of an item.
 */
bool VuoCommandMove::mergeWith(const QUndoCommand *command)
{
	const VuoCommandMove *otherMoveCommand = dynamic_cast<const VuoCommandMove *>(command);
	if (! otherMoveCommand)
		return false;

	// Merge consecutive moves of the same set of components by keypress.
	// However, assume that moves by dragging are fully aggregated (per mouse press/release) ahead of time.
	if ((movedItemIDs == otherMoveCommand->movedItemIDs) && !this->movedByDragging && !otherMoveCommand->movedByDragging)
	{
		this->updatedSnapshot = otherMoveCommand->updatedSnapshot;
		return true;
	}

	return false;
}
