/**
 * @file
 * VuoCompilerGraphvizParser interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERGRAPHVIZPARSER_H
#define VUOCOMPILERGRAPHVIZPARSER_H

class VuoCable;
class VuoNode;
class VuoNodeClass;
class VuoPortClass;
class VuoPublishedPort;
class VuoType;
class VuoCompiler;
class VuoCompilerCable;
class VuoCompilerPort;

struct Agraph_t;
struct Agnode_t;
typedef struct Agraph_t graph_t; ///< Shorthand for @c Agraph_t.


/**
 * Parses nodes and cables from a .vuo composition file.
 */
class VuoCompilerGraphvizParser
{
private:
	VuoCompiler *compiler;
	graph_t *graph;
	map<string, VuoNodeClass *> dummyNodeClassForName;
	map<string, VuoNodeClass *> nodeClassForName;
	map<string, VuoNode *> nodeForName;
	vector<VuoNode *> orderedNodes;
	vector<VuoCable *> orderedCables;
	vector<VuoPublishedPort *> publishedInputPorts;
	vector<VuoPublishedPort *> publishedOutputPorts;
	VuoNode *publishedInputNode;
	VuoNode *publishedOutputNode;
	map< size_t, pair< VuoCompilerCable *, pair<string, string> > > publishedCablesInProgress;
	string name;
	string description;
	string copyright;

	VuoCompilerGraphvizParser(const string &composition, VuoCompiler *compiler);
	void makeDummyNodeClasses(void);
	void makeNodeClasses(void);
	void makeNodes(void);
	void makeCables(void);
	void makePublishedPorts(void);
	void setInputPortConstantValues(void);
	void setPublishedPortDetails(void);
	void setTriggerPortEventThrottling(void);
	map<string, string> parsePortConstantValues(Agnode_t *n);
	bool parseAttributeOfPort(Agnode_t *n, string portName, string suffix, string &attributeValue);
	void checkPortClasses(string nodeClassName, vector<VuoPortClass *> dummy, vector<VuoPortClass *> actual);
	void saveNodeDeclarations(const string &compositionAsString);
	static VuoType * inferTypeForPublishedPort(string name, const set<VuoCompilerPort *> &connectedPorts);

public:
	static VuoCompilerGraphvizParser * newParserFromCompositionFile(string path, VuoCompiler *compiler = NULL);
	static VuoCompilerGraphvizParser * newParserFromCompositionString(const string &composition, VuoCompiler *compiler = NULL);
	vector<VuoNode *> getNodes(void);
	vector<VuoCable *> getCables(void);
	vector<VuoPublishedPort *> getPublishedInputPorts(void);
	vector<VuoPublishedPort *> getPublishedOutputPorts(void);
	string getName(void);
	string getDescription(void);
	string getCopyright(void);
};

#endif
