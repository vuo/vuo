/**
 * @file
 * VuoCommandPublishPort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoPort;
class VuoRendererCable;

/**
 * An undoable action for publishing a port.
 */
class VuoCommandPublishPort : public VuoCommandCommon
{
public:
	VuoCommandPublishPort(VuoPort *port,
						  VuoRendererCable *displacedCable,
						  VuoEditorWindow *window,
						  bool forceEventOnlyPublication,
						  string publishedPortName="",
						  bool merge=false);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
