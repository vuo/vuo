/**
 * @file
 * VuoCommandSetCommentText implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandSetCommentText.hh"
#include "VuoComment.hh"
#include "VuoCompilerComment.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererComment.hh"

/**
 * Creates a command for setting the text of a comment.
 */
VuoCommandSetCommentText::VuoCommandSetCommentText(VuoRendererComment *comment, string text, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Set Comment Content"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		comment->setContent(text);
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("Set %s text to %s",
		comment->getBase()->getCompiler()->getGraphvizIdentifier().c_str(),
		text.c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandSetCommentText::id() const
{
	return VuoCommandCommon::setCommentTextCommandID;
}

/**
 * Restores the comment to its old text content.
 */
void VuoCommandSetCommentText::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
}

/**
 * Updates the comment with its new text content.
 */
void VuoCommandSetCommentText::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
}
