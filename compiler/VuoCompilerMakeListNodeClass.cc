/**
 * @file
 * VuoCompilerMakeListNodeClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoPort.hh"
#include "VuoStringUtilities.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerMakeListNodeClass.hh"


/// The common beginning of all "Make List" node class names (before the item count and item type).
const string VuoCompilerMakeListNodeClass::makeListNodeClassNamePrefix = "vuo.list.make.";

/// The common description of all "Make List" node classes.
const string VuoCompilerMakeListNodeClass::makeListNodeClassDescription = "Creates a list from the given items.";


/**
 * Creates a Make List node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(string nodeClassName, Module *module) :
	VuoCompilerSpecializedNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass, VuoNode *nodeToBack) :
	VuoCompilerSpecializedNodeClass(compilerNodeClass, nodeToBack)
{
}

/**
 * Generates a node class for a "Make List" node.
 * The node class's event function inputs zero or more items and outputs a list of those items.
 *
 * @param nodeClassName The name of the node class to generate. It should have the format "vuo.list.make.<item count>.<item type>" (e.g. "vuo.list.make.3.VuoInteger").
 * @param compiler The compiler to use for looking up types.
 * @param nodeToBack Optionally, a 'Make List' node whose generic types should be used to determine this node class's backing types.
 * @return The generated node class, or null if the node class name does not have the correct format.
 */
VuoNodeClass * VuoCompilerMakeListNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler, VuoNode *nodeToBack)
{
	string itemCountAndType = VuoStringUtilities::substrAfter(nodeClassName, makeListNodeClassNamePrefix);
	size_t dotPos = itemCountAndType.find(".");
	if (dotPos == string::npos || dotPos == 0 || dotPos == itemCountAndType.length() - 1)
		return NULL;

	string itemCountStr = itemCountAndType.substr(0, dotPos);
	unsigned long itemCount = atol(itemCountStr.c_str());
	string itemTypeStr = itemCountAndType.substr(dotPos + 1);
	VuoCompilerType *itemType;
	VuoCompilerType *listType;
	if (nodeToBack)
	{
		VuoCompilerPort *itemPort = static_cast<VuoCompilerPort *>(nodeToBack->getInputPorts().back()->getCompiler());
		itemType = itemPort->getDataVuoType()->getCompiler();
		VuoCompilerPort *listPort = static_cast<VuoCompilerPort *>(nodeToBack->getOutputPorts().back()->getCompiler());
		listType = listPort->getDataVuoType()->getCompiler();
	}
	else
	{
		itemType = compiler->getType(itemTypeStr);
		listType = compiler->getType(VuoType::listTypeNamePrefix + itemTypeStr);
	}
	Type *pointerToListType = PointerType::get(listType->getType(), 0);

	Type *itemParamSecondType = NULL;
	Type *itemParamType = itemType->getFunctionParameterType(&itemParamSecondType);
	Attributes itemParamAttributes = itemType->getFunctionParameterAttributes();

	Module *module = new Module("", getGlobalContext());
	Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	// const char *moduleDetails = ...;
	string moduleDetails =
			"{ \"title\" : \"Make List\", "
			"\"description\" : \"" + makeListNodeClassDescription + "\", "
			"\"version\" : \"1.0.0\" }";
	Constant *moduleDetailsValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, moduleDetails, ".str");  // VuoCompilerBitcodeParser::resolveGlobalToConst requires that the variable have a name
	GlobalVariable *moduleDetailsVariable = new GlobalVariable(*module, pointerToCharType, false, GlobalValue::ExternalLinkage, 0, "moduleDetails");
	moduleDetailsVariable->setInitializer(moduleDetailsValue);


	// void nodeEvent
	// (
	//	VuoInputData(<item type>,<default value>) item1,
	//	...,
	//	VuoInputData(<item type>,<default value>) item<item count>,
	//	VuoOutputData(VuoList_<item type>) list
	// )

	vector<Type *> eventFunctionParams;
	for (unsigned long i = 0; i < itemCount; ++i)
	{
		eventFunctionParams.push_back(itemParamType);
		if (itemParamSecondType)
			eventFunctionParams.push_back(itemParamSecondType);
	}
	eventFunctionParams.push_back(pointerToListType);
	FunctionType *eventFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), eventFunctionParams, false);
	Function *eventFunction = Function::Create(eventFunctionType, GlobalValue::ExternalLinkage, "nodeEvent", module);

	for (unsigned long i = 0; i < itemCount; ++i)
	{
		int attributeIndex = (itemParamSecondType ? 2*i + 1 : i + 1);
		eventFunction->addAttribute(attributeIndex, itemParamAttributes);
		if (itemParamSecondType)
			eventFunction->addAttribute(attributeIndex + 1, itemParamAttributes);
	}

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", eventFunction, 0);
	Function::arg_iterator argIter = eventFunction->arg_begin();
	for (unsigned long i = 0; i < itemCount; ++i)
	{
		Value *arg = argIter++;
		ostringstream oss;
		oss << "item" << i+1;
		string argName = oss.str();
		arg->setName(argName);

		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoType:" + itemType->getBase()->getModuleKey(), "", 0);
		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoInputData", "", 0);

		if (itemParamSecondType)
		{
			arg = argIter++;
			argName += ".1";
			arg->setName(argName);
		}
	}
	{
		Value *arg = argIter++;
		string argName = "list";
		arg->setName(argName);

		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoType:" + listType->getBase()->getModuleKey(), "", 0);
		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoOutputData", "", 0);
	}


	// {
	//		*list = VuoListCreate_<item type>();
	//		VuoListAppendValue_<item type>(*list, item1);
	//		...
	//		VuoListAppendValue_<item type>(*list, item<item count>);
	// }

	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	string itemBackingTypeName;
	if (nodeToBack)
		itemBackingTypeName = static_cast<VuoCompilerGenericType *>(itemType)->getBackingTypeName();
	else if (VuoGenericType::isGenericTypeName(itemTypeStr))
		itemBackingTypeName = VuoCompilerGenericType::chooseBackingTypeName(itemTypeStr, set<string>());
	else
		itemBackingTypeName = itemTypeStr;

	string listCreateFunctionName = "VuoListCreate_" + itemBackingTypeName;
	Function *listCreateFunction = module->getFunction(listCreateFunctionName);
	if (! listCreateFunction)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(voidPointerType, functionParams, false);
		listCreateFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, listCreateFunctionName, module);
	}

	string listAppendFunctionName = "VuoListAppendValue_" + itemBackingTypeName;
	Function *listAppendFunction = module->getFunction(listAppendFunctionName);
	if (! listAppendFunction)
	{
		vector<Type *> functionParams;
		functionParams.push_back(voidPointerType);
		functionParams.push_back(itemParamType);
		if (itemParamSecondType)
			functionParams.push_back(itemParamSecondType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		listAppendFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, listAppendFunctionName, module);

		listAppendFunction->addAttribute(2, itemParamAttributes);
	}

	argIter = eventFunction->arg_begin();
	vector<Value *> itemValues;
	for (unsigned long i = 0; i < itemCount; ++i)
	{
		itemValues.push_back(argIter++);
		if (itemParamSecondType)
			itemValues.push_back(argIter++);
	}
	Value *listVariable = argIter++;

	CallInst *listValue = CallInst::Create(listCreateFunction, "", block);
	new StoreInst(listValue, listVariable, false, block);

	for (unsigned long i = 0; i < itemCount; ++i)
	{
		int itemValuesIndex = (itemParamSecondType ? 2*i : i);
		vector<Value *> listAppendParams;
		listAppendParams.push_back(listValue);
		listAppendParams.push_back(itemValues[itemValuesIndex]);
		if (itemParamSecondType)
			listAppendParams.push_back(itemValues[itemValuesIndex + 1]);
		CallInst *call = CallInst::Create(listAppendFunction, listAppendParams, "", block);

		call->addAttribute(2, itemType->getFunctionParameterAttributes());
	}

	ReturnInst::Create(module->getContext(), block);


	VuoCompilerMakeListNodeClass *dummyNodeClass = new VuoCompilerMakeListNodeClass(nodeClassName, module);

	// Reconstruct, this time with a base VuoNodeClass containing actual (non-dummy) ports.
	VuoCompilerMakeListNodeClass *nodeClass = new VuoCompilerMakeListNodeClass(dummyNodeClass, nodeToBack);
	delete dummyNodeClass;

	nodeClass->itemCount = itemCount;
	nodeClass->listType = listType;
	nodeClass->specializedForGenericTypeName[ VuoGenericType::createGenericTypeName(1) ] = itemTypeStr;

	return nodeClass->getBase();
}

/**
 * Returns true if the name has the format of a "Make List" node class name.
 * (A "Make List" node class by that name may or may not exist.)
 */
bool VuoCompilerMakeListNodeClass::isMakeListNodeClassName(string nodeClassName)
{
	return VuoStringUtilities::beginsWith(nodeClassName, makeListNodeClassNamePrefix);
}

/**
 * Returns true if the given type is a VuoList type.
 */
bool VuoCompilerMakeListNodeClass::isListType(VuoCompilerType *type)
{
	if (! type)
		return false;

	return VuoStringUtilities::beginsWith(type->getBase()->getModuleKey(), VuoType::listTypeNamePrefix);
}

/**
 * Returns the name that a "Make List" node class would have if it were to input the given number of items
 * and output the given type of list.
 */
string VuoCompilerMakeListNodeClass::getNodeClassName(unsigned long itemCount, VuoCompilerType *listType)
{
	ostringstream oss;
	oss << itemCount;
	string itemCountStr = oss.str();

	string itemTypeStr = VuoStringUtilities::substrAfter(listType->getBase()->getModuleKey(), VuoType::listTypeNamePrefix);

	return makeListNodeClassNamePrefix + itemCountStr + "." + itemTypeStr;
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
 * Returns this port's type in the (hypothetical) unspecialized Make List node class.
 */
VuoType * VuoCompilerMakeListNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	if (portClass->getPortType() != VuoPortClass::dataAndEventPort)
		return NULL;

	string typeName = VuoGenericType::createGenericTypeName(1);
	return new VuoGenericType(typeName, set<string>());
}

/**
 * Returns the original node's class name (without any type suffixes).
 */
string VuoCompilerMakeListNodeClass::getOriginalGenericNodeClassName(void)
{
	return "vuo.list.make";
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
VuoNodeSet *VuoCompilerMakeListNodeClass::getOriginalGenericNodeSet(void)
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

	ostringstream unspecializedNodeClassName;
	unspecializedNodeClassName << makeListNodeClassNamePrefix << itemCount << "." << VuoGenericType::createGenericTypeName(1);
	return unspecializedNodeClassName.str();
}
