/**
 * @file
 * VuoCompilerPassiveEdge interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPASSIVEEDGE_H
#define VUOCOMPILERPASSIVEEDGE_H

#include "VuoCompilerEdge.hh"
#include "VuoCompilerOutputEventPort.hh"


/**
 * This class represents one or more cables connecting one node's output ports to another
 * (or the same) node's input ports.
 */
class VuoCompilerPassiveEdge : public VuoCompilerEdge
{
public:
	VuoCompilerPassiveEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode,
						   const set< pair<VuoCompilerOutputEventPort *, VuoCompilerInputEventPort *> > &cables);
	void generateTransmission(Module *module, BasicBlock *initialBlock, BasicBlock *finalBlock);
	set<VuoCompilerOutputEventPort *> getOutputPorts(void);
	set<VuoCompilerInputEventPort *> getInputPortsConnectedToOutputPort(VuoCompilerOutputEventPort *outputPort);
};

#endif
