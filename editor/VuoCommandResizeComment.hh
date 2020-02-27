/**
 * @file
 * VuoCommandResizeComment interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoRendererComment;
class VuoEditorWindow;

/**
 * An undoable action for setting the size of a comment.
 */
class VuoCommandResizeComment : public VuoCommandCommon
{
public:
	VuoCommandResizeComment(VuoRendererComment *comment, qreal dx, qreal dy, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
