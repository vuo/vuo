/**
 * @file
 * VuoCommandSetCableHidden interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoRendererCable;

/**
 * An undoable action for setting the wireless (hidden) status of a cable.
 */
class VuoCommandSetCableHidden : public VuoCommandCommon
{
public:
	VuoCommandSetCableHidden(VuoRendererCable *cable, bool hidden, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
