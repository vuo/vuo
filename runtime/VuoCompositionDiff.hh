/**
 * @file
 * VuoCompositionDiff interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoRuntimeState;
#include "VuoCompositionState.h"

/**
 * Manages the diff between composition versions before and after a live-coding reload.
 */
class VuoCompositionDiff
{
public:
	/**
	 * Possible changes to a node across a live-coding reload.
	 */
	enum ChangeType
	{
		ChangeStartStop,  ///< The composition is being started or stopped (not a live-coding reload).
		ChangeNone,  ///< The node is carried across the live-coding reload.
		ChangeAdd,  ///< The node has been added.
		ChangeRemove,  ///< The node has been removed.
		ChangeReplace,  ///< The node has been removed with a replacement provided or added as a replacement for another.
		ChangeMove  ///< The node has been moved to a different (sub)composition.
	};

private:
	char *diff;  ///< Differences between the old and new composition, when replacing compositions for live coding.

	static string joinPortIdentifier(const string &nodeIdentifier, const string &portName);
	static string convertIdentifierToPath(const char *compositionIdentifier, const char *nodeIdentifier);
	static void convertPathToIdentifier(const char *nodePath, string &compositionIdentifier, string &nodeIdentifier);

public:
	VuoCompositionDiff(void);
	~VuoCompositionDiff(void);
	void setDiff(char *diff);
	ChangeType findNode(const char *compositionIdentifier, const char *nodeIdentifier, json_object **replacementObj);
	bool isNodeBeingRemovedOrReplaced(const char *compositionIdentifier, const char *nodeIdentifier, json_object **replacementObj);
	bool isNodeBeingAddedOrReplaced(const char *compositionIdentifier, const char *nodeIdentifier, json_object **replacementObj);
	bool isPortBeingReplaced(const char *portName, json_object *replacementObj);
	bool isPortReplacingAnother(const char *portName, json_object *replacementObj, string &oldNodeIdentifier, string &oldPortIdentifier);
	bool isNodeBeingMovedToHere(const char *newCompositionIdentifier, const char *nodeIdentifier, json_object *replacementObj, string &oldCompositionIdentifier);
	bool isPortBeingCopied(const char *portName, json_object *replacementObj, string &destinationCompositionIdentifier, string &destinationPortIdentifier);
	bool isCompositionStartingOrStopping(void);
};

extern "C"
{
void vuoSetCompositionDiff(VuoCompositionState *compositionState, char *diff);
bool vuoIsNodeBeingRemovedOrReplaced(VuoCompositionState *compositionState, const char *nodeIdentifier, json_object **replacementObj);
bool vuoIsNodeBeingAddedOrReplaced(VuoCompositionState *compositionState, const char *nodeIdentifier, json_object **replacementObj);
}
