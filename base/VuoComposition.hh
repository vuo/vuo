/**
 * @file
 * VuoComposition interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPOSITION_HH
#define VUOCOMPOSITION_HH

#include "VuoBase.hh"

class VuoCompilerComposition;
class VuoRendererComposition;
class VuoCable;
class VuoNode;
class VuoPort;
class VuoPublishedPort;

/**
 * A collection of nodes and the cables connecting them.
 */
class VuoComposition : public VuoBase<VuoCompilerComposition,VuoRendererComposition>
{
public:
	VuoComposition(void);

	void setName(string name);
	string getName(void);

	void setDirectory(string directory);
	string getDirectory(void);

	void setDescription(string description);
	string getDescription(void);

	void setCopyright(string copyright);
	string getCopyright(void);

	void addNode(VuoNode *node);
	void removeNode(VuoNode *node);
	void replaceNode(VuoNode *oldNode, VuoNode *newNode);
	set<VuoNode *> getNodes(void);

	void addCable(VuoCable *cable);
	void removeCable(VuoCable *cable);
	set<VuoCable *> getCables(void);

	void addPublishedInputPort(VuoPublishedPort *port, int index);
	void addPublishedOutputPort(VuoPublishedPort *port, int index);
	void removePublishedInputPort(int index);
	void removePublishedOutputPort(int index);
	vector<VuoPublishedPort *> getPublishedInputPorts(void);
	vector<VuoPublishedPort *> getPublishedOutputPorts(void);
	VuoPublishedPort * getPublishedInputPortWithName(string name);
	VuoPublishedPort * getPublishedOutputPortWithName(string name);
	int getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput);

	static void parseHeader(const string &compositionAsString, string &name, string &description, string &copyright);

private:
	string name;
	string directory;
	string description;
	string copyright;

	set<VuoNode *> nodes;
	set<VuoCable *> cables;
	vector<VuoPublishedPort *> publishedInputPorts;
	vector<VuoPublishedPort *> publishedOutputPorts;

	VuoPublishedPort * getPublishedPortWithName(string name, bool isInput);
};

#endif // VUOCOMPOSITION_HH
