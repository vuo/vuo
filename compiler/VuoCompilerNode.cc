/**
 * @file
 * VuoCompilerNode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"

#include "VuoPort.hh"
#include "VuoStringUtilities.hh"

const string VuoCompilerNode::RuntimeSuffix = "__runtime";


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
}

/**
 * Generates the allocation of the node's runtime representation, which consists of the runtime representations for the ports and the instance data.
 *
 * @param module The destination LLVM module (i.e., generated code).
 */
void VuoCompilerNode::generateAllocation(Module *module)
{
	string nodeInstanceRuntimeIdentifier = getIdentifier() + RuntimeSuffix;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		(*i)->getCompiler()->generateAllocation(module, nodeInstanceRuntimeIdentifier);

	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		(*i)->getCompiler()->generateAllocation(module, nodeInstanceRuntimeIdentifier);

	if (instanceData)
		instanceData->generateAllocation(module, nodeInstanceRuntimeIdentifier);
}

/**
 * Generates a call to @c nodeEvent or @c nodeInstanceEvent.
 *
 * The generated code sets up the arguments for the call, does the call, transmits events from input ports to output ports,
 * and handles memory management for the node's runtime representation.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param function The function in which to generate code.
 * @param initialBlock The block to which the function call will be appended.
 * @param finalBlock The block following the function call.
 */
void VuoCompilerNode::generateEventFunctionCall(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getEventFunction();

	generateFunctionCall(functionSrc, module, initialBlock);

	// The output port should transmit an event if it's a done port or any non-blocking input port received the event.
	// The output port should not transmit an event if no non-blocking or door input ports received the event.
	// Otherwise, the output port's event transmission is handled by the node class implementation.

	VuoCompilerOutputEventPort *donePort = dynamic_cast<VuoCompilerOutputEventPort *>( getBase()->getDonePort()->getCompiler() );
	donePort->generateStore(true, initialBlock);

	vector<VuoPort *> nonBlockingInputPorts;
	vector<VuoPort *> doorInputPorts;
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoPortClass::EventBlocking eventBlocking = (*i)->getClass()->getEventBlocking();
		if (eventBlocking == VuoPortClass::EventBlocking_None)
			nonBlockingInputPorts.push_back(*i);
		else if (eventBlocking == VuoPortClass::EventBlocking_Door)
			doorInputPorts.push_back(*i);
	}

	Value *eventHitNonBlockingInputPort = generateReceivedEventCondition(initialBlock, nonBlockingInputPorts);
	Value *eventHitDoorInputPort = generateReceivedEventCondition(initialBlock, doorInputPorts);

	Value *transmitForAllOutputPorts = eventHitNonBlockingInputPort;
	Value *blockForAllOutputPorts = BinaryOperator::Create(Instruction::And,
														   BinaryOperator::CreateNot(eventHitNonBlockingInputPort, "", initialBlock),
														   BinaryOperator::CreateNot(eventHitDoorInputPort, "", initialBlock),
														   "", initialBlock);
	Value *handleTransmission = BinaryOperator::Create(Instruction::Or, transmitForAllOutputPorts, blockForAllOutputPorts, "", initialBlock);

	Constant *zeroValue = ConstantInt::get(transmitForAllOutputPorts->getType(), 0);
	ICmpInst *handleTransmissionComparison = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, handleTransmission, zeroValue, "");
	BasicBlock *handleTransmissionBlock = BasicBlock::Create(module->getContext(), "transmitThroughNode", function, 0);
	BranchInst::Create(handleTransmissionBlock, finalBlock, handleTransmissionComparison, initialBlock);

	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort && eventPort != donePort)
			eventPort->generateStore(transmitForAllOutputPorts, handleTransmissionBlock);
	}

	BranchInst::Create(finalBlock, handleTransmissionBlock);
}

/**
 * Generates a call to @c nodeInstanceInit, stores the return value into the node's instance data,
 * and retains the node's instance data.
 *
 * Assumes the node is stateful. Assumes any trigger ports' function declarations have been generated.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 */
void VuoCompilerNode::generateInitFunctionCall(Module *module, BasicBlock *block)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getInitFunction();

	CallInst *call = generateFunctionCall(functionSrc, module, block);
	instanceData->generateStore(call, block);

	// Retain the instance data.
	LoadInst *instanceDataValue = instanceData->generateLoad(block);
	VuoCompilerCodeGenUtilities::generateRetainCall(module, block, instanceDataValue);
}

/**
 * Generates a call to @c nodeInstanceFini and releases the node's instance data.
 *
 * Assumes the node is stateful.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 */
void VuoCompilerNode::generateFiniFunctionCall(Module *module, BasicBlock *block)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getFiniFunction();

	generateFunctionCall(functionSrc, module, block);

	// Release the instance data.
	LoadInst *instanceDataValue = instanceData->generateLoad(block);
	VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, instanceDataValue);
}

/**
 * Generates a call to @c nodeInstanceTriggerStart, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 */
void VuoCompilerNode::generateCallbackStartFunctionCall(Module *module, BasicBlock *block)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStartFunction();
	if (! functionSrc)
		return;

	generateFunctionCall(functionSrc, module, block);
}

/**
 * Generates a call to @c nodeInstanceTriggerUpdate, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 */
void VuoCompilerNode::generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackUpdateFunction();
	if (! functionSrc)
		return;

	generateFunctionCall(functionSrc, module, block);
}

/**
 * Generates a call to @c nodeInstanceTriggerStop, if the function exists.
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 */
void VuoCompilerNode::generateCallbackStopFunctionCall(Module *module, BasicBlock *block)
{
	Function *functionSrc = getBase()->getNodeClass()->getCompiler()->getCallbackStopFunction();
	if (! functionSrc)
		return;

	generateFunctionCall(functionSrc, module, block);
}

/**
 * Sets up the arguments to pass to the node class's event, init, callback start, or callback update function.
 *
 * @param functionSrc The node class's function in the source module.
 * @param module The module in which to generate code (destination module).
 * @param block The block in which to generate code.
 */
CallInst * VuoCompilerNode::generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block)
{
	Function *functionDst = VuoCompilerModule::declareFunctionInModule(module, functionSrc);

	vector<Value *> args(functionDst->getFunctionType()->getNumParams());

	// Set up the arguments for input ports.
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			if (isArgumentInFunction(eventPort, functionSrc))
			{
				size_t index = getArgumentIndexInFunction(eventPort, functionSrc);
				args[index] = eventPort->generateLoad(block);
			}

			VuoCompilerInputData *data = eventPort->getData();
			if (data)
			{
				if (isArgumentInFunction(data, functionSrc))
				{
					size_t index = getArgumentIndexInFunction(data, functionSrc);
					Value *arg = data->generateLoad(block);

					bool isLoweredToTwoParameters = static_cast<VuoCompilerInputDataClass *>(data->getBase()->getClass()->getCompiler())->isLoweredToTwoParameters();
					Value *secondArg = NULL;
					Value **secondArgIfNeeded = (isLoweredToTwoParameters ? &secondArg : NULL);
					arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, functionDst, index, secondArgIfNeeded, module, block);

					args[index] = arg;
					if (secondArg)
						args[index + 1] = secondArg;
				}
			}
		}
	}

	// Set up the arguments for output ports.
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			if (isArgumentInFunction(eventPort, functionSrc))
			{
				size_t index = getArgumentIndexInFunction(eventPort, functionSrc);
				args[index] = eventPort->getVariable();
			}

			VuoCompilerOutputData *data = eventPort->getData();
			if (data)
			{
				if (isArgumentInFunction(data, functionSrc))
				{
					size_t index = getArgumentIndexInFunction(data, functionSrc);
					Value *arg = data->getVariable();
					arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, functionDst, index, NULL, module, block);
					args[index] = arg;
				}
			}
		}
		else
		{
			VuoCompilerTriggerPort *triggerPort = (VuoCompilerTriggerPort *)((*i)->getCompiler());
			if (isArgumentInFunction(triggerPort, functionSrc))
			{
				size_t index = getArgumentIndexInFunction(triggerPort, functionSrc);
				args[index] = triggerPort->getFunction();
			}
		}
	}

	// Set up the argument for the instance data (if it exists).
	if (instanceData)
	{
		if (isArgumentInFunction(instanceData, functionSrc))
		{
			size_t index = getArgumentIndexInFunction(instanceData, functionSrc);
			args[index] = instanceData->getVariable();
		}
	}

	// Save the old output port values.
	map<VuoCompilerOutputData *, LoadInst *> oldOutputDataValues;
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			VuoCompilerOutputData *data = eventPort->getData();
			if (data && isArgumentInFunction(data, functionSrc))
				oldOutputDataValues[data] = data->generateLoad(block);
		}
	}

	LoadInst *oldInstanceDataValue = NULL;
	if (instanceData && isArgumentInFunction(instanceData, functionSrc))
	{
		// Save the old instance data value.
		oldInstanceDataValue = instanceData->generateLoad(block);
	}

	// Call the node class's function.
	CallInst *call = CallInst::Create(functionDst, args, "", block);

	// Retain the new output port values.
	for (map<VuoCompilerOutputData *, LoadInst *>::iterator i = oldOutputDataValues.begin(); i != oldOutputDataValues.end(); ++i)
	{
		VuoCompilerOutputData *data = i->first;
		LoadInst *outputDataValue = data->generateLoad(block);
		VuoCompilerCodeGenUtilities::generateRetainCall(module, block, outputDataValue);
	}

	// Release the old output port values.
	for (map<VuoCompilerOutputData *, LoadInst *>::iterator i = oldOutputDataValues.begin(); i != oldOutputDataValues.end(); ++i)
	{
		LoadInst *oldOutputPortValue = i->second;
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, oldOutputPortValue);
	}

	if (oldInstanceDataValue)
	{
		// Retain the new instance data value.
		LoadInst *instanceDataValue = instanceData->generateLoad(block);
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
Value * VuoCompilerNode::generateReceivedEventCondition(BasicBlock *block)
{
	return generateReceivedEventCondition(block, getBase()->getInputPorts());
}

/**
 * Generates a condition that is true if any of the given input event ports of this node have received an event.
 */
Value * VuoCompilerNode::generateReceivedEventCondition(BasicBlock *block, vector<VuoPort *> selectedInputPorts)
{
	Value *conditionValue = ConstantInt::getFalse(block->getContext());
	for (vector<VuoPort *>::iterator i = selectedInputPorts.begin(); i != selectedInputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			Value *pushValue = eventPort->generateLoad(block);
			if (((IntegerType *)pushValue->getType())->getBitWidth() > 1)
				pushValue = new TruncInst(pushValue, IntegerType::get(block->getContext(), 1), "", block);
			conditionValue = BinaryOperator::Create(Instruction::Or, conditionValue, pushValue, "", block);
		}
	}
	return conditionValue;
}

/**
 * Generates code to set all of this node's input and output event ports to an unpushed state.
 */
void VuoCompilerNode::generatePushedReset(BasicBlock *block)
{
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		if (eventPort)
			eventPort->generateStore(false, block);
	}
	vector<VuoPort *> outputPorts = getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		VuoCompilerOutputEventPort *eventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (eventPort)
			eventPort->generateStore(false, block);
	}
}

/**
 * Releases all reference-counted port values.
 */
void VuoCompilerNode::generateFinalization(Module *module, BasicBlock *block, bool isInput)
{
	vector<VuoPort *> ports = isInput ? getBase()->getInputPorts() : getBase()->getOutputPorts();

	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>((*i)->getCompiler());
		if (eventPort)
		{
			VuoCompilerData *data = eventPort->getData();
			if (data)
			{
				LoadInst *dataValue = data->generateLoad(block);
				VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, dataValue);
			}
		}
		/// @todo release trigger port data
	}
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
 * @eg{vuo_math_add_integer__Add2}
 */
string VuoCompilerNode::getIdentifier(void)
{
	return getBase()->getNodeClass()->getCompiler()->getClassIdentifier() + "__" + getGraphvizIdentifier();
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
	declaration << getGraphvizIdentifier();

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
		if (data && (shouldUsePlaceholders || (! eventPort->hasConnectedDataCable(true) && ! data->getInitialValue().empty())))
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
 * Generates code that creates a string containing the declaration for this node
 * as it would appear in a .vuo (Graphviz dot format) file.
 */
Value * VuoCompilerNode::generateSerializedString(Module *module, BasicBlock *block)
{
	string formatString = getSerializedFormatString();
	vector<Value *> replacementValues;
	Function *transcodeToGraphvizIdentifierFunction = VuoCompilerCodeGenUtilities::getTranscodeToGraphvizIdentifierFunction(module);
	Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);

	vector<VuoPort *> ports = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerInputEventPort *port = static_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		VuoCompilerInputData *data = port->getData();
		if (data)
		{
			// char *serializedPort = <type>_stringFromValue(...);
			Value *portValue = data->generateLoad(block);
			VuoCompilerDataClass *dataClass = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler());
			VuoCompilerType *type = dataClass->getVuoType()->getCompiler();
			Value *serializedPortValue = type->generateStringFromValueFunctionCall(module, block, portValue);

			// char *escapedPort = transcodeToGraphvizIdentifier(serializedPort);
			CallInst *escapedPortValue = CallInst::Create(transcodeToGraphvizIdentifierFunction, serializedPortValue, "", block);
			replacementValues.push_back(escapedPortValue);

			// free(serializedPort);
			CallInst::Create(freeFunction, serializedPortValue, "", block);
		}
	}

	if (instanceData)
	{
		// char *serializedInstanceData = ...;
		Value *instanceDataValue = instanceData->generateLoad(block);
		Value *serializedInstanceDataValue = VuoCompilerCodeGenUtilities::generateSerialization(module, block, instanceDataValue);

		// char *escapedInstanceData = transcodeToGraphvizIdentifier(serializedInstanceData);
		CallInst *escapedInstanceDataValue = CallInst::Create(transcodeToGraphvizIdentifierFunction, serializedInstanceDataValue, "", block);
		replacementValues.push_back(escapedInstanceDataValue);

		// free(serializedInstanceData);
		CallInst::Create(freeFunction, serializedInstanceDataValue, "", block);
	}

	// sprintf(serializedNode, "...%s...", <escaped strings>);
	Value *formattedString = VuoCompilerCodeGenUtilities::generateFormattedString(module, block, formatString, replacementValues);

	// foreach (char *escapedString, <escaped strings>)
	//   free(escapedString);
	for (vector<Value *>::iterator i = replacementValues.begin(); i != replacementValues.end(); ++i)
		CallInst::Create(freeFunction, *i, "", block);

	return formattedString;
}

/**
 * Generates code that sets the input port values and instance data of this node from the given Graphviz graph (graph_t) value.
 */
void VuoCompilerNode::generateUnserialization(Module *module, Function *function, BasicBlock *&block, Value *graphValue)
{
	Value *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, graphvizIdentifier);

	Function *getConstantValueFromGraphvizFunction = VuoCompilerCodeGenUtilities::getGetConstantValueFromGraphvizFunction(module);
	Function *setInputPortValueFunction = VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(module);

	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	ConstantPointerNull *nullPointerToChar = ConstantPointerNull::get(pointerToCharType);

	vector<VuoPort *> ports = getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoCompilerInputEventPort *port = static_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());
		VuoCompilerInputData *data = port->getData();
		if (data)
		{
			string portName = port->getBase()->getClass()->getName();
			Value *portNameValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, portName);
			string portIdentifier = port->getIdentifier();
			Value *portIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, portIdentifier);

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

			// { setInputPortValue(portLLVMIdentifier, serializedPort, false); }
			ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
			vector<Value *> setInputPortValueArgs;
			setInputPortValueArgs.push_back(portIdentifierValue);
			setInputPortValueArgs.push_back(serializedPortValue);
			setInputPortValueArgs.push_back(zeroValue);
			CallInst::Create(setInputPortValueFunction, setInputPortValueArgs, "", setInputPortValueBlock);

			BranchInst::Create(nextBlock, setInputPortValueBlock);
			block = nextBlock;
		}
	}

	if (instanceData)
	{
		Value *instanceDataNameValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, "instanceData");

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
		LoadInst *oldInstanceDataValue = instanceData->generateLoad(setInstanceDataValueBlock);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, setInstanceDataValueBlock, oldInstanceDataValue);
		VuoCompilerCodeGenUtilities::generateUnserialization(module, setInstanceDataValueBlock, serializedDataValue, instanceData->getVariable());
		LoadInst *newInstanceDataValue = instanceData->generateLoad(setInstanceDataValueBlock);
		VuoCompilerCodeGenUtilities::generateRetainCall(module, setInstanceDataValueBlock, newInstanceDataValue);

		BranchInst::Create(nextBlock, setInstanceDataValueBlock);
		block = nextBlock;
	}
}
