/**
 * @file
 * VuoCommandUnpublishPort interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoPort;
class VuoPublishedPort;

/**
 * An undoable action for unpublishing a port.
 */
class VuoCommandUnpublishPort : public VuoCommandCommon
{
public:
	VuoCommandUnpublishPort(VuoPublishedPort *port, VuoEditorWindow *window);
	VuoCommandUnpublishPort(VuoPort *port, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	void initialize(VuoPublishedPort *explicitExternalPort,
					bool isPublishedInput,
					vector<pair<VuoPort *, VuoPublishedPort *> > internalExternalPortCombinations,
					VuoEditorWindow *window);
};
