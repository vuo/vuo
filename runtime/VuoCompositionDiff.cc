/**
 * @file
 * VuoCompositionDiff implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
 * This needs to be kept in sync with `VuoStringUtilities::buildPortIdentifier()`.
 */
string VuoCompositionDiff::joinPortIdentifier(const string &nodeIdentifier, const string &portName)
{
	return nodeIdentifier + ":" + portName;
}

/**
 * Constructs the path of a node as it would be expressed in a diff.
 */
string VuoCompositionDiff::convertIdentifierToPath(const char *compositionIdentifier, const char *nodeIdentifier)
{
	return string(compositionIdentifier) + "/" + nodeIdentifier;
}

/**
 * Extracts the composition identifier and node identifier from a node path in a diff.
 */
void VuoCompositionDiff::convertPathToIdentifier(const char *nodePath, string &compositionIdentifier, string &nodeIdentifier)
{
	string nodePathStr = nodePath;
	size_t pos = nodePathStr.rfind("/");
	if (pos == string::npos)
		return;

	compositionIdentifier = nodePathStr.substr(0, pos);
	nodeIdentifier = nodePathStr.substr(pos+1);
}

/**
 * Searches `compositionDiff` for changes made to the node across a live-coding reload.
 *
 * This needs to be kept in sync with @ref VuoCompilerCompositionDiff::diff.
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
VuoCompositionDiff::ChangeType VuoCompositionDiff::findNode(const char *compositionIdentifier, const char *nodeIdentifier,
															json_object **replacementObj)
{
	if (! diff)
		return ChangeStartStop;

	json_object *diffJson = json_tokener_parse(diff);
	if (! diffJson)
	{
		VUserLog("Couldn't parse the composition diff: %s", diff);
		return ChangeNone;
	}

	string nodePath = convertIdentifierToPath(compositionIdentifier, nodeIdentifier);

	set<string> keysFound;
	int numChanges = json_object_array_length(diffJson);
	for (int i = 0; i < numChanges; ++i)
	{
		json_object *change = json_object_array_get_idx(diffJson, i);
		json_object_object_foreach(change, key, val)
		{
			if (! strcmp(nodePath.c_str(), json_object_get_string(val)))
			{
				keysFound.insert(key);

				if (! strcmp(key, "replace") || ! strcmp(key, "with") || ! strcmp(key, "move") || ! strcmp(key, "to"))
				{
					json_object_get(change);
					*replacementObj = change;
				}
			}
		}
	}

	json_object_put(diffJson);

	if (keysFound.find("add") != keysFound.end())
		return ChangeAdd;
	else if (keysFound.find("remove") != keysFound.end())
		return ChangeRemove;
	else if (keysFound.find("replace") != keysFound.end() || keysFound.find("with") != keysFound.end())
		return ChangeReplace;
	else if (keysFound.find("move") != keysFound.end() || keysFound.find("to") != keysFound.end())
		return ChangeMove;

	return ChangeNone;
}

/**
 * Returns true if the node is among the removals or replacees across a live-coding reload,
 * or if the composition is being stopped (not a live-coding reload).
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
bool VuoCompositionDiff::isNodeBeingRemovedOrReplaced(const char *compositionIdentifier, const char *nodeIdentifier,
													  json_object **replacementObj)
{
	enum ChangeType changeType = findNode(compositionIdentifier, nodeIdentifier, replacementObj);
	return changeType == ChangeStartStop || changeType == ChangeRemove || changeType == ChangeReplace;
}

/**
 * Returns true if the node is among the additions or replacers across a live-coding reload,
 * or if the composition is being started (not a live-coding reload).
 *
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
bool VuoCompositionDiff::isNodeBeingAddedOrReplaced(const char *compositionIdentifier, const char *nodeIdentifier,
													json_object **replacementObj)
{
	enum ChangeType changeType = findNode(compositionIdentifier, nodeIdentifier, replacementObj);
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

		if (json_object_object_get_ex(replacementObj, "replace", &o))
		{
			const char *oldNodePath = json_object_get_string(o);
			string oldCompositionIdentifier;
			convertPathToIdentifier(oldNodePath, oldCompositionIdentifier, oldNodeIdentifier);
		}

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
 * Returns true if the node is being moved to the new composition across a live-coding reload.
 */
bool VuoCompositionDiff::isNodeBeingMovedToHere(const char *newCompositionIdentifier, const char *nodeIdentifier, json_object *replacementObj,
												string &oldCompositionIdentifier)
{
	string moveFrom;
	string moveTo;

	json_object *o;
	if (json_object_object_get_ex(replacementObj, "move", &o))
		moveFrom = json_object_get_string(o);

	if (json_object_object_get_ex(replacementObj, "to", &o))
		moveTo = json_object_get_string(o);

	if (moveTo == convertIdentifierToPath(newCompositionIdentifier, nodeIdentifier))
	{
		string unused;
		convertPathToIdentifier(moveFrom.c_str(), oldCompositionIdentifier, unused);
		return true;
	}

	return false;
}

/**
 * Returns true if the port's data is being copied to another port across a live-coding reload.
 */
bool VuoCompositionDiff::isPortBeingCopied(const char *portName, json_object *replacementObj,
										   string &destinationCompositionIdentifier, string &destinationPortIdentifier)
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

		if (json_object_object_get_ex(portMapping, "copy", &o) && ! strcmp(json_object_get_string(o), portName))
		{
			if (json_object_object_get_ex(portMapping, "to", &o))
			{
				const char *destinationPortPath = json_object_get_string(o);
				convertPathToIdentifier(destinationPortPath, destinationCompositionIdentifier, destinationPortIdentifier);
			}

			return true;
		}
	}

	return false;
}

/**
 * Returns true if the composition is either starting for the first time or stopping for the last time,
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
	return runtimeState->persistentState->compositionDiff->isNodeBeingRemovedOrReplaced(compositionState->compositionIdentifier,
																						nodeIdentifier, replacementObj);
}

/**
 * C wrapper for VuoCompositionDiff::isNodeBeingAddedOrReplaced().
 */
bool vuoIsNodeBeingAddedOrReplaced(VuoCompositionState *compositionState, const char *nodeIdentifier, json_object **replacementObj)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->compositionDiff->isNodeBeingAddedOrReplaced(compositionState->compositionIdentifier,
																					  nodeIdentifier, replacementObj);
}

}
