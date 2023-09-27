/**
 * @file
 * VuoCompilerMakeListNodeClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>

#include "VuoGenericType.hh"
#include "VuoStringUtilities.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPort.hh"
#include "VuoMakeDependencies.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"


/// The common beginning of all "Make List" node class names (before the item count and item type).
const string VuoCompilerMakeListNodeClass::makeListNodeClassNamePrefix = "vuo.list.make";

/// The common description of all "Make List" node classes.
const string VuoCompilerMakeListNodeClass::makeListNodeClassDescription = "Creates a list from the given items.";


/**
 * Creates a Make List node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(string nodeClassName, Module *module) :
	VuoCompilerSpecializedNodeClass(nodeClassName, module)
{
	itemCount = 0;
	string itemTypeName;
	parseNodeClassName(nodeClassName, itemCount, itemTypeName);

	listType = nullptr;
	listTypeName = VuoGenericType::isGenericTypeName(itemTypeName) ? "VuoList" : VuoType::listTypeNamePrefix + itemTypeName;
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass) :
	VuoCompilerSpecializedNodeClass(compilerNodeClass)
{
	this->itemCount = compilerNodeClass->itemCount;
	this->listType = compilerNodeClass->listType;
	this->listTypeName = compilerNodeClass->listTypeName;
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerSpecializedNodeClass(baseNodeClass)
{
	itemCount = 0;
	listType = nullptr;
}

/**
 * Generates a node class for a "Make List" node.
 * The node class's event function inputs zero or more items and outputs a list of those items.
 *
 * @param nodeClassName The name of the node class to generate. It should have the format "vuo.list.make.<item count>.<item type>" (e.g. "vuo.list.make.3.VuoInteger").
 * @param compiler The compiler to use for looking up types.
 * @param llvmQueue Synchronizes access to LLVM's global context.
 * @return The generated node class, or null if the node class name does not have the correct format.
 */
VuoNodeClass * VuoCompilerMakeListNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	unsigned long itemCount;
	string itemTypeStr;
	bool parsedOk = parseNodeClassName(nodeClassName, itemCount, itemTypeStr);
	if (! parsedOk)
		return NULL;

	VuoCompilerType *itemType = compiler->getType(itemTypeStr);
	VuoCompilerType *listType = compiler->getType(VuoType::listTypeNamePrefix + itemTypeStr);
	if (! itemType || ! listType)
		return NULL;

	map<string, string> specializedForGenericTypeName = {{VuoGenericType::createGenericTypeName(1), itemTypeStr}};

	__block VuoCompilerMakeListNodeClass *nodeClass;

	if (! dynamic_cast<VuoGenericType *>(itemType->getBase()))
	{
		// The generic port types have been specialized, so generate LLVM bitcode for the node class.

		dispatch_sync(llvmQueue, ^{

						  vector<Type *> itemParamTypes = itemType->getFunctionParameterTypes();
						  AttributeList itemFunctionAttributes = itemType->getFunctionAttributes();

						  Module *module = new Module("", *VuoCompiler::globalLLVMContext);

						  // VuoModuleMetadata({…});
						  json_object *metadata = json_object_new_object();
						  json_object_object_add(metadata, "title", json_object_new_string("Make List"));
						  json_object_object_add(metadata, "description", json_object_new_string(makeListNodeClassDescription.c_str()));
						  json_object_object_add(metadata, "version", json_object_new_string("2.0.0"));
						  json_object *specializedModuleDetails = buildSpecializedModuleDetails(specializedForGenericTypeName);
						  json_object_object_add(metadata, "specializedModule", specializedModuleDetails);
						  string metadataStr = json_object_to_json_string(metadata);
						  json_object_put(metadata);
						  VuoCompilerCodeGenUtilities::generateModuleMetadata(module, metadataStr, "");


						  // void nodeEvent
						  // (
						  //	VuoInputData(<item type>,<default value>) item1,
						  //	...,
						  //	VuoInputData(<item type>,<default value>) item<item count>,
						  //	VuoOutputData(VuoList_<item type>) list
						  // )

						  vector<VuoPort *> modelInputPorts;
						  for (unsigned long i = 0; i < itemCount; ++i)
						  {
							  ostringstream oss;
							  oss << i+1;
							  string portName = oss.str();

							  VuoCompilerInputEventPortClass *modelItemPortClass = new VuoCompilerInputEventPortClass(portName);
							  VuoCompilerInputDataClass *dataClass = new VuoCompilerInputDataClass("");
							  dataClass->setVuoType( itemType->getBase() );
							  modelItemPortClass->setDataClass(dataClass);
							  VuoCompilerPort *modelItemPort = modelItemPortClass->newPort();

							  modelInputPorts.push_back( modelItemPort->getBase() );
						  }

						  VuoCompilerOutputEventPortClass *modelListPortClass = new VuoCompilerOutputEventPortClass("list");
						  VuoCompilerOutputDataClass *dataClass = new VuoCompilerOutputDataClass("");
						  dataClass->setVuoType( listType->getBase() );
						  modelListPortClass->setDataClass(dataClass);
						  VuoCompilerPort *modelListPort = modelListPortClass->newPort();
						  vector<VuoPort *> modelOutputPorts( 1, modelListPort->getBase() );
						  map<VuoPort *, size_t> indexOfParameter;
						  map<VuoPort *, size_t> indexOfEventParameter;
						  VuoCompilerConstantsCache constantsCache(module);

						  Function *eventFunction = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, "", false, false,
																									  nullptr, modelInputPorts, modelOutputPorts,
																									  map<VuoPort *, json_object *>(), map<VuoPort *, string>(),
																									  map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
																									  indexOfParameter, indexOfEventParameter, &constantsCache);


						  // {
						  //		*list = VuoListCreate_<item type>();
						  //		VuoListAppendValue_<item type>(*list, item1);
						  //		...
						  //		VuoListAppendValue_<item type>(*list, item<item count>);
						  // }

						  BasicBlock *block = &(eventFunction->getEntryBlock());

						  string itemBackingTypeName;
						  if (VuoGenericType::isGenericTypeName(itemTypeStr))
						  {
							  itemBackingTypeName = VuoCompilerGenericType::chooseBackingTypeName(itemTypeStr, vector<string>());
						  }
						  else
						  {
							  itemBackingTypeName = itemTypeStr;
						  }

						  string listCreateFunctionName = "VuoListCreate_" + itemBackingTypeName;
						  Function *listCreateFunctionSrc = listType->getModule()->getFunction(listCreateFunctionName);
						  Function *listCreateFunction = declareFunctionInModule(module, listCreateFunctionSrc);

						  string listAppendFunctionName = "VuoListAppendValue_" + itemBackingTypeName;
						  Function *listAppendFunctionSrc = listType->getModule()->getFunction(listAppendFunctionName);
						  Function *listAppendFunction = declareFunctionInModule(module, listAppendFunctionSrc);

						  vector<Value *> itemValues;
						  for (VuoPort *itemPort : modelInputPorts)
						  {
							  size_t argIndex = indexOfParameter[itemPort];
							  Value *itemValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, argIndex);
							  itemValues.push_back(itemValue);
							  if (itemParamTypes.size() > 1)
							  {
								  Value *itemSecondValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, argIndex + 1);
								  itemValues.push_back(itemSecondValue);
							  }
						  }
						  size_t listArgIndex = indexOfParameter[modelListPort->getBase()];
						  Value *listArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(eventFunction, listArgIndex);

						  CallInst *listValue = CallInst::Create(listCreateFunction, "", block);
						  Value *listPointer = VuoCompilerCodeGenUtilities::generatePointerToValue(block, listValue);
						  VuoCompilerCodeGenUtilities::generateMemoryCopy(module, block, listPointer, listArg, listType);

						  for (unsigned long i = 0; i < itemCount; ++i)
						  {
							  vector<Value *> listAppendArgs;
							  listAppendArgs.push_back(listValue);

							  int itemValuesIndex = (itemParamTypes.size() > 1 ? 2*i : i);

							  Value *itemArg = itemValues[itemValuesIndex];

							  if (VuoCompilerCodeGenUtilities::isPointerToStruct(itemArg->getType()))
								  itemArg = new BitCastInst(itemArg, listAppendFunction->getFunctionType()->getParamType(1), "", block);

							  listAppendArgs.push_back(itemArg);

							  if (itemParamTypes.size() > 1)
								  listAppendArgs.push_back(itemValues[itemValuesIndex + 1]);

							  CallInst *call = CallInst::Create(listAppendFunction, listAppendArgs, "", block);

							  VuoCompilerCodeGenUtilities::copyParameterAttributes(module, itemFunctionAttributes, 0, listAppendArgs.size()-1, call, 1);
						  }

						  ReturnInst::Create(module->getContext(), block);


						  for (vector<VuoPort *>::iterator i = modelInputPorts.begin(); i != modelInputPorts.end(); ++i)
						  {
							  delete (*i)->getClass()->getCompiler();
							  delete *i;
						  }
						  delete modelListPort;
						  delete modelListPortClass;


						  VuoCompilerMakeListNodeClass *dummyNodeClass = new VuoCompilerMakeListNodeClass(nodeClassName, module);

						  // Reconstruct, this time with a base VuoNodeClass containing actual (non-dummy) ports.
						  nodeClass = new VuoCompilerMakeListNodeClass(dummyNodeClass);
						  delete dummyNodeClass;
					  });

		string typeDependency = itemType->getDependencyPath();
		vector<string> typeDependencies;
		if (! typeDependency.empty())
			typeDependencies.push_back(typeDependency);
		nodeClass->makeDependencies = VuoMakeDependencies::createFromComponents(VuoMakeDependencies::getPlaceholderCompiledFilePath(), typeDependencies);
	}
	else
	{
		// The generic ports have not been specialized, so construct a node class that doesn't yet have an implementation.

		VuoPortClass *refreshPortClass = (new VuoCompilerInputEventPortClass("refresh"))->getBase();

		vector<VuoPortClass *> inputPortClasses;
		inputPortClasses.push_back(refreshPortClass);
		for (int i = 1; i <= itemCount; ++i)
		{
			ostringstream oss;
			oss << i;
			VuoCompilerInputEventPortClass *portClass = new VuoCompilerInputEventPortClass(oss.str());
			VuoCompilerInputDataClass *dataClass = new VuoCompilerInputDataClass("");
			portClass->setDataClass(dataClass);
			portClass->setDataVuoType(itemType->getBase());
			inputPortClasses.push_back(portClass->getBase());
		}

		vector<VuoPortClass *> outputPortClasses;
		{
			VuoCompilerOutputEventPortClass *portClass = new VuoCompilerOutputEventPortClass("list");
			VuoCompilerOutputDataClass *dataClass = new VuoCompilerOutputDataClass("");
			portClass->setDataClass(dataClass);
			portClass->setDataVuoType(listType->getBase());
			outputPortClasses.push_back(portClass->getBase());
		}

		VuoNodeClass *baseNodeClass = new VuoNodeClass(nodeClassName, refreshPortClass, inputPortClasses, outputPortClasses);
		nodeClass = new VuoCompilerMakeListNodeClass(baseNodeClass);

		nodeClass->itemCount = itemCount;
		nodeClass->listType = listType;
		nodeClass->listTypeName = listType->getBase()->getModuleKey();
		nodeClass->specializedForGenericTypeName = specializedForGenericTypeName;

		nodeClass->getBase()->setDefaultTitle("Make List");
		nodeClass->getBase()->setDescription(makeListNodeClassDescription);
		nodeClass->getBase()->setVersion("2.0.0");
	}

	return nodeClass->getBase();
}

/**
 * Creates a compiler and base node class from the node class implementation in the module,
 * or returns null if the implementation is not of a Make List node class.
 */
VuoNodeClass * VuoCompilerMakeListNodeClass::newNodeClass(string nodeClassName, Module *module)
{
	if (isMakeListNodeClassName(nodeClassName))
	{
		VuoCompilerMakeListNodeClass *cnc = new VuoCompilerMakeListNodeClass(nodeClassName, module);
		VuoCompilerMakeListNodeClass *cnc2 = new VuoCompilerMakeListNodeClass(cnc);
		delete cnc;
		return cnc2->getBase();
	}

	return nullptr;
}

/**
 * Returns true if the name has the format of a "Make List" node class name.
 * (A "Make List" node class by that name may or may not exist.)
 */
bool VuoCompilerMakeListNodeClass::isMakeListNodeClassName(string nodeClassName)
{
	return VuoStringUtilities::beginsWith(nodeClassName, makeListNodeClassNamePrefix + ".");
}

/**
 * Returns the name that a "Make List" node class would have if it were to input the given number of items
 * and output the given type of list.
 */
string VuoCompilerMakeListNodeClass::getNodeClassName(unsigned long itemCount, VuoCompilerType *listType)
{
	string itemTypeStr = VuoStringUtilities::substrAfter(listType->getBase()->getModuleKey(), VuoType::listTypeNamePrefix);
	return buildNodeClassName(itemCount, itemTypeStr);
}

/**
 * Parses item count and type from the "Make List" node class name.
 *
 * @return True if the node class name was successfully parsed, false otherwise.
 */
bool VuoCompilerMakeListNodeClass::parseNodeClassName(string nodeClassName, unsigned long &itemCount, string &itemTypeName)
{
	string itemCountAndType = VuoStringUtilities::substrAfter(nodeClassName, makeListNodeClassNamePrefix + ".");
	size_t dotPos = itemCountAndType.find(".");
	if (dotPos == string::npos || dotPos == 0 || dotPos == itemCountAndType.length() - 1)
		return false;

	string itemCountStr = itemCountAndType.substr(0, dotPos);
	itemCount = atol(itemCountStr.c_str());
	itemTypeName = itemCountAndType.substr(dotPos + 1);
	return true;
}

/**
 * Creates a "Make List" node class name from the item count and type.
 */
string VuoCompilerMakeListNodeClass::buildNodeClassName(unsigned long itemCount, string itemTypeName)
{
	ostringstream oss;
	oss << itemCount;
	string itemCountStr = oss.str();

	return makeListNodeClassNamePrefix + "." + itemCountStr + "." + itemTypeName;
}

/**
 * Returns the number of item input ports for this "Make List" node class.
 */
unsigned long VuoCompilerMakeListNodeClass::getItemCount(void)
{
	return itemCount;
}

/**
 * Returns the type of the list output port for this "Make List" node class.
 */
VuoCompilerType * VuoCompilerMakeListNodeClass::getListType(void)
{
	return listType;
}

/**
 * Attempts to update the stored reference to the type of this node class's output port.
 *
 * Returns true if the type was actually found by @a lookUpNodeClass and the reference was updated.
 */
bool VuoCompilerMakeListNodeClass::updateListType(std::function<VuoCompilerType *(const string &)> lookUpType)
{
	listType = lookUpType(listTypeName);
	return listType != nullptr;
}

/**
 * Returns this port's type in the (hypothetical) unspecialized Make List node class.
 */
VuoType * VuoCompilerMakeListNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	if (portClass->getPortType() != VuoPortClass::dataAndEventPort)
		return NULL;

	string typeName = VuoGenericType::createGenericTypeName(1);
	return new VuoGenericType(typeName, vector<string>());
}

/**
 * Returns the original node's class name (without any type suffixes).
 */
string VuoCompilerMakeListNodeClass::getOriginalGenericNodeClassName(void)
{
	return makeListNodeClassNamePrefix;
}

/**
 * Returns the original node's class description.
 */
string VuoCompilerMakeListNodeClass::getOriginalGenericNodeClassDescription(void)
{
	return makeListNodeClassDescription;
}

/**
 * Returns the original node's node set.
 */
VuoNodeSet * VuoCompilerMakeListNodeClass::getOriginalGenericNodeSet(void)
{
	/// @todo somehow return the vuo.list node set
	return NULL;
}

/**
 * Returns the name for the Make List node class that would result if the given port were changed back to its original type.
 */
string VuoCompilerMakeListNodeClass::createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize)
{
	bool foundDataAndEvent = false;
	for (set<VuoPortClass *>::iterator i = portClassesToUnspecialize.begin(); i != portClassesToUnspecialize.end(); ++i)
		if ((*i)->getPortType() == VuoPortClass::dataAndEventPort)
			foundDataAndEvent = true;

	if (! foundDataAndEvent)
		return getBase()->getClassName();

	return buildNodeClassName(itemCount, VuoGenericType::createGenericTypeName(1));
}

/**
 * Returns the name for the Make List node class that would result if the given specialized type were substituted for the
 * generic item type.
 */
string VuoCompilerMakeListNodeClass::createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName)
{
	unsigned long itemCount = 0;
	string itemTypeName;
	parseNodeClassName(getBase()->getClassName(), itemCount, itemTypeName);

	string specializedItemTypeName = (itemTypeName == genericTypeName ? specializedTypeName : itemTypeName);
	return buildNodeClassName(itemCount, specializedItemTypeName);
}
