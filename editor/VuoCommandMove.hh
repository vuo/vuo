/**
 * @file
 * VuoCommandMove interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoRendererComment;
class VuoRendererNode;

/**
 * An undoable action for moving nodes and/or comments to a different position in the composition.
 */
class VuoCommandMove : public VuoCommandCommon
{
public:
	VuoCommandMove(set<VuoRendererNode *> movedNodes,
				   set<VuoRendererComment *> movedComments,
				   qreal dx,
				   qreal dy,
				   VuoEditorWindow *window,
				   bool movedByDragging=false);

	int id() const;
	void undo();
	void redo();
	bool mergeWith(const QUndoCommand *command);

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	// Used only for merging Undo commands:
	bool movedByDragging;
	set<string> movedItemIDs;
	qreal dx;
	qreal dy;
};
