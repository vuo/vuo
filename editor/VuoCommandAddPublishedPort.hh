/**
 * @file
 * VuoCommandAddPublishedPort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoRendererPublishedPort;

/**
 * An undoable action for adding an externally visible published port
 * without any composition-internal connections.
 */
class VuoCommandAddPublishedPort : public VuoCommandCommon
{
public:
	VuoCommandAddPublishedPort(VuoRendererPublishedPort *publishedPort, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
