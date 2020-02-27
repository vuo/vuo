/**
 * @file
 * VuoCommandSetItemTint interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"
#include "VuoNode.hh"

class VuoEditorWindow;

/**
 * An undoable action for setting the tint color of a node or comment.
 */
class VuoCommandSetItemTint : public VuoCommandCommon
{
public:
	VuoCommandSetItemTint(QGraphicsItem *item, VuoNode::TintColor tintColor, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
