/**
 * @file
 * VuoCompilerTriggerEdge implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTriggerEdge.hh"

#include "VuoPort.hh"

/**
 * Creates an edge from the @c fromTrigger on @c fromNode to @c toPorts on @c toNode.
 */
VuoCompilerTriggerEdge::VuoCompilerTriggerEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode,
												   VuoCompilerTriggerPort *fromTrigger, const set<VuoCompilerInputEventPort *> &toPorts) :
	VuoCompilerEdge(fromNode, toNode)
{
	for (set<VuoCompilerInputEventPort *>::iterator i = toPorts.begin(), e = toPorts.end(); i != e; ++i)
		cables.insert(make_pair(fromTrigger, *i));
}

/**
 * Generates code to transmit the data (if any) and an event through each cable in this edge.
 */
void VuoCompilerTriggerEdge::generateTransmission(Module *module, BasicBlock *block, Value *triggerDataValue)
{
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCompilerInputEventPort *inputEventPort = i->second;
		generateTransmissionThroughCable(module, block, triggerDataValue, inputEventPort);
	}
}

/**
 * Returns the trigger port in this edge.
 */
VuoCompilerTriggerPort * VuoCompilerTriggerEdge::getTrigger(void)
{
	return (cables.empty() ? NULL : (VuoCompilerTriggerPort *)cables.begin()->first);
}

/**
 * Returns the input ports connected to the trigger port by this edge.
 */
set<VuoCompilerInputEventPort *> VuoCompilerTriggerEdge::getInputPorts(void)
{
	set<VuoCompilerInputEventPort *> inputPorts;
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCompilerInputEventPort *inputPort = i->second;
		inputPorts.insert(inputPort);
	}
	return inputPorts;
}
