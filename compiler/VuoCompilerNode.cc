/**
 * @file
 * VuoCompilerNode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInstanceData.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"


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

	constantStrings = NULL;
}

/**
 * Sets the cache used to generate constant string values. This must be called before generating bitcode.
 */
void VuoCompilerNode::setConstantStringCache(VuoCompilerConstantStringCache *constantStrings)
{
	this->constantStrings = constantStrings;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
		port->setConstantStringCache(constantStrings);
	}
}

/**
 * Generates a `char *` value prefixed with @a compositionIdentifierValue and suffixed with this node's identifier.
 */
Value * VuoCompilerNode::generateIdentifierValue(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	vector<Value *> identifierParts;
	identifierParts.push_back(compositionIdentifierValue);
	identifierParts.push_back(constantStrings->get(module, "__" + getGraphvizIdentifier()));
	return VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, identifierParts, *constantStrings);
}

/**
 * Generates code to look up the `NodeContext *` indexed under this node's full identifier (prefixed with the composition's identifier).
 */
Value * VuoCompilerNode::generateGetContext(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
	return nodeContextValue;
}

/**
 * Generates code to create and initialize a data structure containing port values and other information
 * specific to this node. If this node is a subcomposition, `compositionContextInit()` is called.
 *
 * The generated code registers the created node context with the runtime.
 */
Value * VuoCompilerNode::generateContextInit(Module *module, BasicBlock *block, Value *compositionIdentifierValue,
											 unsigned long nodeIndex, const vector<VuoCompilerType *> &orderedTypes)
{
	Value *nodeContextValue;
	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);

	Function *subcompositionFunctionSrc = getBase()->getNodeClass()->getCompiler()->getCompositionContextInitFunction();
	if (subcompositionFunctionSrc)
	{
		Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);
		nodeContextValue = CallInst::Create(subcompositionFunctionDst, nodeIdentifierValue, "", block);

		PointerType *pointerToNodeContextType = PointerType::get( VuoCompilerCodeGenUtilities::getNodeContextType(module), 0 );
		nodeContextValue = new BitCastInst(nodeContextValue, pointerToNodeContextType, "", block);  // cast needed since NodeContext type comes from a different module
	}
	else
	{
		nodeContextValue = VuoCompilerCodeGenUtilities::generateCreateNodeContext(module, block, instanceData, false, 0);

		VuoCompilerCodeGenUtilities::generateAddNodeContext(module, block, nodeIdentifierValue, nodeContextValue);
	}

	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);

	Value *nodeSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeContextSemaphore(module, block, nodeContextValue);
	Type *unsignedLongType = IntegerType::get(module->getContext(), 64);
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *nodeIndexValue = ConstantInt::get(unsignedLongType, nodeIndex);

	Function *setPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);
	Value *falseValue = ConstantInt::get(setPortValueFunction->getFunctionType()->getParamType(3), 0);
	Value *trueValue = ConstantInt::get(setPortValueFunction->getFunctionType()->getParamType(3), 1);

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

		Value *portIdentifierValue = constantStrings->get(module, port->getIdentifier());
		Value *portDataVariable;
		Value *typeIndexValue;

		VuoType *dataType = port->getDataVuoType();
		if (dataType)
		{
			portDataVariable = VuoCompilerCodeGenUtilities::generateGetPortContextDataVariableAsVoidPointer(module, block, portContextValue);

			vector<VuoCompilerType *>::const_iterator typeIter = find(orderedTypes.begin(), orderedTypes.end(), dataType->getCompiler());
			size_t typeIndex = typeIter - orderedTypes.begin();
			typeIndexValue = ConstantInt::get(unsignedLongType, typeIndex);
		}
		else
		{
			portDataVariable = ConstantPointerNull::get(voidPointerType);

			size_t invalidTypeIndex = orderedTypes.size();
			typeIndexValue = ConstantInt::get(unsignedLongType, invalidTypeIndex);
		}

		VuoCompilerCodeGenUtilities::generateAddPortIdentifier(module, block, compositionIdentifierValue, portIdentifierValue,
															   portDataVariable, nodeSemaphoreValue, nodeIndexValue, typeIndexValue);

		if (dataType)
		{
			VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(port);
			string initialValue = (inputEventPort ? inputEventPort->getData()->getInitialValue() : "");
			Value *initialValueValue = constantStrings->get(module, initialValue);

			vector<Value *> setPortValueArgs;
			setPortValueArgs.push_back(compositionIdentifierValue);
			setPortValueArgs.push_back(portIdentifierValue);
			setPortValueArgs.push_back(initialValueValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(trueValue);
			CallInst::Create(setPortValueFunction, setPortValueArgs, "", block);
		}
	}

	VuoCompilerCodeGenUtilities::generateSetNodeContextPortContexts(module, block, nodeContextValue, portContextValues);

	return nodeContextValue;
}

/**
 * Generates code to deallocate the data structure created by code generated by VuoCompilerNode::generateContextInit().
 * If this node is a subcomposition, `compositionContextFini()` is called.
 *
 * The generated code releases the data of each data-and-devent port, except for the input ports of nodes being carried
 * across a live-coding reload.
 */
void VuoCompilerNode::generateContextFini(Module *module, BasicBlock *block, BasicBlock *releaseInputsBlock,
										  Value *compositionIdentifierValue, Value *nodeIdentifierValue, Value *nodeContextValue)
{
	Function *setPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);
	Type *boolType = setPortValueFunction->getFunctionType()->getParamType(3);
	Value *falseValue = ConstantInt::get(boolType, 0);
	Value *trueValue = ConstantInt::get(boolType, 1);
	Constant *emptyStringValue = constantStrings->get(module, "");

	// Release the input port data.
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
		VuoType *dataType = port->getDataVuoType();
		if (dataType)
		{
			Value *portIdentifierValue = constantStrings->get(module, port->getIdentifier());

			vector<Value *> setPortValueArgs;
			setPortValueArgs.push_back(compositionIdentifierValue);
			setPortValueArgs.push_back(portIdentifierValue);
			setPortValueArgs.push_back(emptyStringValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(trueValue);
			setPortValueArgs.push_back(falseValue);
			CallInst::Create(setPortValueFunction, setPortValueArgs, "", releaseInputsBlock);
		}
	}

	// Release the output port data.
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
		VuoType *dataType = port->getDataVuoType();
		if (dataType)
		{
			Value *portIdentifierValue = constantStrings->get(module, port->getIdentifier());

			vector<Value *> setPortValueArgs;
			setPortValueArgs.push_back(compositionIdentifierValue);
			setPortValueArgs.push_back(portIdentifierValue);
			setPortValueArgs.push_back(emptyStringValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(falseValue);
			setPortValueArgs.push_back(trueValue);
			setPortValueArgs.push_back(falseValue);
			CallInst::Create(setPortValueFunction, setPortValueArgs, "", block);
		}
	}

	Function *subcompositionFunctionSrc = getBase()->getNodeClass()->getCompiler()->getCompositionContextFiniFunction();
	if (subcompositionFunctionSrc)
	{
		Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);
		CallInst::Create(subcompositionFunctionDst, nodeIdentifierValue, "", block);
	}
	else
		VuoCompilerCodeGenUtilities::generateFreeNodeContext(module, block, nodeContextValue, inputPorts.size() + outputPorts.size());
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
 * @param nodeIdentifierValue This node's fully qualified identifier.
 */
void VuoCompilerNode::generateEventFunctionCall(Module *module, Function *function, BasicBlock *&currentBlock,
												Value *nodeIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getEventFunction();

	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, currentBlock, nodeIdentifierValue);

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

	generateFunctionCall(functionSrc, module, currentBlock, nodeIdentifierValue, nodeContextValue, portContextForEventPort);

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
 * @param compositionIdentifierValue The identifier of the composition containing this node.
 */
void VuoCompilerNode::generateInitFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getInitFunction();

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	CallInst *call = generateFunctionCall(functionSrc, module, block, nodeIdentifierValue, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);

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
 * @param compositionIdentifierValue The identifier of the composition containing this node.
 */
void VuoCompilerNode::generateFiniFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getFiniFunction();

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	generateFunctionCall(functionSrc, module, block, nodeIdentifierValue, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);

	// Release the instance data.
	Value *instanceDataValue = instanceData->generateLoad(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, instanceDataValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerStart, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionIdentifierValue The identifier of the composition containing this node.
 */
void VuoCompilerNode::generateCallbackStartFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStartFunction();
	if (! functionSrc)
		return;

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	generateFunctionCall(functionSrc, module, block, nodeIdentifierValue, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerUpdate, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionIdentifierValue The identifier of the composition containing this node.
 */
void VuoCompilerNode::generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackUpdateFunction();
	if (! functionSrc)
		return;

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	generateFunctionCall(functionSrc, module, block, nodeIdentifierValue, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerStop, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param compositionIdentifierValue The identifier of the composition containing this node.
 */
void VuoCompilerNode::generateCallbackStopFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStopFunction();
	if (! functionSrc)
		return;

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
	generateFunctionCall(functionSrc, module, block, nodeIdentifierValue, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
}

/**
 * Sets up the arguments to pass to the node class's event, init, callback start, or callback update function.
 *
 * @param functionSrc The node class's function in the source module.
 * @param module The module in which to generate code (destination module).
 * @param block The block in which to generate code.
 * @param nodeIdentifierValue This node's fully qualified identifier.
 * @param nodeContextValue This node's context.
 */
CallInst * VuoCompilerNode::generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block,
												 Value *nodeIdentifierValue, Value *nodeContextValue,
												 const map<VuoCompilerEventPort *, Value *> &portContextForEventPort)
{
	Function *functionDst = VuoCompilerModule::declareFunctionInModule(module, functionSrc);

	vector<Value *> args(functionDst->getFunctionType()->getNumParams());

	// Set up the argument for the composition identifier (if a subcomposition).
	if (getBase()->getNodeClass()->getCompiler()->isSubcomposition())
		args[0] = nodeIdentifierValue;

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
						eventPort->getDataVuoType()->getCompiler()->getFunctionParameterAttributes().hasAttribute(Attributes::ByVal))
				{
					Value *argAsPointer = VuoCompilerCodeGenUtilities::generatePointerToValue(block, arg);
					arg = new BitCastInst(argAsPointer, functionSrc->getFunctionType()->getParamType(index), "", block);
				}
				else
				{
					bool isLoweredToTwoParameters = static_cast<VuoCompilerInputDataClass *>(data->getBase()->getClass()->getCompiler())->isLoweredToTwoParameters();
					Value **secondArgIfNeeded = (isLoweredToTwoParameters ? &secondArg : NULL);
					arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, functionDst, index, secondArgIfNeeded, module, block);
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

	// Call the node class's function.
	CallInst *call = CallInst::Create(functionDst, args, "", block);

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
	string title = getBase()->getTitle();
	string titleWithoutSpaces;
	bool first = true;
	bool uppercaseNext = true;
	for (string::iterator i = title.begin(); i != title.end(); ++i)
	{
		if (first && !isalpha(*i))
			continue;
		first = false;

		if (!isalnum(*i))
		{
			uppercaseNext = true;
			continue;
		}

		if (uppercaseNext)
		{
			titleWithoutSpaces += toupper(*i);
			uppercaseNext = false;
		}
		else
			titleWithoutSpaces += *i;
	}

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
string VuoCompilerNode::getGraphvizDeclaration(bool shouldPrintPosition, double xPositionOffset, double yPositionOffset)
{
	return getGraphvizDeclarationWithOptions(false, shouldPrintPosition, xPositionOffset, yPositionOffset);
}

/**
 * Returns a string containing the declaration for this node
 * as it would appear in a .vuo (Graphviz dot format) file,
 * but with printf-style placeholders for the port and instance data values.
 */
string VuoCompilerNode::getSerializedFormatString(void)
{
	return getGraphvizDeclarationWithOptions(true, false, 0, 0);
}

/**
 * Helper function for getGraphvizDeclaration() and getSerializedFormatString().
 */
string VuoCompilerNode::getGraphvizDeclarationWithOptions(bool shouldUsePlaceholders, bool shouldPrintPosition, double xPositionOffset, double yPositionOffset)
{
	ostringstream declaration;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();

	// name
	declaration << (shouldUsePlaceholders ? "%s" : getGraphvizIdentifier());

	// type
	declaration << " [type=\"" << getBase()->getNodeClass()->getClassName() << "\"";

	// version
	declaration << " version=\"" << getBase()->getNodeClass()->getVersion() << "\"";

	// label
	declaration << " label=\"" << VuoStringUtilities::transcodeToGraphvizIdentifier(getBase()->getTitle());
	if (shouldUsePlaceholders)
	{
		declaration << "|<instanceData>instanceData\\l";
	}
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
		if (data && (shouldUsePlaceholders || (! eventPort->hasConnectedDataCable() && ! data->getInitialValue().empty())))
		{
			string portConstant = (shouldUsePlaceholders ? "%s" : data->getInitialValue());
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

	// instance data
	if (instanceData && shouldUsePlaceholders)
	{
		declaration << " _instanceData=\"%s\"";
	}

	declaration << "];";

	return declaration.str();
}

/**
 * Generates code that creates a Graphviz-formatted declaration for this node, with the node's fully-qualified identifier
 * (including composition prefix) and the port and instance data values.
 *
 * If this node is a subcomposition, `compositionSerialize()` is called to create the declaration of each contained node.
 */
Value * VuoCompilerNode::generateSerializedString(Module *module, BasicBlock *block, Value *compositionIdentifierValue)
{
	string formatString = getSerializedFormatString();
	vector<Value *> replacementValues;
	Function *transcodeToGraphvizIdentifierFunction = VuoCompilerCodeGenUtilities::getTranscodeToGraphvizIdentifierFunction(module);

	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);

	replacementValues.push_back(nodeIdentifierValue);

	Function *getPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionGetPortValueFunction(module);
	Value *stringSerializationValue = ConstantInt::get(getPortValueFunction->getFunctionType()->getParamType(2), 1);
	Value *falseValue = ConstantInt::get(getPortValueFunction->getFunctionType()->getParamType(3), 0);

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *port = static_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		VuoCompilerInputData *data = port->getData();
		if (data)
		{
			Constant *portIdentifierValue = constantStrings->get(module, port->getIdentifier());

			// char *serializedPort = compositionGetPortValue(compositionIdentifier, portIdentifier, 2, false);
			vector<Value *> getPortValueArgs;
			getPortValueArgs.push_back(compositionIdentifierValue);
			getPortValueArgs.push_back(portIdentifierValue);
			getPortValueArgs.push_back(stringSerializationValue);
			getPortValueArgs.push_back(falseValue);
			CallInst *serializedPortValue = CallInst::Create(getPortValueFunction, getPortValueArgs, "", block);

			// char *escapedPort = transcodeToGraphvizIdentifier(serializedPort);
			CallInst *escapedPortValue = CallInst::Create(transcodeToGraphvizIdentifierFunction, serializedPortValue, "", block);
			replacementValues.push_back(escapedPortValue);

			// free(serializedPort);
			VuoCompilerCodeGenUtilities::generateFreeCall(module, block, serializedPortValue);
		}
	}

	if (instanceData)
	{
		// char *serializedInstanceData = ...;
		Value *instanceDataValue = instanceData->generateLoad(module, block, nodeContextValue);
		Value *serializedInstanceDataValue = VuoCompilerCodeGenUtilities::generateSerialization(module, block, instanceDataValue, *constantStrings);

		// char *escapedInstanceData = transcodeToGraphvizIdentifier(serializedInstanceData);
		CallInst *escapedInstanceDataValue = CallInst::Create(transcodeToGraphvizIdentifierFunction, serializedInstanceDataValue, "", block);
		replacementValues.push_back(escapedInstanceDataValue);

		// free(serializedInstanceData);
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, serializedInstanceDataValue);
	}

	// sprintf(serializedNode, "...%s...", <escaped strings>);
	Value *serializedNodeValue = VuoCompilerCodeGenUtilities::generateFormattedString(module, block, formatString, replacementValues, *constantStrings);

	Value *lineSeparatorValue = constantStrings->get(module, "\n");
	vector<Value *> serializedNodeAndLineSeparator;
	serializedNodeAndLineSeparator.push_back(serializedNodeValue);
	serializedNodeAndLineSeparator.push_back(lineSeparatorValue);
	serializedNodeValue = VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, serializedNodeAndLineSeparator, *constantStrings);


	Function *subcompositionFunctionSrc = getBase()->getNodeClass()->getCompiler()->getCompositionSerializeFunction();
	if (subcompositionFunctionSrc)
	{
		// char *serializedContainedNodes = <subcomposition>_compositionSerialize(nodeIdentifier);
		Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);
		Value *serializedContainedNodesValue = CallInst::Create(subcompositionFunctionDst, nodeIdentifierValue, "", block);

		// char *serializedNodeAndContainedNodes = malloc(...);
		// strcat(serializedNodeAndContainedNodes, serializedNode);
		// strcat(serializedNodeAndContainedNodes, serializedContainedNodes);
		// free(serializedNode);
		// free(serializedContainedNodes);
		vector<Value *> parts;
		parts.push_back(serializedNodeValue);
		parts.push_back(serializedContainedNodesValue);
		Value *serializedNodeAndContainedNodesValue = VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, parts, *constantStrings);
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, serializedNodeValue);
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, serializedContainedNodesValue);
		serializedNodeValue = serializedNodeAndContainedNodesValue;
	}


	// foreach (char *escapedString, <escaped strings>)
	//   free(escapedString);
	for (vector<Value *>::iterator i = replacementValues.begin(); i != replacementValues.end(); ++i)
		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, *i);

	return serializedNodeValue;
}

/**
 * Generates code that sets this node's input port values and instance data from the declaration found in the given Graphviz graph
 * (`graph_t *`).
 *
 * If this node is a subcomposition, `compositionUnserialize()` is called to unserialize each contained node.
 */
void VuoCompilerNode::generateUnserialization(Module *module, Function *function, BasicBlock *&block, Value *compositionIdentifierValue,
											  Value *graphValue)
{
	Value *nodeIdentifierValue = generateIdentifierValue(module, block, compositionIdentifierValue);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);

	Function *getConstantValueFromGraphvizFunction = VuoCompilerCodeGenUtilities::getGetConstantValueFromGraphvizFunction(module);
	Function *setInputPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);

	ConstantPointerNull *nullPointerToChar = ConstantPointerNull::get( PointerType::get(IntegerType::get(module->getContext(), 8), 0) );
	Value *falseValue = ConstantInt::get( setInputPortValueFunction->getFunctionType()->getParamType(3), 0 );
	Value *trueValue = ConstantInt::get( setInputPortValueFunction->getFunctionType()->getParamType(3), 1 );

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *port = static_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		VuoType *type = port->getDataVuoType();
		if (type)
		{
			string portName = port->getBase()->getClass()->getName();
			Value *portNameValue = constantStrings->get(module, portName);
			Value *portIdentifierValue = constantStrings->get(module, port->getIdentifier());

			// char *serializedPort = getConstantValueFromGraphviz(graph, nodeGraphvizIdentifier, portGraphvizIdentifier);
			vector<Value *> getConstantValueFromGraphvizArgs;
			getConstantValueFromGraphvizArgs.push_back(graphValue);
			getConstantValueFromGraphvizArgs.push_back(nodeIdentifierValue);
			getConstantValueFromGraphvizArgs.push_back(portNameValue);
			CallInst *serializedPortValue = CallInst::Create(getConstantValueFromGraphvizFunction, getConstantValueFromGraphvizArgs, "", block);

			// if (serializedPort != NULL)
			BasicBlock *setInputPortValueBlock = BasicBlock::Create(module->getContext(), "setInputPortValue", function, NULL);
			BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "getSerializedValue", function, NULL);
			ICmpInst *serializedPortValueNotNull = new ICmpInst(*block, ICmpInst::ICMP_NE, serializedPortValue, nullPointerToChar, "");
			BranchInst::Create(setInputPortValueBlock, nextBlock, serializedPortValueNotNull, block);

			// compositionSetInputPortValue(compositionIdentifier, portIdentifier, serializedPort, false, false);
			vector<Value *> setInputPortValueArgs;
			setInputPortValueArgs.push_back(compositionIdentifierValue);
			setInputPortValueArgs.push_back(portIdentifierValue);
			setInputPortValueArgs.push_back(serializedPortValue);
			setInputPortValueArgs.push_back(falseValue);
			setInputPortValueArgs.push_back(falseValue);
			setInputPortValueArgs.push_back(falseValue);
			setInputPortValueArgs.push_back(trueValue);
			setInputPortValueArgs.push_back(trueValue);
			CallInst::Create(setInputPortValueFunction, setInputPortValueArgs, "", setInputPortValueBlock);

			BranchInst::Create(nextBlock, setInputPortValueBlock);
			block = nextBlock;
		}
	}

	if (instanceData)
	{
		Value *instanceDataNameValue = constantStrings->get(module, "instanceData");

		// char *serializedData = getConstantValueFromGraphviz(graph, nodeGraphvizIdentifier, "instanceData");
		vector<Value *> getConstantValueFromGraphvizArgs;
		getConstantValueFromGraphvizArgs.push_back(graphValue);
		getConstantValueFromGraphvizArgs.push_back(nodeIdentifierValue);
		getConstantValueFromGraphvizArgs.push_back(instanceDataNameValue);
		CallInst *serializedDataValue = CallInst::Create(getConstantValueFromGraphvizFunction, getConstantValueFromGraphvizArgs, "", block);

		// if (serializedData != NULL)
		BasicBlock *setInstanceDataValueBlock = BasicBlock::Create(module->getContext(), "setInstanceDataValue", function, NULL);
		BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "getSerializedValue", function, NULL);
		ICmpInst *serializedDataValueNotNull = new ICmpInst(*block, ICmpInst::ICMP_NE, serializedDataValue, nullPointerToChar, "");
		BranchInst::Create(setInstanceDataValueBlock, nextBlock, serializedDataValueNotNull, block);

		// { /* unserialize serializedData */ }
		Value *oldInstanceDataValue = instanceData->generateLoad(module, setInstanceDataValueBlock, nodeContextValue);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, setInstanceDataValueBlock, oldInstanceDataValue);
		Value *instanceDataVariable = instanceData->getVariable(module, setInstanceDataValueBlock, nodeContextValue);
		VuoCompilerCodeGenUtilities::generateUnserialization(module, setInstanceDataValueBlock, serializedDataValue, instanceDataVariable, *constantStrings);
		Value *newInstanceDataValue = instanceData->generateLoad(module, setInstanceDataValueBlock, nodeContextValue);
		VuoCompilerCodeGenUtilities::generateRetainCall(module, setInstanceDataValueBlock, newInstanceDataValue);

		BranchInst::Create(nextBlock, setInstanceDataValueBlock);
		block = nextBlock;
	}


	Function *subcompositionFunctionSrc = getBase()->getNodeClass()->getCompiler()->getCompositionUnserializeFunction();
	if (subcompositionFunctionSrc)
	{
		Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);

		PointerType *pointerToCharType = static_cast<PointerType *>( subcompositionFunctionDst->getFunctionType()->getParamType(1) );
		Value *nullSerializedCompositionValue = ConstantPointerNull::get(pointerToCharType);

		Value *convertedGraphValue = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(graphValue, subcompositionFunctionDst, 2, NULL, module, block);

		// <subcomposition>_compositionUnserialize(nodeIdentifier, NULL, graph);
		vector<Value *> args;
		args.push_back(nodeIdentifierValue);
		args.push_back(nullSerializedCompositionValue);
		args.push_back(convertedGraphValue);
		CallInst::Create(subcompositionFunctionDst, args, "", block);
	}

	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
}
