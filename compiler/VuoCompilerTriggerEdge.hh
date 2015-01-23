/**
 * @file
 * VuoCompilerTriggerEdge interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGEREDGE_H
#define VUOCOMPILERTRIGGEREDGE_H

#include "VuoCompilerEdge.hh"
#include "VuoCompilerTriggerPort.hh"

/**
 * This class represents one or more cables connecting one node's trigger port to another
 * (or the same) node's input ports.
 */
class VuoCompilerTriggerEdge : public VuoCompilerEdge
{
public:
	VuoCompilerTriggerEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode,
							VuoCompilerTriggerPort *fromTrigger, const set<VuoCompilerInputEventPort *> &toPorts);
	void generateTransmission(Module *module, BasicBlock *block, Value *triggerDataValue);
	VuoCompilerTriggerPort * getTrigger(void);
	set<VuoCompilerInputEventPort *> getInputPorts(void);
};

#endif
