/**
 * @file
 * VuoCompilerEdge implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerEdge.hh"
#include "VuoCompilerOutputEventPort.hh"

#include "VuoPort.hh"

/**
 * Creates an edge from @c fromNode to @c toNode.
 */
VuoCompilerEdge::VuoCompilerEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode)
{
	this->fromNode = fromNode;
	this->toNode = toNode;
}

VuoCompilerEdge::~VuoCompilerEdge(void)
{
}

/**
 * Returns true if an event coming into a node through this edge may ever be transmitted to the node's output ports.
 */
bool VuoCompilerEdge::mayTransmitThroughNode(void)
{
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCompilerInputEventPort *inputPort = i->second;
		VuoPortClass::EventBlocking eventBlocking = inputPort->getBase()->getClass()->getEventBlocking();
		if (eventBlocking != VuoPortClass::EventBlocking_Wall)
			return true;
	}

	return false;
}

/**
 * Generates code to transmit the data (if any) and an event from an output port to an input port.
 */
void VuoCompilerEdge::generateTransmissionThroughCable(Module *module, BasicBlock *block, Value *outputDataValue, VuoCompilerInputEventPort *inputEventPort)
{
	if (outputDataValue)
	{
		VuoCompilerInputData *inputData = inputEventPort->getData();
		LoadInst *oldInputDataValue = oldInputDataValue = inputData->generateLoad(block);

		// If needed, retain the new input port value.
		VuoCompilerCodeGenUtilities::generateRetainCall(module, block, outputDataValue);

		// Store the new value in the input port.
		inputData->generateStore(outputDataValue, block);

		// If needed, release the old input port value.
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, oldInputDataValue);
	}

	// Push the input port.
	inputEventPort->generateStore(true, block);
}

/**
 * Returns the node from which this edge is output.
 */
VuoCompilerNode * VuoCompilerEdge::getFromNode(void)
{
	return fromNode;
}

/**
 * Returns the node to which this edge is input.
 */
VuoCompilerNode * VuoCompilerEdge::getToNode(void)
{
	return toNode;
}
