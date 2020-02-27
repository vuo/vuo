/**
 * @file
 * VuoCommandSetTriggerThrottling interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoPort;
#include "VuoPortClass.hh"

/**
 * An undoable action for editing the event-throttling behavior of a trigger port.
 */
class VuoCommandSetTriggerThrottling : public VuoCommandCommon
{
public:
	VuoCommandSetTriggerThrottling(VuoPort *port, enum VuoPortClass::EventThrottling eventThrottling, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
};
