/**
 * @file
 * VuoCommandReorderPublishedPorts interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoPublishedPort;

/**
 * An undoable action for reordering a composition's published ports.
 */
class VuoCommandReorderPublishedPorts : public VuoCommandCommon
{
public:
	VuoCommandReorderPublishedPorts(vector<VuoPublishedPort *> ports, bool isInput, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
