/**
 * @file
 * VuoCommandConnect interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorWindow;
class VuoInputEditorManager;
class VuoPort;
class VuoRendererCable;
class VuoRendererNode;
class VuoRendererPort;

/**
 * An undoable action for connecting a cable to a port.
 */
class VuoCommandConnect : public VuoCommandCommon
{
public:
	VuoCommandConnect(VuoRendererCable *cableInProgress,
					  VuoRendererPort *targetPort,
					  VuoRendererCable *displacedCable,
					  VuoRendererPort *portToUnpublish,
					  VuoEditorWindow *window,
					  VuoInputEditorManager *inputEditorManager);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	// Used only for live-coding updates:
	set<string> updatedPortIDs;

	// The following are not and must not be used outside of the constructor and its helper functions:
	// (@todo: Eliminate as member variables?)
	VuoRendererCable *cableInProgress;
	VuoRendererCable *displacedCable;
	VuoRendererNode *addedNode;
	VuoPort *fromPortForAddedCable;
	VuoPort *toPortForAddedCable;
	bool operationInvolvesGenericPort;
	VuoInputEditorManager *inputEditorManager;
	map<VuoRendererCable *, VuoPort *> revertedFromPortForCable;
	map<VuoRendererCable *, VuoPort *> revertedToPortForCable;
	map<VuoRendererCable *, VuoPort *> updatedFromPortForCable;
	map<VuoRendererCable *, VuoPort *> updatedToPortForCable;
	map<VuoPort *, string> revertedConstantForPort;
	map<VuoPort *, string> updatedConstantForPort;

	bool modifiedComponentsIncludeGenericPorts();
	void inventorySharedValueToBackpropagate();
};
