/**
 * @file
 * VuoRuntimeHelper implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

#include <graphviz/gvc.h>

extern "C"
{

struct PortContext;
struct NodeContext;

bool isPaused = false;  ///< True if node execution is currently paused.

unsigned long vuoLastEventId = 0;  ///< The ID most recently assigned to any event, composition-wide. Used to generate a unique ID for each event.

static map<string, struct NodeContext *> nodeContextForIdentifier;  ///< A registry of all NodeContext values in the running composition.
char *vuoTopLevelCompositionIdentifier;  ///< The key of the top-level composition in nodeContextForIdentifier.
static map<string, map<string, void *> > dataForPort;  ///< The `data` field in the port's context, indexed by composition and port identifier.
static map<string, map<string, dispatch_semaphore_t> > nodeSemaphoreForPort;  ///< The `semaphore` field in the node's context, indexed by composition and port identifier.
static map<string, map<string, unsigned long> > nodeIndexForPort;  ///< The index for a node, indexed by composition and port identifier.
static map<string, map<string, unsigned long> > typeIndexForPort;  ///< The index for the port's type, indexed by composition and port identifier.

char *compositionDiff = NULL;  ///< Differences between the old and new composition, when replacing compositions for live coding.

#include <graphviz/gvplugin.h>
extern gvplugin_library_t gvplugin_dot_layout_LTX_library;  ///< Reference to the statically-built Graphviz Dot library.
extern gvplugin_library_t gvplugin_core_LTX_library;  ///< Reference to the statically-built Graphviz core library.

static GVC_t *graphvizContext = NULL;  ///< The context used when working with a Graphviz graph.


/**
 * Returns a unique event ID.
 */
unsigned long vuoGetNextEventId(void)
{
	return __sync_add_and_fetch(&vuoLastEventId, 1);
}


/**
 * Registers a node context for the node identifier.
 */
void vuoAddNodeContext(const char *nodeIdentifier, struct NodeContext *nodeContext)
{
	nodeContextForIdentifier[nodeIdentifier] = nodeContext;
}

/**
 * Returns the node context registered for the node identifier, or null if none is found.
 */
struct NodeContext * vuoGetNodeContext(const char *nodeIdentifier)
{
	map<string, struct NodeContext *>::iterator nodeContextIter = nodeContextForIdentifier.find(nodeIdentifier);
	if (nodeContextIter != nodeContextForIdentifier.end())
		return nodeContextIter->second;

	VUserLog("Couldn't find context for node '%s'", nodeIdentifier);
	return NULL;
}

/**
 * Returns the `data` field in a port's context, given the port's identifier.
 */
void * vuoGetDataForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, void *> >::iterator compIter = dataForPort.find(compositionIdentifier);
	if (compIter != dataForPort.end())
	{
		map<string, void *>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find data for port '%s'", portIdentifier);
	return NULL;
}

/**
 * Returns the `semaphore` field in a node's context, given the identifier of a port on the node.
 */
dispatch_semaphore_t vuoGetNodeSemaphoreForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, dispatch_semaphore_t> >::iterator compIter = nodeSemaphoreForPort.find(compositionIdentifier);
	if (compIter != nodeSemaphoreForPort.end())
	{
		map<string, dispatch_semaphore_t>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find node semaphore for port '%s'", portIdentifier);
	return NULL;
}

/**
 * Returns the numerical index for a node, given the identifier of a port on the node.
 */
unsigned long vuoGetNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, unsigned long> >::iterator compIter = nodeIndexForPort.find(compositionIdentifier);
	if (compIter != nodeIndexForPort.end())
	{
		map<string, unsigned long>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find node index for port '%s'", portIdentifier);
	return 0;
}

/**
 * Returns the numerical index for a port's type, given the port's identifier.
 */
unsigned long vuoGetTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, unsigned long> >::iterator compIter = typeIndexForPort.find(compositionIdentifier);
	if (compIter != typeIndexForPort.end())
	{
		map<string, unsigned long>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find type index for port '%s'", portIdentifier);
	return 0;
}

/**
 * Stores information about the port, indexed on the port's identifier, so it can be efficiently retrieved later.
 */
void vuoAddPortIdentifier(const char *compositionIdentifier, const char *portIdentifier, void *data,
						  dispatch_semaphore_t nodeSemaphore, unsigned long nodeIndex, unsigned long typeIndex)
{
	dataForPort[compositionIdentifier][portIdentifier] = data;
	nodeSemaphoreForPort[compositionIdentifier][portIdentifier] = nodeSemaphore;
	nodeIndexForPort[compositionIdentifier][portIdentifier] = nodeIndex;
	typeIndexForPort[compositionIdentifier][portIdentifier] = typeIndex;
}

/**
 * Returns true if the node is found in both the old and the new composition, when replacing compositions for live coding.
 *
 * This needs to be kept in sync with VuoCompilerComposition::diffAgainstOlderComposition().
 */
bool isNodeInBothCompositions(const char *nodeIdentifier)
{
	if (! compositionDiff)
		return false;

	json_object *diff = json_tokener_parse(compositionDiff);
	if (! diff)
		return false;

	bool isInChanges = false;
	int numChanges = json_object_array_length(diff);
	for (int i = 0; i < numChanges; ++i)
	{
		json_object *change = json_object_array_get_idx(diff, i);
		json_object *nodeIdentifierObj;
		if (json_object_object_get_ex(change, "add", &nodeIdentifierObj))
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(nodeIdentifierObj)))
			{
				isInChanges = true;
				break;
			}
		}
		else if (json_object_object_get_ex(change, "remove", &nodeIdentifierObj))
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(nodeIdentifierObj)))
			{
				isInChanges = true;
				break;
			}
		}
	}

	json_object_put(diff);

	return ! isInChanges;
}


/**
 * Returns a Graphviz graph constructed from the given Graphviz-format string.
 */
graph_t * openGraphvizGraph(const char *graphString)
{
	// Use builtin Graphviz plugins, not demand-loaded plugins.
	lt_symlist_t lt_preloaded_symbols[] =
	{
		{ "gvplugin_dot_layout_LTX_library", &gvplugin_dot_layout_LTX_library},
		{ "gvplugin_core_LTX_library", &gvplugin_core_LTX_library},
		{ 0, 0}
	};
	bool demandLoading = false;
	graphvizContext = gvContextPlugins(lt_preloaded_symbols, demandLoading);

	graph_t *graph = agmemread((char *)graphString);
	agraphattr(graph, (char *)"rankdir", (char *)"LR");
	agnodeattr(graph, (char *)"shape", (char *)"Mrecord");
	gvLayout(graphvizContext, graph, "dot");

	return graph;
}

/**
 * Cleans up a Graphviz graph when it is no longer in use.
 */
void closeGraphvizGraph(graph_t * graph)
{
	gvFreeLayout(graphvizContext, graph);
	agclose(graph);
	gvFreeContext(graphvizContext);

	graphvizContext = NULL;
}

/**
 * If the new node and port have a mapping from the old composition, when replacing compositions for live coding,
 * finds the old node and port that they map from.
 *
 * This needs to be kept in sync with VuoCompilerComposition::diffAgainstOlderComposition().
 */
static void mapFromReplacementNodeAndPort(const char *newNodeIdentifier, const char *newPortIdentifier,
										  char **oldNodeIdentifier, char **oldPortIdentifier)
{
	*oldNodeIdentifier = NULL;
	*oldPortIdentifier = NULL;

	if (! compositionDiff)
		return;

	json_object *diff = json_tokener_parse(compositionDiff);
	if (! diff)
		return;

	int numChanges = json_object_array_length(diff);
	for (int i = 0; i < numChanges && ! *oldNodeIdentifier && ! *oldPortIdentifier; ++i)
	{
		json_object *change = json_object_array_get_idx(diff, i);
		json_object *newNodeIdentifierObj;
		if (json_object_object_get_ex(change, "to", &newNodeIdentifierObj))
		{
			if (! strcmp(newNodeIdentifier, json_object_get_string(newNodeIdentifierObj)))
			{
				json_object *oldNodeIdentifierObj;
				if (json_object_object_get_ex(change, "map", &oldNodeIdentifierObj))
				{
					*oldNodeIdentifier = strdup( json_object_get_string(oldNodeIdentifierObj) );
				}

				json_object *portsObj;
				if (json_object_object_get_ex(change, "ports", &portsObj))
				{
					int numPorts = json_object_array_length(portsObj);
					for (int j = 0; j < numPorts; ++j)
					{
						json_object *portObj = json_object_array_get_idx(portsObj, j);
						json_object *newPortIdentifierObj;
						if (json_object_object_get_ex(portObj, "to", &newPortIdentifierObj))
						{
							if (! strcmp(newPortIdentifier, json_object_get_string(newPortIdentifierObj)))
							{
								json_object *oldPortIdentifierObj;
								if (json_object_object_get_ex(portObj, "map", &oldPortIdentifierObj))
								{
									*oldPortIdentifier = strdup( json_object_get_string(oldPortIdentifierObj) );
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	if (! (*oldNodeIdentifier && *oldPortIdentifier) )
	{
		free(*oldNodeIdentifier);
		free(*oldPortIdentifier);
		*oldNodeIdentifier = NULL;
		*oldPortIdentifier = NULL;
	}

	json_object_put(diff);
}

/**
 * Returns the constant value of the input port in the serialized composition, or null if it is not found.
 *
 * The input port is looked up from the Graphviz graph, using any mappings of old-to-new nodes and ports
 * in the composition diff.
 */
const char * getConstantValueFromGraphviz(graph_t *graph, const char *node, const char *port)
{
	char *portConstantUnescaped = NULL;

	char *replacedNode, *replacedPort;
	mapFromReplacementNodeAndPort(node, port, &replacedNode, &replacedPort);

	const char *nodeInGraph, *portInGraph;
	if (replacedNode && replacedPort)
	{
		nodeInGraph = replacedNode;
		portInGraph = replacedPort;
	}
	else
	{
		nodeInGraph = node;
		portInGraph = port;
	}

	for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
	{
		if (! strcmp(n->name, nodeInGraph))
		{
			field_t *nodeInfo = (field_t *)ND_shape_info(n);
			int numNodeInfoFields = nodeInfo->n_flds;

			for (int i = 0; i < numNodeInfoFields; i++)
			{
				field_t *nodeInfoField = nodeInfo->fld[i];
				if (nodeInfoField->id && ! strcmp(nodeInfoField->id, portInGraph))
				{
					char portInDotInitializer[strlen("_")+strlen(portInGraph)+1];
					sprintf(portInDotInitializer, "_%s", portInGraph);

					char *portConstant = agget(n, portInDotInitializer);
					if (! portConstant)
						break;

					size_t portConstantLen = strlen(portConstant);
					portConstantUnescaped = (char *)malloc(portConstantLen+1);
					int k = 0;
					for (int j = 0; j < portConstantLen; ++j)
					{
						if (j < portConstantLen-1 && portConstant[j] == '\\' && portConstant[j+1] == '\\')
							++j;
						portConstantUnescaped[k++] = portConstant[j];
					}
					portConstantUnescaped[k] = 0;

					break;
				}
			}

			break;
		}
	}

	free(replacedNode);
	free(replacedPort);

	return portConstantUnescaped;
}

/**
 * Replaces '"' with '\"' and '\' with '\\' in a string.
 */
char * vuoTranscodeToGraphvizIdentifier(const char *originalString)
{
	size_t originalStringLen = strlen(originalString);
	size_t escapeCount = 0;
	for (size_t i = 0; i < originalStringLen; ++i)
		if (originalString[i] == '"' || originalString[i] == '\\')
			++escapeCount;

	size_t escapedStrlen = originalStringLen + escapeCount;
	char *escapedString = (char *)malloc(escapedStrlen + 1);
	for (size_t originalPos = 0, escapedPos = 0; originalPos < originalStringLen; ++originalPos, ++escapedPos)
	{
		if (originalString[originalPos] == '"' || originalString[originalPos] == '\\')
			escapedString[escapedPos++] = '\\';
		escapedString[escapedPos] = originalString[originalPos];
	}
	escapedString[escapedStrlen] = '\0';

	return escapedString;
}


/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings2(const char *s0, const char *s1)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strcat(buffer, s0);
	strcat(buffer, s1);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings3(const char *s0, const char *s1, const char *s2)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + strlen(s2) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strcat(buffer, s0);
	strcat(buffer, s1);
	strcat(buffer, s2);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings(const char **strings, size_t stringCount)
{
	size_t bufferLength = 1;
	for (size_t i = 0; i < stringCount; ++i)
		bufferLength += strlen(strings[i]);
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	for (size_t i = 0; i < stringCount; ++i)
		strcat(buffer, strings[i]);
	return buffer;
}

}  // extern "C"
