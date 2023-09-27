/**
 * @file
 * VuoComposition interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBase.hh"

class VuoCompilerComposition;
class VuoRendererComposition;
class VuoCable;
class VuoComment;
class VuoCompositionMetadata;
class VuoNode;
class VuoProtocol;
class VuoPublishedPort;

/**
 * A collection of nodes and the cables connecting them.
 */
class VuoComposition : public VuoBase<VuoCompilerComposition,VuoRendererComposition>
{
public:
	VuoComposition(void);
	~VuoComposition(void);

	void setMetadata(VuoCompositionMetadata *metadata, bool takeOwnership);
	VuoCompositionMetadata * getMetadata(void);

	void setDirectory(string directory);
	string getDirectory(void);

	void addNode(VuoNode *node);
	void removeNode(VuoNode *node);
	set<VuoNode *> getNodes(void);

	void addCable(VuoCable *cable);
	void removeCable(VuoCable *cable);
	set<VuoCable *> getCables(void);

	void addComment(VuoComment *comment);
	void removeComment(VuoComment *comment);
	set<VuoComment *> getComments(void);

	void addPublishedInputPort(VuoPublishedPort *port, int index);
	void addPublishedOutputPort(VuoPublishedPort *port, int index);
	void removePublishedInputPort(int index);
	void removePublishedOutputPort(int index);
	vector<VuoPublishedPort *> getPublishedInputPorts(void);
	vector<VuoPublishedPort *> getPublishedOutputPorts(void);
	VuoPublishedPort * getPublishedInputPortWithName(string name);
	VuoPublishedPort * getPublishedOutputPortWithName(string name);
	int getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput);
	vector<VuoPublishedPort *> getProtocolAwarePublishedPortOrder(VuoProtocol *protocol, bool publishedInputs);

private:
	VuoCompositionMetadata *metadata;
	bool ownsMetadata;
	string directory;

	set<VuoNode *> nodes;
	set<VuoCable *> cables;
	set<VuoComment *> comments;
	vector<VuoPublishedPort *> publishedInputPorts;
	vector<VuoPublishedPort *> publishedOutputPorts;

	VuoPublishedPort * getPublishedPortWithName(string name, bool isInput);
};
