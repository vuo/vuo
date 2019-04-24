/**
 * @file
 * VuoCompositionDiff implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompositionDiff.hh"

#include "VuoRuntimePersistentState.hh"
#include "VuoRuntimeState.hh"

/**
 * Constructor.
 */
VuoCompositionDiff::VuoCompositionDiff(void)
{
	diff = NULL;
}

/**
 * Destructor.
 */
VuoCompositionDiff::~VuoCompositionDiff(void)
{
	free(diff);
}

/**
 * Replaces the composition diff string with @a diff.
 *
 * This class takes ownership of @a diff, so the caller should not free it.
 */
void VuoCompositionDiff::setDiff(char *diff)
{
	free(this->diff);
	this->diff = diff;
}

/**
 * Constructs the identifier of a port on a node.
 *
 * This needs to be kept in sync with `VuoCompilerPort::getIdentifier()`.
 */
string VuoCompositionDiff::joinPortIdentifier(const string &nodeIdentifier, const string &portName)
{
	return nodeIdentifier + "__" + portName;
}

/**
 * Searches `compositionDiff` for changes made to the node across a live-coding reload.
 *
 * This needs to be kept in sync with `VuoCompilerComposition::diffAgainstOlderComposition()`.
 */
VuoCompositionDiff::ChangeType VuoCompositionDiff::findNode(const char *nodeIdentifier, json_object **replacementObj)
{
	if (! diff)
		return ChangeStartStop;

	json_object *diffJson = json_tokener_parse(diff);
	if (! diffJson)
	{
		VUserLog("Couldn't parse the composition diff: %s", diff);
		return ChangeNone;
	}

	set<string> keysFound;
	int numChanges = json_object_array_length(diffJson);
	for (int i = 0; i < numChanges; ++i)
	{
		json_object *change = json_object_array_get_idx(diffJson, i);
		json_object_object_foreach(change, key, val)
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(val)))
			{
				keysFound.insert(key);

				if (! strcmp(key, "map") || ! strcmp(key, "to"))
				{
					json_object_get(change);
					*replacementObj = change;
				}
			}
		}
	}

	json_object_put(diffJson);

	if (keysFound.find("add") != keysFound.end())
	{
		if (keysFound.find("to") != keysFound.end())
			return ChangeReplace;
		else
			return ChangeAdd;
	}
	else if (keysFound.find("remove") != keysFound.end())
	{
		if (keysFound.find("map") != keysFound.end())
			return ChangeReplace;
		else
			return ChangeRemove;
	}

	return ChangeNone;
}

/**
 * Returns true if the node is among the removals or replacees across a live-coding reload,
 * or if the composition is being stopped (not a live-coding reload).
 */
bool VuoCompositionDiff::isNodeBeingRemovedOrReplaced(const char *nodeIdentifier, json_object **replacementObj)
{
	enum ChangeType changeType = findNode(nodeIdentifier, replacementObj);
	return changeType == ChangeStartStop || changeType == ChangeRemove || changeType == ChangeReplace;
}

/**
 * Returns true if the node is among the additions or replacers across a live-coding reload,
 * or if the composition is being started (not a live-coding reload).
 */
bool VuoCompositionDiff::isNodeBeingAddedOrReplaced(const char *nodeIdentifier, json_object **replacementObj)
{
	enum ChangeType changeType = findNode(nodeIdentifier, replacementObj);
	return changeType == ChangeStartStop || changeType == ChangeAdd || changeType == ChangeReplace;
}

/**
 * Returns true if the port is among the replacees across a live-coding reload.
 */
bool VuoCompositionDiff::isPortBeingReplaced(const char *portName, json_object *replacementObj)
{
	json_object *portMappingArray = NULL;
	{
		json_object *o;
		if (json_object_object_get_ex(replacementObj, "ports", &o))
			portMappingArray = o;
	}
	if (! portMappingArray)
		return false;

	int numPortMappings = json_object_array_length(portMappingArray);
	for (int i = 0; i < numPortMappings; ++i)
	{
		json_object *portMapping = json_object_array_get_idx(portMappingArray, i);
		json_object *o;

		if (json_object_object_get_ex(portMapping, "map", &o) && ! strcmp(json_object_get_string(o), portName))
			return true;
	}

	return false;
}

/**
 * Returns true if the port is among the replacers across a live-coding reload.
 */
bool VuoCompositionDiff::isPortReplacingAnother(const char *portName, json_object *replacementObj,
												string &oldNodeIdentifier, string &oldPortIdentifier)
{
	json_object *portMappingArray = NULL;
	{
		json_object *o;

		if (json_object_object_get_ex(replacementObj, "map", &o))
			oldNodeIdentifier = json_object_get_string(o);

		if (json_object_object_get_ex(replacementObj, "ports", &o))
			portMappingArray = o;
	}
	if (! portMappingArray)
		return false;

	string oldPortName;
	bool foundPort = false;
	int numPortMappings = json_object_array_length(portMappingArray);
	for (int i = 0; i < numPortMappings; ++i)
	{
		json_object *portMapping = json_object_array_get_idx(portMappingArray, i);
		json_object *o;

		if (json_object_object_get_ex(portMapping, "to", &o) && ! strcmp(json_object_get_string(o), portName))
		{
			if (json_object_object_get_ex(portMapping, "map", &o))
				oldPortName = json_object_get_string(o);

			foundPort = true;
			break;
		}
	}
	if (! foundPort)
		return false;

	oldPortIdentifier = joinPortIdentifier(oldNodeIdentifier, oldPortName);
	return true;
}

/**
 * Returns true the composition is either starting for the first time or stopping for the last time,
 * false if the composition is being restarted for a live-coding reload.
 */
bool VuoCompositionDiff::isCompositionStartingOrStopping(void)
{
	return ! diff;
}

extern "C"
{
/**
 * C wrapper for VuoCompositionDiff::setDiff().
 */
void vuoSetCompositionDiff(VuoCompositionState *compositionState, char *diff)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->compositionDiff->setDiff(diff);
}

/**
 * C wrapper for VuoCompositionDiff::isNodeBeingRemovedOrReplaced().
 */
bool vuoIsNodeBeingRemovedOrReplaced(VuoCompositionState *compositionState, const char *nodeIdentifier, json_object **replacementObj)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->compositionDiff->isNodeBeingRemovedOrReplaced(nodeIdentifier, replacementObj);
}

/**
 * C wrapper for VuoCompositionDiff::isNodeBeingAddedOrReplaced().
 */
bool vuoIsNodeBeingAddedOrReplaced(VuoCompositionState *compositionState, const char *nodeIdentifier, json_object **replacementObj)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->compositionDiff->isNodeBeingAddedOrReplaced(nodeIdentifier, replacementObj);
}

}
