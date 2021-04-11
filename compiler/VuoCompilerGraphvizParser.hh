/**
 * @file
 * VuoCompilerGraphvizParser interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCable;
class VuoComment;
class VuoCompositionMetadata;
class VuoNode;
class VuoNodeClass;
class VuoPort;
class VuoPortClass;
class VuoPublishedPort;
class VuoType;
class VuoCompiler;
class VuoCompilerCable;
class VuoCompilerPort;

typedef struct Agraph_s Agraph_t;  ///< From `graphviz/cgraph.h`.
typedef struct Agnode_s Agnode_t;  ///< From `graphviz/cgraph.h`.

#include "VuoHeap.h"

/**
 * Parses nodes and cables from a .vuo composition file.
 */
class VuoCompilerGraphvizParser
{
private:
	static dispatch_queue_t graphvizQueue;  ///< Serializes calls to Graphviz parsing functions.
	VuoCompiler *compiler;
	Agraph_t *graph;
	map<string, VuoNodeClass *> dummyNodeClassForName;
	map<string, VuoNodeClass *> nodeClassForName;
	map<string, VuoNode *> nodeForName;
	map<string, VuoComment *> commentForName;
	vector<VuoNode *> orderedNodes;
	vector<VuoCable *> orderedCables;
	vector<VuoComment *> orderedComments;
	vector<VuoPublishedPort *> publishedInputPorts;
	vector<VuoPublishedPort *> publishedOutputPorts;
	VuoNode *publishedInputNode;
	VuoNode *publishedOutputNode;
	map< size_t, pair< VuoCompilerCable *, pair<string, string> > > publishedCablesInProgress;
	VuoNode *manuallyFirableInputNode;
	VuoPort *manuallyFirableInputPort;
	VuoCompositionMetadata *metadata;

	VuoCompilerGraphvizParser(const string &composition, VuoCompiler *compiler, bool nodeClassNamesOnly);
	void makeDummyNodeClasses(void);
	void makeNodeClasses(void);
	void makeNodes(void);
	void makeCables(void);
	void makeComments(void);
	void makePublishedPorts(void);
	void setInputPortConstantValues(void);
	void setPublishedPortDetails(void);
	void setTriggerPortEventThrottling(void);
	void setManuallyFirableInputPort(void);
	map<string, string> parsePortConstantValues(Agnode_t *n);
	bool parseAttributeOfPort(Agnode_t *n, string portName, string suffix, string &attributeValue);
	void checkPortClasses(string nodeClassName, vector<VuoPortClass *> dummy, vector<VuoPortClass *> actual);
	void saveNodeDeclarations(const string &compositionAsString);
	static VuoType * inferTypeForPublishedPort(string name, const set<VuoCompilerPort *> &connectedPorts);

public:
	static VuoCompilerGraphvizParser * newParserFromCompositionFile(const string &path, VuoCompiler *compiler);
	static VuoCompilerGraphvizParser * newParserFromCompositionString(const string &composition, VuoCompiler *compiler);
	static set<string> getNodeClassNamesFromCompositionFile(const string &path);
	static set<string> getNodeClassNamesFromCompositionString(const string &composition);
	vector<VuoNode *> getNodes(void);
	vector<VuoCable *> getCables(void);
	vector<VuoComment *> getComments(void);
	vector<VuoPublishedPort *> getPublishedInputPorts(void);
	vector<VuoPublishedPort *> getPublishedOutputPorts(void);
	VuoNode * getManuallyFirableInputNode(void);
	VuoPort * getManuallyFirableInputPort(void);
	VuoCompositionMetadata * getMetadata(void);
};
