/**
 * @file
 * VuoCompilerNode implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInstanceData.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"


/**
 * Creates a compiler detail for the specified @c baseNode.
 */
VuoCompilerNode::VuoCompilerNode(VuoNode *baseNode)
	: VuoBaseDetail<VuoNode>("VuoNode", baseNode)
{
	getBase()->setCompiler(this);

	instanceData = NULL;
	VuoCompilerInstanceDataClass *instanceDataClass = getBase()->getNodeClass()->getCompiler()->getInstanceDataClass();
	if (instanceDataClass)
		instanceData = new VuoCompilerInstanceData(instanceDataClass);

	graphvizIdentifier = getGraphvizIdentifierPrefix();

	constantsCache = NULL;
	indexInOrderedNodes = ULONG_MAX - 1;
}

/**
 * Tells the node its index in VuoCompilerBitcodeGenerator::orderedNodes. This must be called before generating bitcode.
 */
void VuoCompilerNode::setIndexInOrderedNodes(size_t indexInOrderedNodes)
{
	this->indexInOrderedNodes = indexInOrderedNodes;
}

/**
 * Returns the node's index in VuoCompilerBitcodeGenerator::orderedNodes.
 */
size_t VuoCompilerNode::getIndexInOrderedNodes(void)
{
	return indexInOrderedNodes;
}

/**
 * Sets the cache used to generate constant string values. This must be called before generating bitcode.
 */
void VuoCompilerNode::setConstantsCache(VuoCompilerConstantsCache *constantsCache)
{
	this->constantsCache = constantsCache;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
		port->setConstantsCache(constantsCache);
	}
}

/**
 * Generates a constant `char *` value consisting of the node's identifier.
 */
Value * VuoCompilerNode::generateIdentifierValue(Module *module)
{
	return constantsCache->get(getIdentifier());
}

/**
 * Generates a `char *` value prefixed with @a compositionIdentifierValue and suffixed with this node's identifier.
 * The caller is responsible for generating code to free the value.
 *
 * This needs to be kept in sync with @ref VuoStringUtilities::buildCompositionIdentifier().
 */
Value * VuoCompilerNode::generateSubcompositionIdentifierValue(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	vector<Value *> identifierParts;
	identifierParts.push_back(compositionIdentifierValue);
	identifierParts.push_back(constantsCache->get("/" + getIdentifier()));
	return VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, identifierParts, constantsCache);
}

/**
 * Generates code to look up the `NodeContext *` indexed under this node's full identifier (prefixed with the composition's identifier).
 */
Value * VuoCompilerNode::generateGetContext(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	return VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, compositionStateValue, indexInOrderedNodes);
}

/**
 * Generates code to register metadata for this node and each of its ports with the runtime.
 * If this node is a subcomposition, the generated code calls `compositionAddNodeMetadata()` for the subcomposition.
 */
void VuoCompilerNode::generateAddMetadata(Module *module, BasicBlock *block, Value *compositionStateValue,
										  const vector<VuoCompilerType *> &orderedTypes,
										  Function *compositionCreateContextForNode,
										  Function *compositionSetPortValueFunction, Function *compositionGetPortValueFunction,
										  Function *compositionFireTriggerPortEventFunction, Function *compositionReleasePortDataFunction)
{
	Function *subcompositionFunctionSrc = getBase()->getNodeClass()->getCompiler()->getCompositionAddNodeMetadataFunction();

	Value *nodeIdentifierValue = generateIdentifierValue(module);
	VuoCompilerCodeGenUtilities::generateAddNodeMetadata(module, block, compositionStateValue, nodeIdentifierValue,
														 compositionCreateContextForNode, compositionSetPortValueFunction,
														 compositionGetPortValueFunction, compositionFireTriggerPortEventFunction,
														 compositionReleasePortDataFunction);

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );

		Value *portIdentifierValue = constantsCache->get(port->getIdentifier());
		Value *portNameValue = constantsCache->get(port->getBase()->getClass()->getName());

		size_t typeIndex;
		string initialValue;

		VuoType *dataType = port->getDataVuoType();
		if (dataType)
		{
			vector<VuoCompilerType *>::const_iterator typeIter = find(orderedTypes.begin(), orderedTypes.end(), dataType->getCompiler());
			typeIndex = typeIter - orderedTypes.begin();

			VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(port);
			initialValue = (inputEventPort ? inputEventPort->getData()->getInitialValue() : "");
		}
		else
		{
			typeIndex = orderedTypes.size();
		}

		Value *initialValueValue = constantsCache->get(initialValue);

		VuoCompilerCodeGenUtilities::generateAddPortMetadata(module, block, compositionStateValue, portIdentifierValue,
															 portNameValue, typeIndex, initialValueValue);
	}

	if (subcompositionFunctionSrc)
	{
		Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateRuntimeState(module, block, compositionStateValue);
		Value *compositionIdentifierValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateCompositionIdentifier(module, block, compositionStateValue);
		Value *subcompositionIdentifierValue = generateSubcompositionIdentifierValue(module, block, compositionIdentifierValue);
		Value *subcompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, runtimeStateValue, subcompositionIdentifierValue);

		Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);
		Value *arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(subcompositionStateValue, subcompositionFunctionDst, 0, NULL, module, block);
		CallInst::Create(subcompositionFunctionDst, arg, "", block);

		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, subcompositionStateValue);
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, subcompositionIdentifierValue);
	}
}

/**
 * Generates code to create the node context for this node and the port context for each of its ports.
 */
Value * VuoCompilerNode::generateCreateContext(Module *module, BasicBlock *block)
{
	// Create the node context.

	bool isSubcomposition = getBase()->getNodeClass()->getCompiler()->isSubcomposition();
	unsigned long publishedOutputPortCount = (isSubcomposition ?
												  getBase()->getOutputPorts().size() - VuoNodeClass::unreservedOutputPortStartIndex : 0);

	Value * nodeContextValue = VuoCompilerCodeGenUtilities::generateCreateNodeContext(module, block, instanceData, isSubcomposition, publishedOutputPortCount);

	// Create each port's context.

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	vector<Value *> portContextValues;
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );

		Value *portContextValue = port->generateCreatePortContext(module, block);
		portContextValues.push_back(portContextValue);
	}

	VuoCompilerCodeGenUtilities::generateSetNodeContextPortContexts(module, block, nodeContextValue, portContextValues);

	return nodeContextValue;
}

/**
 * Generates a call to @c nodeEvent or @c nodeInstanceEvent.
 *
 * The generated code sets up the arguments for the call, does the call, transmits events from input ports to output ports,
 * and handles memory management for the node's runtime representation.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param function The function in which to generate code.
 * @param currentBlock The block in which to generate code.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateEventFunctionCall(Module *module, Function *function, BasicBlock *&currentBlock,
												Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getEventFunction();

	Value *nodeContextValue = generateGetContext(module, currentBlock, compositionStateValue);

	map<VuoCompilerEventPort *, Value *> portContextForEventPort;
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>((*i)->getCompiler());
		if (eventPort)
			portContextForEventPort[eventPort] = eventPort->generateGetPortContext(module, currentBlock, nodeContextValue);
	}

	generateFunctionCall(functionSrc, module, currentBlock, compositionStateValue, nodeContextValue, portContextForEventPort);

	// The output port should transmit an event if any non-blocking input port received the event.
	// The output port should not transmit an event if no non-blocking or door input ports received the event.
	// Otherwise, the output port's event transmission is handled by the node class implementation.

	vector<VuoPort *> nonBlockingInputPorts;
	vector<VuoPort *> doorInputPorts;
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoPortClass::EventBlocking eventBlocking = (*i)->getClass()->getEventBlocking();
		if (eventBlocking == VuoPortClass::EventBlocking_None)
			nonBlockingInputPorts.push_back(*i);
		else if (eventBlocking == VuoPortClass::EventBlocking_Door)
			doorInputPorts.push_back(*i);
	}

	Value *eventHitNonBlockingInputPort = generateReceivedEventCondition(module, currentBlock, nodeContextValue, nonBlockingInputPorts,
																		 portContextForEventPort);
	Value *transmitForAllOutputPorts = eventHitNonBlockingInputPort;

	BasicBlock *handleTransmissionBlock = NULL;
	BasicBlock *nextBlock = NULL;
	if (! doorInputPorts.empty())
	{
		handleTransmissionBlock = BasicBlock::Create(module->getContext(), "handleTransmission", function, 0);
		nextBlock = BasicBlock::Create(module->getContext(), "next", function, 0);

		Value *eventHitDoorInputPort = generateReceivedEventCondition(module, currentBlock, nodeContextValue, doorInputPorts,
																	  portContextForEventPort);

		Value *blockForAllOutputPorts = BinaryOperator::Create(Instruction::And,
															   BinaryOperator::CreateNot(eventHitNonBlockingInputPort, "", currentBlock),
															   BinaryOperator::CreateNot(eventHitDoorInputPort, "", currentBlock),
															   "", currentBlock);
		Value *handleTransmission = BinaryOperator::Create(Instruction::Or, transmitForAllOutputPorts, blockForAllOutputPorts, "", currentBlock);

		Constant *zeroValue = ConstantInt::get(transmitForAllOutputPorts->getType(), 0);
		ICmpInst *handleTransmissionComparison = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, handleTransmission, zeroValue, "");
		BranchInst::Create(handleTransmissionBlock, nextBlock, handleTransmissionComparison, currentBlock);
	}
	else
	{
		handleTransmissionBlock = currentBlock;
	}

	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			Value *portContextValue = portContextForEventPort[eventPort];
			eventPort->generateStoreEvent(module, handleTransmissionBlock, nodeContextValue, transmitForAllOutputPorts, portContextValue);
		}
	}

	if (! doorInputPorts.empty())
	{
		BranchInst::Create(nextBlock, handleTransmissionBlock);
		currentBlock = nextBlock;
	}
}

/**
 * Generates a call to @c nodeInstanceInit, stores the return value into the node's instance data,
 * and retains the node's instance data.
 *
 * Assumes the node is stateful. Assumes any trigger ports' function declarations have been generated.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateInitFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getInitFunction();

	Value *nodeContextValue = generateGetContext(module, block, compositionStateValue);
	CallInst *call = generateFunctionCall(functionSrc, module, block, compositionStateValue, nodeContextValue);

	Type *instanceDataType = instanceData->getBase()->getClass()->getCompiler()->getType();
	Value *callCasted = VuoCompilerCodeGenUtilities::generateTypeCast(module, block, call, instanceDataType);
	instanceData->generateStore(module, block, nodeContextValue, callCasted);

	// Retain the instance data.
	Value *instanceDataValue = instanceData->generateLoad(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateRetainCall(module, block, instanceDataValue);
}

/**
 * Generates a call to @c nodeInstanceFini and releases the node's instance data.
 *
 * Assumes the node is stateful.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateFiniFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getFiniFunction();

	Value *nodeContextValue = generateGetContext(module, block, compositionStateValue);
	if (functionSrc)
		generateFunctionCall(functionSrc, module, block, compositionStateValue, nodeContextValue);

	// Release the instance data.
	Value *instanceDataValue = instanceData->generateLoad(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, instanceDataValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerStart, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateCallbackStartFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStartFunction();
	if (! functionSrc)
		return;

	Value *nodeContextValue = generateGetContext(module, block, compositionStateValue);
	generateFunctionCall(functionSrc, module, block, compositionStateValue, nodeContextValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerUpdate, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackUpdateFunction();
	if (! functionSrc)
		return;

	Value *nodeContextValue = generateGetContext(module, block, compositionStateValue);
	generateFunctionCall(functionSrc, module, block, compositionStateValue, nodeContextValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerStop, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @throw VuoCompilerException Failed to generate the call, possibly due to a bug in the compiler or node class.
 */
void VuoCompilerNode::generateCallbackStopFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStopFunction();
	if (! functionSrc)
		return;

	Value *nodeContextValue = generateGetContext(module, block, compositionStateValue);
	generateFunctionCall(functionSrc, module, block, compositionStateValue, nodeContextValue);
}

/**
 * Sets up the arguments to pass to the node class's event, init, callback start, or callback update function.
 *
 * @param functionSrc The node class's function in the source module.
 * @param module The module in which to generate code (destination module).
 * @param block The block in which to generate code.
 * @param compositionStateValue The `VuoCompositionState *` of the composition containing this node.
 * @param nodeContextValue This node's context.
 * @throw VuoCompilerException One of the arguments could not be generated, possibly due to a bug in the compiler
 *     or the node class. (At least throwing an exception prevents a crash / loss of unsaved changes.)
 */
CallInst * VuoCompilerNode::generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block,
												 Value *compositionStateValue, Value *nodeContextValue,
												 const map<VuoCompilerEventPort *, Value *> &portContextForEventPort)
{
	Function *functionDst = VuoCompilerModule::declareFunctionInModule(module, functionSrc);

	vector<Value *> args(functionDst->getFunctionType()->getNumParams());

	// Set up the argument for the composition state (if a subcomposition).
	Value *subcompositionIdentifierValue = NULL;
	Value *subcompositionStateValue = NULL;
	if (getBase()->getNodeClass()->getCompiler()->isSubcomposition())
	{
		Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateRuntimeState(module, block, compositionStateValue);
		Value *compositionIdentifierValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateCompositionIdentifier(module, block, compositionStateValue);
		subcompositionIdentifierValue = generateSubcompositionIdentifierValue(module, block, compositionIdentifierValue);
		subcompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, runtimeStateValue, subcompositionIdentifierValue);
		args[0] = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(subcompositionStateValue, functionDst, 0, NULL, module, block);
	}

	// Set up the arguments for input ports.
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());

		VuoCompilerInputData *data = eventPort->getData();
		bool isEventArgumentInFunction = isArgumentInFunction(eventPort, functionSrc);
		bool isDataArgumentInFunction = data && isArgumentInFunction(data, functionSrc);

		if (isEventArgumentInFunction || isDataArgumentInFunction)
		{
			map<VuoCompilerEventPort *, Value *>::const_iterator iter = portContextForEventPort.find(eventPort);
			Value *portContextValue = (iter != portContextForEventPort.end() ?
												   iter->second :
												   eventPort->generateGetPortContext(module, block, nodeContextValue));

			if (isEventArgumentInFunction)
			{
				size_t index = getArgumentIndexInFunction(eventPort, functionSrc);
				args[index] = eventPort->generateLoadEvent(module, block, nodeContextValue, portContextValue);
			}

			if (isDataArgumentInFunction)
			{
				size_t index = getArgumentIndexInFunction(data, functionSrc);
				Value *arg = eventPort->generateLoadData(module, block, nodeContextValue, portContextValue);
				Value *secondArg = NULL;

				// If we're calling a node function for a subcomposition, and the parameter is a struct that
				// would normally be passed "byval", instead generate code equivalent to the "byval" semantics:
				// Allocate a stack variable, copy the struct into it, and pass the variable's address as
				// the argument to the node function.
				//
				// This is a workaround for a bug where LLVM would sometimes give the node function body
				// an invalid value for a "byval" struct argument. https://b33p.net/kosada/node/11386
				if (getBase()->getNodeClass()->getCompiler()->isSubcomposition() &&
						arg->getType()->isStructTy() &&
						eventPort->getDataVuoType()->getCompiler()->getFunctionParameterAttributes().hasAttrSomewhere(Attribute::ByVal))
				{
					Value *argAsPointer = VuoCompilerCodeGenUtilities::generatePointerToValue(block, arg);
					arg = new BitCastInst(argAsPointer, functionSrc->getFunctionType()->getParamType(index), "", block);
				}
				else
				{
					bool isLoweredToTwoParameters = static_cast<VuoCompilerInputDataClass *>(data->getBase()->getClass()->getCompiler())->isLoweredToTwoParameters();
					Value **secondArgIfNeeded = (isLoweredToTwoParameters ? &secondArg : NULL);
					arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, functionDst, index, secondArgIfNeeded, module, block);
					if (!arg)
					{
						VUserLog("Warning: Couldn't convert argument for input port '%s' of function %s; not generating call.",
								 (*i)->getClass()->getName().c_str(),
								 functionDst->getName().str().c_str());
						return nullptr;
					}
				}

				args[index] = arg;
				if (secondArg)
					args[index + 1] = secondArg;
			}
		}
	}

	// Set up the arguments for output ports.
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	map<VuoCompilerOutputEventPort *, Value *> outputPortDataVariables;
	map<VuoCompilerOutputEventPort *, AllocaInst *> outputPortEventVariables;
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			VuoCompilerOutputData *data = eventPort->getData();
			bool isEventArgumentInFunction = isArgumentInFunction(eventPort, functionSrc);
			bool isDataArgumentInFunction = data && isArgumentInFunction(data, functionSrc);

			if (isEventArgumentInFunction)
			{
				size_t index = getArgumentIndexInFunction(eventPort, functionSrc);
				PointerType *eventPointerType = static_cast<PointerType *>( functionDst->getFunctionType()->getParamType(index) );
				AllocaInst *arg = new AllocaInst(eventPointerType->getElementType(), "", block);
				new StoreInst(ConstantInt::get(eventPointerType->getElementType(), 0), arg, block);
				outputPortEventVariables[eventPort] = arg;
				args[index] = arg;
			}

			if (isDataArgumentInFunction)
			{
				map<VuoCompilerEventPort *, Value *>::const_iterator iter = portContextForEventPort.find(eventPort);
				Value *portContextValue = (iter != portContextForEventPort.end() ?
													   iter->second :
													   eventPort->generateGetPortContext(module, block, nodeContextValue));

				size_t index = getArgumentIndexInFunction(data, functionSrc);
				Type *type = eventPort->getDataVuoType()->getCompiler()->getType();
				Value *dataVariable = VuoCompilerCodeGenUtilities::generateGetPortContextDataVariable(module, block, portContextValue, type);
				outputPortDataVariables[eventPort] = dataVariable;

				Value *arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(dataVariable, functionDst, index, NULL, module, block);
				if (!arg)
				{
					VUserLog("Warning: Couldn't convert argument for output port '%s' of function %s; not generating call.",
							 (*i)->getClass()->getName().c_str(),
							 functionDst->getName().str().c_str());
					return nullptr;
				}
				args[index] = arg;
			}
		}
		else
		{
			VuoCompilerTriggerPort *triggerPort = (VuoCompilerTriggerPort *)((*i)->getCompiler());
			if (isArgumentInFunction(triggerPort, functionSrc))
			{
				size_t index = getArgumentIndexInFunction(triggerPort, functionSrc);
				args[index] = triggerPort->generateLoadFunction(module, block, nodeContextValue);
			}
		}
	}

	// Set up the argument for the instance data (if it exists).
	Value *instanceDataVariable = NULL;
	if (instanceData && isArgumentInFunction(instanceData, functionSrc))
	{
		Type *type = instanceData->getBase()->getClass()->getCompiler()->getType();
		instanceDataVariable = VuoCompilerCodeGenUtilities::generateGetNodeContextInstanceDataVariable(module, block, nodeContextValue, type);

		size_t index = getArgumentIndexInFunction(instanceData, functionSrc);
		Value *arg = instanceDataVariable;
		args[index] = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, functionDst, index, NULL, module, block);
		if (!args[index])
		{
			VUserLog("Warning: Couldn't convert argument for instance data of function %s; not generating call.", functionDst->getName().str().c_str());
			return nullptr;
		}
	}

	// Save the old output port values so they can be released later.
	map<VuoCompilerOutputEventPort *, Value *> oldOutputDataValues;
	for (map<VuoCompilerOutputEventPort *, Value *>::iterator i = outputPortDataVariables.begin(); i != outputPortDataVariables.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = i->first;
		Value *dataVariable = i->second;

		oldOutputDataValues[eventPort] = new LoadInst(dataVariable, "", false, block);
	}

	// Save the old instance data value so it can be released later.
	Value *oldInstanceDataValue = NULL;
	if (instanceDataVariable)
	{
		oldInstanceDataValue = new LoadInst(instanceDataVariable, "", false, block);
	}

	VuoCompilerCodeGenUtilities::generateAddCompositionStateToThreadLocalStorage(module, block, compositionStateValue);

	{
		int i = 0;
		for (auto arg : args)
		{
			if (!arg)
			{
				string s;
				raw_string_ostream type(s);
				functionDst->getFunctionType()->getParamType(i)->print(type);
				ostringstream argIndex;
				argIndex << i;
				string details = "When trying to generate a call to function " + functionDst->getName().str() +
								 ", argument " + argIndex.str() + " (" + type.str() + ") was missing.";
				VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling composition", "", "", details);
				throw VuoCompilerException(issue);
			}
			++i;
		}
	}

	// Call the node class's function.
	CallInst *call = CallInst::Create(functionDst, args, "", block);

	VuoCompilerCodeGenUtilities::generateRemoveCompositionStateFromThreadLocalStorage(module, block);

	// Save the output events from temporary variables to the ports.
	// (The temporary variables are needed to avoid mismatched types between the parameters and the arguments, e.g. i8* vs. i64*.)
	for (map<VuoCompilerOutputEventPort *, AllocaInst *>::iterator i = outputPortEventVariables.begin(); i != outputPortEventVariables.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = i->first;
		AllocaInst *eventVariable = i->second;
		Value *eventValue = new LoadInst(eventVariable, "", false, block);
		eventPort->generateStoreEvent(module, block, nodeContextValue, eventValue);
	}

	// Retain the new output port values.
	for (map<VuoCompilerOutputEventPort *, Value *>::iterator i = outputPortDataVariables.begin(); i != outputPortDataVariables.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = i->first;
		Value *dataVariable = i->second;

		Value *outputDataValue = new LoadInst(dataVariable, "", false, block);
		VuoCompilerType *type = eventPort->getDataVuoType()->getCompiler();
		type->generateRetainCall(module, block, outputDataValue);
	}

	// Release the old output port values.
	for (map<VuoCompilerOutputEventPort *, Value *>::iterator i = oldOutputDataValues.begin(); i != oldOutputDataValues.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = i->first;
		Value *oldOutputDataValue = i->second;

		VuoCompilerType *type = eventPort->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, oldOutputDataValue);
	}

	if (instanceDataVariable)
	{
		// Retain the new instance data value.
		Value *instanceDataValue = new LoadInst(instanceDataVariable, "", false, block);
		VuoCompilerCodeGenUtilities::generateRetainCall(module, block, instanceDataValue);

		// Release the old instance data value.
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, oldInstanceDataValue);
	}

	if (getBase()->getNodeClass()->getCompiler()->isSubcomposition())
	{
		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, subcompositionStateValue);
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, subcompositionIdentifierValue);
	}

	return call;
}

/**
 * Returns true if the argument matches one of the parameters in the specified node class function.
 *
 * @param argument The argument to check.
 * @param function The node class's event, init, fini, callback start, callback update, or callback stop function.
 */
bool VuoCompilerNode::isArgumentInFunction(VuoCompilerNodeArgument *argument, Function *function)
{
	VuoCompilerNodeArgumentClass *argumentClass = argument->getBase()->getClass()->getCompiler();
	if (function == getBase()->getNodeClass()->getCompiler()->getEventFunction())
		return argumentClass->isInEventFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getInitFunction())
		return argumentClass->isInInitFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackStartFunction())
		return argumentClass->isInCallbackStartFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackUpdateFunction())
		return argumentClass->isInCallbackUpdateFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackStopFunction())
		return argumentClass->isInCallbackStopFunction();
	else if (instanceData == argument &&
			 (function == getBase()->getNodeClass()->getCompiler()->getFiniFunction()))
		return true;
	return false;
}

/**
 * Returns the index of the parameter matching the argument in the specified node class function.
 *
 * @param argument The argument to check.
 * @param function The node class's event, init, fini, callback start, callback update, or callback stop function.
 */
size_t VuoCompilerNode::getArgumentIndexInFunction(VuoCompilerNodeArgument *argument, Function *function)
{
	VuoCompilerNodeArgumentClass *argumentClass = argument->getBase()->getClass()->getCompiler();
	if (function == getBase()->getNodeClass()->getCompiler()->getEventFunction())
		return argumentClass->getIndexInEventFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getInitFunction())
		return argumentClass->getIndexInInitFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getFiniFunction())
		return (getBase()->getNodeClass()->getCompiler()->isSubcomposition() ? 1 : 0);
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackStartFunction())
		return argumentClass->getIndexInCallbackStartFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackUpdateFunction())
		return argumentClass->getIndexInCallbackUpdateFunction();
	else if (function == getBase()->getNodeClass()->getCompiler()->getCallbackStopFunction())
		return argumentClass->getIndexInCallbackStopFunction();
	return 0;
}

/**
 * Generates a condition that is true if any of this node's input event ports have received an event.
 */
Value * VuoCompilerNode::generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	return generateReceivedEventCondition(module, block, nodeContextValue, getBase()->getInputPorts());
}

/**
 * Generates a condition that is true if any of the given input event ports of this node have received an event.
 */
Value * VuoCompilerNode::generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue,
														vector<VuoPort *> selectedInputPorts,
														const map<VuoCompilerEventPort *, Value *> &portContextForEventPort)
{
	Value *conditionValue = ConstantInt::getFalse(block->getContext());
	for (vector<VuoPort *>::iterator i = selectedInputPorts.begin(); i != selectedInputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			map<VuoCompilerEventPort *, Value *>::const_iterator iter = portContextForEventPort.find(eventPort);
			Value *portContextValue = (iter != portContextForEventPort.end() ?
												   iter->second :
												   eventPort->generateGetPortContext(module, block, nodeContextValue));

			Value *pushValue = eventPort->generateLoadEvent(module, block, nodeContextValue, portContextValue);
			conditionValue = BinaryOperator::Create(Instruction::Or, conditionValue, pushValue, "", block);
		}
	}
	return conditionValue;
}

/**
 * If this node is stateful, returns the instance data.  Otherwise null.
 */
VuoCompilerInstanceData * VuoCompilerNode::getInstanceData(void)
{
	return instanceData;
}

/**
 * Returns the identifier prefix in the generated LLVM bitcode for this node instance.
 * This identifier is derived from the Graphviz identifier.
 *
 * @eg{Add2}
 */
string VuoCompilerNode::getIdentifier(void)
{
	return getGraphvizIdentifier();
}

/**
 * Returns a suggested UpperCamelCase prefix for the node's Graphviz identifier, based on its title.
 *
 * Removes non-alphanumeric characters, and uppercases the character after each former space.
 * Removes non-alpha leading characters.
 *
 * \eg{Node title "42 Fire on Start 42!" becomes `FireOnStart42`.}
 */
string VuoCompilerNode::getGraphvizIdentifierPrefix(void)
{
	string titleWithoutSpaces = VuoStringUtilities::convertToCamelCase(getBase()->getTitle(), true, false, false);

	if (titleWithoutSpaces.empty())
		return "Node";
	else
		return titleWithoutSpaces;
}

/**
 * Sets the identifier that will appear in .vuo (Graphviz dot format) files.
 */
void VuoCompilerNode::setGraphvizIdentifier(string graphvizIdentifier)
{
	this->graphvizIdentifier = graphvizIdentifier;
}

/**
 * Gets the identifier that will appear in .vuo (Graphviz dot format) files.
 */
string VuoCompilerNode::getGraphvizIdentifier(void)
{
	return graphvizIdentifier;
}

/**
 * Returns a string containing the declaration for this node
 * as it would appear in a .vuo (Graphviz dot format) file.
 */
string VuoCompilerNode::getGraphvizDeclaration(bool shouldPrintPosition, double xPositionOffset, double yPositionOffset,
											   VuoPort *manuallyFirableInputPort)
{
	ostringstream declaration;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();

	// name
	declaration << getGraphvizIdentifier();

	// type
	declaration << " [type=\"" << getBase()->getNodeClass()->getClassName() << "\"";

	// version
	declaration << " version=\"" << getBase()->getNodeClass()->getVersion() << "\"";

	// label
	declaration << " label=\"" << VuoStringUtilities::transcodeToGraphvizIdentifier(getBase()->getTitle());
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		string portName = VuoStringUtilities::transcodeToGraphvizIdentifier((*i)->getClass()->getName());
		declaration << "|<" << portName << ">" << portName << "\\l";
	}
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		string portName = VuoStringUtilities::transcodeToGraphvizIdentifier((*i)->getClass()->getName());
		declaration << "|<" << portName << ">" << portName << "\\r";
	}
	declaration << "\"";

	// position
	if (shouldPrintPosition)
		declaration << " pos=\"" << getBase()->getX()+xPositionOffset << "," << getBase()->getY()+yPositionOffset << "\"";

	// collapsed
	if (getBase()->isCollapsed())
		declaration << " collapsed";

	// tint color
	if (getBase()->getTintColor() != VuoNode::TintNone)
		declaration << " fillcolor=\"" << getBase()->getTintColorGraphvizName() << "\"";

	// constant values
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoPort *port = *i;
		VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>(port->getCompiler());
		VuoCompilerInputData *data = eventPort->getData();
		if (data && (! eventPort->hasConnectedDataCable() && ! data->getInitialValue().empty()))
		{
			string portConstant = data->getInitialValue();
			string escapedPortConstant = VuoStringUtilities::transcodeToGraphvizIdentifier(portConstant);
			declaration << " _" << port->getClass()->getName() << "=\"" << escapedPortConstant << "\"";
		}
	}

	// event throttling
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoPort *port = *i;
		if (port->getClass()->getPortType() == VuoPortClass::triggerPort)
		{
			string eventThrottling = (port->getEventThrottling() == VuoPortClass::EventThrottling_Enqueue ? "enqueue" : "drop");
			declaration << " _" << port->getClass()->getName() << "_eventThrottling=\"" << eventThrottling << "\"";
		}
	}

	// manually firable input port
	if (manuallyFirableInputPort)
		declaration << " _" << manuallyFirableInputPort->getClass()->getName() << "_manuallyFirable";

	declaration << "];";

	return declaration.str();
}
