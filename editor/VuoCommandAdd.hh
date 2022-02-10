/**
 * @file
 * VuoCommandAdd interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;

/**
 * An undoable action for adding nodes, cables, and/or comments to the composition.
 */
class VuoCommandAdd : public VuoCommandCommon
{
public:
	VuoCommandAdd(QList<QGraphicsItem *> addedComponents, VuoEditorWindow *window, string commandDescription="Add", bool disableAttachmentInsertion=false);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	bool operationInvolvesGenericPort;
	bool operationRequiresRunningCompositionUpdate;
};
