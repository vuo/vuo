/**
 * @file
 * VuoCommandSetNodeTitle interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"
#include "VuoCompilerNode.hh"

class VuoEditorWindow;

/**
 * An undoable action for setting the title of a node.
 */
class VuoCommandSetNodeTitle : public VuoCommandCommon
{
public:
	VuoCommandSetNodeTitle(VuoCompilerNode *node, string title, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
