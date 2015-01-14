/**
 * @file
 * VuoCompilerPassiveEdge implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPassiveEdge.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerOutputEventPort.hh"

#include "VuoPort.hh"

/**
 * Creates an edge from @c fromNode to @c toNode with the specified @c cables.
 */
VuoCompilerPassiveEdge::VuoCompilerPassiveEdge(VuoCompilerNode *fromNode, VuoCompilerNode *toNode,
											   const set< pair<VuoCompilerOutputEventPort *, VuoCompilerInputEventPort *> > &cables) :
VuoCompilerEdge(fromNode, toNode)
{
	for (set< pair<VuoCompilerOutputEventPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(), e = cables.end(); i != e; ++i)
		this->cables.insert(*i);
}

/**
 * Generates code to transmit the data (if any) and event (if any) through each cable in this edge.
 */
void VuoCompilerPassiveEdge::generateTransmission(Module *module, BasicBlock *initialBlock, BasicBlock *finalBlock)
{
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCompilerOutputEventPort *outputEventPort = (VuoCompilerOutputEventPort *)i->first;
		VuoCompilerInputEventPort *inputEventPort = i->second;

		// If the output port transmitted an event...
		bool isDonePort = outputEventPort->getBase() == fromNode->getBase()->getDonePort();
		BasicBlock *transmissionBlock = NULL;
		BasicBlock *noTransmissionBlock = NULL;
		if (! isDonePort)
		{
			transmissionBlock = BasicBlock::Create(module->getContext(), "", initialBlock->getParent(), NULL);
			noTransmissionBlock = BasicBlock::Create(module->getContext(), "", initialBlock->getParent(), NULL);

			LoadInst *eventValue = outputEventPort->generateLoad(initialBlock);
			ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
			ICmpInst *eventValueIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
			BranchInst::Create(transmissionBlock, noTransmissionBlock, eventValueIsTrue, initialBlock);
		}
		else
		{
			transmissionBlock = initialBlock;
		}

		// ... transmit the event and data (if any).
		VuoCompilerOutputData *outputData = outputEventPort->getData();
		Value *outputDataValue = (outputData ? outputData->generateLoad(transmissionBlock) : NULL);
		generateTransmissionThroughCable(module, transmissionBlock, outputDataValue, inputEventPort);

		if (! isDonePort)
		{
			BranchInst::Create(noTransmissionBlock, transmissionBlock);
			initialBlock = noTransmissionBlock;
		}
	}

	BranchInst::Create(finalBlock, initialBlock);
}

/**
 * Returns the output ports attached to cables in this edge.
 */
set<VuoCompilerOutputEventPort *> VuoCompilerPassiveEdge::getOutputPorts(void)
{
	set <VuoCompilerOutputEventPort *> outputPorts;
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCompilerOutputEventPort *outputPort = static_cast<VuoCompilerOutputEventPort *>(i->first);
		outputPorts.insert(outputPort);
	}
	return outputPorts;
}

/**
 * Returns the input ports connected to the given output port by this edge.
 */
set<VuoCompilerInputEventPort *> VuoCompilerPassiveEdge::getInputPortsConnectedToOutputPort(VuoCompilerOutputEventPort *outputPort)
{
	set<VuoCompilerInputEventPort *> inputPorts;
	for (set< pair<VuoCompilerPort *, VuoCompilerInputEventPort *> >::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		if (i->first == outputPort)
		{
			VuoCompilerInputEventPort *inputPort = i->second;
			inputPorts.insert(inputPort);
		}
	}
	return inputPorts;
}
