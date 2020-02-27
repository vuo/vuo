/**
 * @file
 * VuoCommandSetPortConstant interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoCompilerPort;
class VuoEditorWindow;

/**
 * An undoable action for setting the constant value of a port.
 */
class VuoCommandSetPortConstant : public VuoCommandCommon
{
public:
	VuoCommandSetPortConstant(VuoCompilerPort *port, string constant, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	// Used only for live-coding updates:
	string portID;
	bool portIsPublished;
};
