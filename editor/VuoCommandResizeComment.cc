/**
 * @file
 * VuoCommandResizeComment implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandResizeComment.hh"
#include "VuoComment.hh"
#include "VuoCompilerComment.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererComment.hh"

/**
 * Creates a command for setting the size of a comment.
 */
VuoCommandResizeComment::VuoCommandResizeComment(VuoRendererComment *comment, qreal dx, qreal dy, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Resize Comment"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Since the only current way to resize a comment is by dragging, the resizing must already have happened.
	bool changeAlreadyMade = true;
	if (changeAlreadyMade)
	{
		// If the item's size has already been updated, reconstruct its original size
		// for the composition's "Before" snapshot.
		comment->getBase()->setWidth(comment->getBase()->getWidth() - dx);
		comment->getBase()->setHeight(comment->getBase()->getHeight() - dy);

		this->revertedSnapshot = window->getComposition()->takeSnapshot();

		// Now re-re-size it.
		comment->getBase()->setWidth(comment->getBase()->getWidth() + dx);
		comment->getBase()->setHeight(comment->getBase()->getHeight() + dy);
	}
	else
		this->revertedSnapshot = window->getComposition()->takeSnapshot();


	// Start of command content.
	{
		// Would only need to do something here if @c changeAlreadyMade were false.
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Resize comment %s to %d,%d",
		comment->getBase()->getCompiler()->getGraphvizIdentifier().c_str(),
		comment->getBase()->getWidth(),
		comment->getBase()->getHeight());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandResizeComment::id() const
{
	return VuoCommandCommon::resizeCommentCommandID;
}

/**
 * Restores the comment to its old size.
 */
void VuoCommandResizeComment::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
}

/**
 * Updates the comment with its new size.
 */
void VuoCommandResizeComment::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
}
