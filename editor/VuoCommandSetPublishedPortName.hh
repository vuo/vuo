/**
 * @file
 * VuoCommandSetPublishedPortName interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoRendererPublishedPort;

/**
 * An undoable action for setting the name of a published port.
 */
class VuoCommandSetPublishedPortName : public VuoCommandCommon
{

public:
	VuoCommandSetPublishedPortName(VuoRendererPublishedPort *port, string name, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
