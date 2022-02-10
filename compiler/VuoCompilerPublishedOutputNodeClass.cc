/**
 * @file
 * VuoCompilerPublishedOutputNodeClass implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoGenericType.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"


VuoCompilerPublishedOutputNodeClass * VuoCompilerPublishedOutputNodeClass::singleton = nullptr;

/**
 * Returns an empty instance used to enable polymorphism. Functions that would otherwise be static are called on this instance.
 */
VuoCompilerPublishedOutputNodeClass * VuoCompilerPublishedOutputNodeClass::getSingleton(void)
{
	if (! singleton)
		singleton = new VuoCompilerPublishedOutputNodeClass(new VuoNodeClass(VuoNodeClass::publishedOutputNodeClassName, vector<string>(), vector<string>()));

	return singleton;
}

/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerPublishedOutputNodeClass::VuoCompilerPublishedOutputNodeClass(string nodeClassName, Module *module) :
	VuoCompilerPublishedNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerPublishedOutputNodeClass::VuoCompilerPublishedOutputNodeClass(VuoCompilerPublishedOutputNodeClass *compilerNodeClass) :
	VuoCompilerPublishedNodeClass(compilerNodeClass)
{
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerPublishedOutputNodeClass::VuoCompilerPublishedOutputNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerPublishedNodeClass(baseNodeClass)
{
}

/**
 * Returns a new node class with port names and types as specified by @a nodeClassName, or null if @a nodeClassName is not a
 * valid published output node class name.
 */
VuoNodeClass * VuoCompilerPublishedOutputNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	return VuoCompilerPublishedNodeClass::newNodeClass(nodeClassName, compiler, llvmQueue, getSingleton());
}

/**
 * Returns a new node class with port names and types corresponding to @a publishedOutputPorts.
 */
VuoNodeClass * VuoCompilerPublishedOutputNodeClass::newNodeClass(vector<VuoPublishedPort *> publishedOutputPorts, dispatch_queue_t llvmQueue)
{
	return VuoCompilerPublishedNodeClass::newNodeClass(publishedOutputPorts, llvmQueue, getSingleton());
}

/**
 * Returns a node class that has, for each of @a publishedOutputPorts, a corresponding input port of the same
 * data type. In addition, the node class has an event input port to be used for gathering events.
 */
VuoNodeClass * VuoCompilerPublishedOutputNodeClass::newNodeClassWithImplementation(const string &nodeClassName,
																				   const vector<string> &portNames, const vector<VuoType *> &types)
{
	Module *module = new Module(nodeClassName, *VuoCompiler::globalLLVMContext);

	// VuoModuleMetadata({});
	VuoCompilerCodeGenUtilities::generateModuleMetadata(module, "{}", "");

	// For published ports a (data+event) and b (event):
	//
	// void nodeEvent
	// (
	//   VuoInputData(<data type>) a,
	//   VuoInputEvent() b,
	//   VuoInputEvent() gather
	// )
	// {
	// }


	vector<VuoPort *> modelInputPorts;
	for (size_t i = 0; i < portNames.size(); ++i)
	{
		VuoCompilerPublishedPort *modelPort = VuoCompilerPublishedPort::newPort(portNames.at(i), types.at(i));
		modelInputPorts.push_back(modelPort->getBase());
	}

	VuoCompilerInputEventPortClass *gatherPortClass = new VuoCompilerInputEventPortClass("gather");
	VuoCompilerPort *gatherPort = gatherPortClass->newPort();
	modelInputPorts.push_back(gatherPort->getBase());

	map<VuoPort *, size_t> indexOfDataParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	VuoCompilerConstantsCache constantsCache(module);

	Function *eventFunction = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, "", false, false,
																				nullptr, modelInputPorts, vector<VuoPort *>(),
																				map<VuoPort *, json_object *>(), map<VuoPort *, string>(),
																				map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
																				indexOfDataParameter, indexOfEventParameter, &constantsCache);

	BasicBlock *block = &(eventFunction->getEntryBlock());
	ReturnInst::Create(module->getContext(), block);


	VuoCompilerPublishedOutputNodeClass *dummyNodeClass = new VuoCompilerPublishedOutputNodeClass(nodeClassName, module);
	VuoCompilerNodeClass *nodeClass = new VuoCompilerPublishedOutputNodeClass(dummyNodeClass);
	delete dummyNodeClass;

	nodeClass->getBase()->setDefaultTitle(VuoNodeClass::publishedOutputNodeIdentifier);
	nodeClass->getBase()->setVersion("1.0.0");

	return nodeClass->getBase();
}

/**
 * Returns a node class with the same ports as the return value of VuoCompilerPublishedOutputNodeClass::newNodeClass(),
 * but without an implementation (LLVM module).
 */
VuoNodeClass * VuoCompilerPublishedOutputNodeClass::newNodeClassWithoutImplementation(const string &nodeClassName,
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

		inputPortClasses.push_back(inputPortClass->getBase());
	}

	VuoPortClass *gatherPortClass = (new VuoCompilerInputEventPortClass("gather"))->getBase();
	inputPortClasses.push_back(gatherPortClass);

	VuoNodeClass *baseNodeClass = new VuoNodeClass(nodeClassName, refreshPortClass, inputPortClasses, outputPortClasses);
	VuoCompilerNodeClass *nodeClass = new VuoCompilerPublishedOutputNodeClass(baseNodeClass);

	nodeClass->getBase()->setDefaultTitle(VuoNodeClass::publishedOutputNodeIdentifier);

	return nodeClass->getBase();
}

/**
 * Returns the index of the input port on this node class that corresponds to the published output port.
 */
size_t VuoCompilerPublishedOutputNodeClass::getInputPortIndexForPublishedOutputPort(size_t publishedOutputPortIndex)
{
	return VuoNodeClass::unreservedInputPortStartIndex + publishedOutputPortIndex;
}

/**
 * Returns the index of the input port on this node class that is intended for gathering events.
 */
size_t VuoCompilerPublishedOutputNodeClass::getGatherInputPortIndex(void)
{
	return getBase()->getInputPortClasses().size() - 1;
}

/**
 * Creates a class name for a published output node with the given published output ports.
 */
string VuoCompilerPublishedOutputNodeClass::buildNodeClassName(const vector<VuoPublishedPort *> &publishedOutputPorts)
{
	return getSingleton()->buildNodeClassNameFromPorts(publishedOutputPorts);
}

/**
 * Returns the prefix common to all published output node class names.
 */
string VuoCompilerPublishedOutputNodeClass::getNodeClassNamePrefix(void)
{
	return VuoNodeClass::publishedOutputNodeClassName;
}

/**
 * Returns the names of input port classes that are added automatically to all published output node classes.
 */
set<string> VuoCompilerPublishedOutputNodeClass::getReservedPortNames(void)
{
	set<string> names;
	names.insert("refresh");
	names.insert("gather");
	return names;
}
