/**
 * @file
 * VuoCompilerMakeListNodeClass implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>

#include "VuoStringUtilities.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerMakeListNodeClass.hh"


/// The common beginning of all "Make List" node class names (before the item count and item type).
const string VuoCompilerMakeListNodeClass::makeListNodeClassNamePrefix = "vuo.list.make.";
/// The common beginning of all VuoList type names (before the item type name).
const string VuoCompilerMakeListNodeClass::listTypeNamePrefix = "VuoList_";


/**
 * Creates a Make List node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(string nodeClassName, Module *module) :
	VuoCompilerNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerMakeListNodeClass::VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass) :
	VuoCompilerNodeClass(compilerNodeClass)
{
}

/**
 * Generates a node class for a "Make List" node.
 * The node class's event function inputs zero or more items and outputs a list of those items.
 *
 * @param nodeClassName The name of the node class to generate. It should have the format "vuo.list.make.<item count>.<item type>" (e.g. "vuo.list.make.3.VuoInteger").
 * @param compiler The compiler to use for looking up types.
 * @return The generated node class, or null if the node class name does not have the correct format.
 */
VuoNodeClass * VuoCompilerMakeListNodeClass::newNodeClass(string nodeClassName, VuoCompiler *compiler)
{
	string itemCountAndType = VuoStringUtilities::substrAfter(nodeClassName, makeListNodeClassNamePrefix);
	size_t dotPos = itemCountAndType.find(".");
	if (dotPos == string::npos || dotPos == 0 || dotPos == itemCountAndType.length() - 1)
		return NULL;

	string itemCountStr = itemCountAndType.substr(0, dotPos);
	unsigned long itemCount = atol(itemCountStr.c_str());
	string itemTypeStr = itemCountAndType.substr(dotPos + 1);
	VuoCompilerType *itemType = compiler->getType(itemTypeStr);
	VuoCompilerType *listType = compiler->getType(listTypeNamePrefix + itemTypeStr);
	Type *pointerToListType = PointerType::get(listType->getType(), 0);

	Type *itemParamSecondType = NULL;
	Type *itemParamType = itemType->getFunctionParameterType(&itemParamSecondType);
	Attributes itemParamAttributes = itemType->getFunctionParameterAttributes();

	Module *module = new Module("", getGlobalContext());
	Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	// const char *moduleDetails = ...;
	string moduleDetails =
			"{ \"title\" : \"Make List\", "
			"\"description\" : \"Creates a list from the given items\", "
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

	string listCreateFunctionName = "VuoListCreate_" + itemType->getBase()->getModuleKey();
	Function *listCreateFunction = module->getFunction(listCreateFunctionName);
	if (! listCreateFunction)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(voidPointerType, functionParams, false);
		listCreateFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, listCreateFunctionName, module);
	}

	string listAppendFunctionName = "VuoListAppendValue_" + itemType->getBase()->getModuleKey();
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
		CallInst::Create(listAppendFunction, listAppendParams, "", block);
	}

	ReturnInst::Create(module->getContext(), block);


	VuoCompilerMakeListNodeClass *dummyNodeClass = new VuoCompilerMakeListNodeClass(nodeClassName, module);

	// Reconstruct, this time with a base VuoNodeClass containing actual (non-dummy) ports.
	VuoCompilerMakeListNodeClass *nodeClass = new VuoCompilerMakeListNodeClass(dummyNodeClass);
	delete dummyNodeClass;

	nodeClass->itemCount = itemCount;
	nodeClass->listType = listType;

	return nodeClass->getBase();
}

/**
 * Retrieves the VuoCompilerMakeListNodeClass with the given name if the compiler already has it,
 * or else creates a new VuoCompilerMakeListNodeClass and adds it to the compiler.
 *
 * @param nodeClassName The name of the node class. It should have the format "vuo.list.make.<item count>.<item type>" (e.g. "vuo.list.make.3.VuoInteger").
 * @param compiler The compiler to use for looking up node classes.
 * @return The retrieved or created node class, or null if the node class name does not have the correct format.
 */
VuoCompilerMakeListNodeClass * VuoCompilerMakeListNodeClass::getNodeClass(string nodeClassName, VuoCompiler *compiler)
{
	VuoCompilerMakeListNodeClass *nodeClass = dynamic_cast<VuoCompilerMakeListNodeClass *>(compiler->getNodeClass(nodeClassName));
	if (! nodeClass)
	{
		nodeClass = static_cast<VuoCompilerMakeListNodeClass *>(newNodeClass(nodeClassName, compiler)->getCompiler());
		if (nodeClass)
			compiler->addNodeClass(nodeClass);
	}
	return nodeClass;
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

	return VuoStringUtilities::beginsWith(type->getBase()->getModuleKey(), listTypeNamePrefix);
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

	string itemTypeStr = VuoStringUtilities::substrAfter(listType->getBase()->getModuleKey(), listTypeNamePrefix);

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
