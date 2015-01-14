/**
 * @file
 * VuoCompilerEdge interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEREDGE_H
#define VUOCOMPILEREDGE_H

#include "VuoCompilerNode.hh"


/**
 * This class represents one or more cables connecting a pair of nodes (or one node in a feedback loop).
 */
class VuoCompilerEdge
{
protected:
	VuoCompilerNode *fromNode; ///< The node from which this edge is output.
	VuoCompilerNode *toNode; ///< The node to which this edge is input.
	set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> > cables; ///< One or more cables comprising this edge.

	VuoCompilerEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode);
	virtual ~VuoCompilerEdge(void);  ///< to make this class dynamic_cast-able
	void generateTransmissionThroughCable(Module *module, BasicBlock *block, Value *outputDataValue, VuoCompilerInputEventPort *inputEventPort);

public:
	VuoCompilerNode * getFromNode(void);
	VuoCompilerNode * getToNode(void);
	bool mayTransmitThroughNode(void);
};

#endif
