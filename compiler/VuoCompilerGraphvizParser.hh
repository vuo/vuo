/**
 * @file
 * VuoCompilerGraphvizParser interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERGRAPHVIZPARSER_H
#define VUOCOMPILERGRAPHVIZPARSER_H

#include "VuoCompilerTriggerEdge.hh"
#include "VuoCompilerPassiveEdge.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCable.hh"

class VuoCompiler;

struct Agraph_t;
struct Agnode_t;
typedef struct Agraph_t graph_t; ///< Shorthand for @c Agraph_t.


/**
 * Parses nodes and edges from a .vuo composition file.
 */
class VuoCompilerGraphvizParser
{
private:
	VuoCompiler *compiler;
	graph_t *graph;
	map<string, set<string> > dummyInputNamesForNodeClassName;
	map<string, set<string> > dummyOutputNamesForNodeClassName;
	map<string, VuoNodeClass *> dummyNodeClassForName;
	map<string, VuoNodeClass *> nodeClassForName;
	map<string, VuoNode *> nodeForName;
	vector<VuoNode *> orderedNodes;
	vector<VuoCable *> orderedCables;
	vector<VuoCompilerPublishedPort *> publishedInputPorts;
	vector<VuoCompilerPublishedPort *> publishedOutputPorts;
	VuoNode *publishedInputNode;
	VuoNode *publishedOutputNode;
	vector<VuoCable *> publishedInputCables;
	vector<VuoCable *> publishedOutputCables;
	string description;

	VuoCompilerGraphvizParser(const string &composition, VuoCompiler *compiler, set<VuoCompilerNodeClass *> extraNodeClasses);
	void makeDummyPorts(void);
	void makeDummyNodeClasses(void);
	void makeNodeClasses(set<VuoCompilerNodeClass *> extraNodeClasses);
	void makeNodes(void);
	void makeCables(void);
	void makePublishedPorts(void);
	void setInputPortConstantValues(void);
	void setTriggerPortEventThrottling(void);
	map<string, string> parsePortConstantValues(Agnode_t *n);
	bool parseAttributeOfPort(Agnode_t *n, string portName, string suffix, string &attributeValue);
	void checkPortClasses(vector<VuoPortClass *> dummy, vector<VuoPortClass *> actual);
	void saveNodeDeclarations(const string &compositionAsString);
	static VuoType * inferTypeForPublishedPort(string name, const set<VuoCompilerPort *> &connectedPorts);

public:
	static VuoCompilerGraphvizParser * newParserFromCompositionFile(string path, VuoCompiler *compiler = NULL,
																	set<VuoCompilerNodeClass *> extraNodeClasses = set<VuoCompilerNodeClass *>());
	static VuoCompilerGraphvizParser * newParserFromCompositionString(const string &composition, VuoCompiler *compiler = NULL,
																	  set<VuoCompilerNodeClass *> extraNodeClasses = set<VuoCompilerNodeClass *>());
	vector<VuoNode *> getNodes(void);
	vector<VuoCable *> getCables(void);
	vector<VuoCompilerPublishedPort *> getPublishedInputPorts(void);
	vector<VuoCompilerPublishedPort *> getPublishedOutputPorts(void);
	VuoNode * getPublishedInputNode(void);
	VuoNode * getPublishedOutputNode(void);
	vector<VuoCable *> getPublishedInputCables(void);
	vector<VuoCable *> getPublishedOutputCables(void);
	string getDescription(void);
	static string parseDescription(const string &compositionAsString);
};

#endif
