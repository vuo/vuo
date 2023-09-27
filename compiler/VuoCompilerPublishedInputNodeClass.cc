/**
 * @file
 * VuoCompilerPublishedInputNodeClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"


VuoCompilerPublishedInputNodeClass * VuoCompilerPublishedInputNodeClass::singleton = NULL;

/**
 * Returns an empty instance used to enable polymorphism. Functions that would otherwise be static are called on this instance.
 */
VuoCompilerPublishedInputNodeClass * VuoCompilerPublishedInputNodeClass::getSingleton(void)
{
	if (! singleton)
		singleton = new VuoCompilerPublishedInputNodeClass(new VuoNodeClass(VuoNodeClass::publishedInputNodeClassName, vector<string>(), vector<string>()));

	return singleton;
}

/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(string nodeClassName, Module *module) :
	VuoCompilerPublishedNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *compilerNodeClass) :
	VuoCompilerPublishedNodeClass(compilerNodeClass)
{
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerPublishedNodeClass(baseNodeClass)
{
}

/**
 * Returns a new node class with port names and types as specified by @a nodeClassName, or null if @a nodeClassName is not a
 * valid published input node class name.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	return VuoCompilerPublishedNodeClass::newNodeClass(nodeClassName, compiler, llvmQueue, getSingleton());
}

/**
 * Returns a new node class with port names and types corresponding to @a publishedInputPorts.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClass(vector<VuoPublishedPort *> publishedInputPorts, dispatch_queue_t llvmQueue)
{
	return VuoCompilerPublishedNodeClass::newNodeClass(publishedInputPorts, llvmQueue, getSingleton());
}

/**
 * Creates a compiler and base node class from the node class implementation in the module,
 * or returns null if the implementation is not of a published input node class.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClass(string nodeClassName, Module *module)
{
	if (VuoStringUtilities::beginsWith(nodeClassName, VuoNodeClass::publishedInputNodeIdentifier + "."))
	{
		VuoCompilerPublishedInputNodeClass *cnc = new VuoCompilerPublishedInputNodeClass(nodeClassName, module);
		VuoCompilerPublishedInputNodeClass *cnc2 = new VuoCompilerPublishedInputNodeClass(cnc);
		delete cnc;
		return cnc2->getBase();
	}

	return nullptr;
}

/**
 * Returns a node class that has, for each of @a publishedInputPorts, a corresponding door input port and
 * a corresponding output port, both of the same data type as the published port.
 *
 * When an event hits one of the input ports, it transmits through the corresponding output port.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClassWithImplementation(const string &nodeClassName,
																				  const vector<string> &portNames, const vector<VuoType *> &types)
{
	Module *module = new Module(nodeClassName, *VuoCompiler::globalLLVMContext);

	// VuoModuleMetadata({…});
	json_object *metadata = json_object_new_object();
	json_object_object_add(metadata, "title", json_object_new_string(VuoNodeClass::publishedInputNodeIdentifier.c_str()));
	json_object_object_add(metadata, "version", json_object_new_string("1.0.0"));
	json_object *specializedModuleDetails = getSingleton()->buildSpecializedModuleDetails(types);
	json_object_object_add(metadata, "specializedModule", specializedModuleDetails);
	string metadataStr = json_object_to_json_string(metadata);
	json_object_put(metadata);
	VuoCompilerCodeGenUtilities::generateModuleMetadata(module, metadataStr, "");

	// For published ports a (data+event) and b (event):
	//
	// void nodeEvent
	// (
	//   VuoInputData(<data type>,<default value>) a,
	//   VuoInputEvent({"eventBlocking":"door","data":"a"}) aEvent
	//   VuoInputEvent({"eventBlocking":"door"}) b,
	//   VuoOutputData(<data type>) aOut,
	//   VuoOutputEvent({"data":"aOut"}) aOutEvent,
	//   VuoOutputEvent() bOut
	// )
	// {
	//     if (aEvent)
	//     {
	//         *aOut = a;
	//         *aOutEvent = true;
	//     }
	//     if (b)
	//     {
	//         *bOut = true;
	//     }
	// }


	vector<VuoPort *> modelInputPorts;
	for (size_t i = 0; i < portNames.size(); ++i)
	{
		VuoCompilerPublishedPort *modelPort = VuoCompilerPublishedPort::newPort(portNames.at(i), types.at(i));
		modelInputPorts.push_back(modelPort->getBase());
	}

	vector<VuoPort *> modelOutputPorts;
	for (size_t i = 0; i < portNames.size(); ++i)
	{
		VuoCompilerPublishedPort *modelPort = VuoCompilerPublishedPort::newPort(portNames.at(i) + "Out", types.at(i));
		modelOutputPorts.push_back(modelPort->getBase());
	}

	map<VuoPort *, VuoPortClass::EventBlocking> eventBlockingForInputPorts;
	for (VuoPort *modelInputPort : modelInputPorts)
		eventBlockingForInputPorts[modelInputPort] = VuoPortClass::EventBlocking_Door;

	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	VuoCompilerConstantsCache constantsCache(module);

	Function *eventFunction = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, "", false, false,
																				nullptr, modelInputPorts, modelOutputPorts,
																				map<VuoPort *, json_object *>(), map<VuoPort *, string>(),
																				map<VuoPort *, string>(), eventBlockingForInputPorts,
																				indexOfParameter, indexOfEventParameter, &constantsCache);

	BasicBlock *initialBlock = &(eventFunction->getEntryBlock());
	BasicBlock *currBlock = initialBlock;

	for (size_t i = 0; i < portNames.size(); ++i)
	{
		BasicBlock *eventBlock = BasicBlock::Create(module->getContext(), "event", eventFunction);
		BasicBlock *noEventBlock = BasicBlock::Create(module->getContext(), "noEvent", eventFunction);

		VuoType *dataType = types.at(i);

		// if (aEvent)

		size_t inputEventArgIndex = (dataType ? indexOfEventParameter[ modelInputPorts[i] ] : indexOfParameter[ modelInputPorts[i] ]);
		Value *inputEventArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, inputEventArgIndex);

		Constant *falseValue = ConstantInt::get(inputEventArg->getType(), 0);
		ICmpInst *inputEventIsTrue = new ICmpInst(*currBlock, ICmpInst::ICMP_NE, inputEventArg, falseValue);
		BranchInst::Create(eventBlock, noEventBlock, inputEventIsTrue, currBlock);

		//     *aOutEvent = true;

		size_t outputEventArgIndex = (dataType ? indexOfEventParameter[ modelOutputPorts[i] ] : indexOfParameter[ modelOutputPorts[i] ]);
		Value *outputEventArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, outputEventArgIndex);

		Constant *trueValue = ConstantInt::get(inputEventArg->getType(), 1);
		new StoreInst(trueValue, outputEventArg, eventBlock);

		//     *aOut = a;

		if (dataType)
		{
			size_t inputDataArgIndex = indexOfParameter[ modelInputPorts[i] ];
			Value *inputDataPointer = dataType->getCompiler()->convertArgsToPortData(module, eventBlock, eventFunction, inputDataArgIndex);

			size_t outputDataArgIndex = indexOfParameter[ modelOutputPorts[i] ];
			Value *outputDataArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, outputDataArgIndex);

			VuoCompilerCodeGenUtilities::generateMemoryCopy(module, eventBlock, inputDataPointer, outputDataArg, dataType->getCompiler());
		}

		BranchInst::Create(noEventBlock, eventBlock);
		currBlock = noEventBlock;
	}

	ReturnInst::Create(module->getContext(), currBlock);


	VuoCompilerPublishedInputNodeClass *dummyNodeClass = new VuoCompilerPublishedInputNodeClass(nodeClassName, module);
	VuoCompilerNodeClass *nodeClass = new VuoCompilerPublishedInputNodeClass(dummyNodeClass);
	delete dummyNodeClass;

	return nodeClass->getBase();
}

/**
 * Returns a node class with the same ports as the return value of VuoCompilerPublishedInputNodeClass::newNodeClass(),
 * but without an implementation (LLVM module).
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClassWithoutImplementation(const string &nodeClassName,
																					 const vector<string> &portNames, const vector<VuoType *> &types)
{
	vector<VuoPortClass *> inputPortClasses;
	vector<VuoPortClass *> outputPortClasses;

	VuoPortClass *refreshPortClass = (new VuoCompilerInputEventPortClass("refresh"))->getBase();
	inputPortClasses.push_back(refreshPortClass);

	for (size_t i = 0; i < portNames.size(); ++i)
	{
		VuoCompilerInputEventPortClass *inputPortClass = new VuoCompilerInputEventPortClass(portNames.at(i));
		if (types.at(i))
		{
			VuoCompilerInputDataClass *inputDataClass = new VuoCompilerInputDataClass("");
			inputPortClass->setDataClass(inputDataClass);
			inputPortClass->setDataVuoType(types.at(i));
		}

		inputPortClass->getBase()->setEventBlocking(VuoPortClass::EventBlocking_Door);

		inputPortClasses.push_back(inputPortClass->getBase());
	}

	set<string> takenPortNames;
	for (VuoPortClass *inputPortClass : inputPortClasses)
		takenPortNames.insert(inputPortClass->getName());

	for (size_t i = 0; i < portNames.size(); ++i)
	{
		string preferredName = portNames.at(i) + "Out";
		string outputName = VuoStringUtilities::formUniqueIdentifier(takenPortNames, preferredName);

		VuoCompilerInputEventPortClass *outputPortClass = new VuoCompilerInputEventPortClass(outputName);
		if (types.at(i))
		{
			VuoCompilerInputDataClass *outputDataClass = new VuoCompilerInputDataClass("");
			outputPortClass->setDataClass(outputDataClass);
			outputPortClass->setDataVuoType(types.at(i));
		}

		outputPortClasses.push_back(outputPortClass->getBase());
	}

	VuoNodeClass *baseNodeClass = new VuoNodeClass(nodeClassName, refreshPortClass, inputPortClasses, outputPortClasses);
	VuoCompilerNodeClass *nodeClass = new VuoCompilerPublishedInputNodeClass(baseNodeClass);

	nodeClass->getBase()->setDefaultTitle(VuoNodeClass::publishedInputNodeIdentifier);

	return nodeClass->getBase();
}

/**
 * Returns the index of the input port on this node class that corresponds to the published port.
 */
size_t VuoCompilerPublishedInputNodeClass::getInputPortIndexForPublishedInputPort(size_t publishedInputPortIndex)
{
	return VuoNodeClass::unreservedInputPortStartIndex + publishedInputPortIndex;
}

/**
 * Returns the index of the output port on this node class that corresponds to the published port.
 */
size_t VuoCompilerPublishedInputNodeClass::getOutputPortIndexForPublishedInputPort(size_t publishedInputPortIndex)
{
	return VuoNodeClass::unreservedOutputPortStartIndex + publishedInputPortIndex;
}

/**
 * Creates a class name for a published input node with the given published input ports.
 */
string VuoCompilerPublishedInputNodeClass::buildNodeClassName(const vector<VuoPublishedPort *> &publishedInputPorts)
{
	return getSingleton()->buildNodeClassNameFromPorts(publishedInputPorts);
}

/**
 * Returns the prefix common to all published input node class names.
 */
string VuoCompilerPublishedInputNodeClass::getNodeClassNamePrefix(void)
{
	return VuoNodeClass::publishedInputNodeClassName;
}

/**
 * Returns the names of input port classes that are added automatically to all published input node classes.
 */
set<string> VuoCompilerPublishedInputNodeClass::getReservedPortNames(void)
{
	set<string> names;
	names.insert("refresh");
	return names;
}
