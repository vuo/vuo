/**
 * @file
 * VuoCompilerType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop
#include <sstream>

#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerType.hh"


/**
 * Creates a type from an LLVM module, and creates its corresponding base @c VuoType.
 */
VuoCompilerType::VuoCompilerType(string typeName, Module *module)
	: VuoBaseDetail<VuoType>("VuoCompilerType", new VuoType(typeName)),
	  VuoCompilerModule(getBase(), module)
{
	getBase()->setCompiler(this);

	makeFromJsonFunction = NULL;
	getJsonFunction = NULL;
	getInterprocessJsonFunction = NULL;
	makeFromStringFunction = NULL;
	getStringFunction = NULL;
	getInterprocessStringFunction = NULL;
	getSummaryFunction = NULL;
	retainFunction = NULL;
	releaseFunction = NULL;
	llvmType = NULL;

	parse();
}

/**
 * Creates a type from the type definition in the module. If the module does not contain
 * a type definition, returns NULL.
 */
VuoCompilerType * VuoCompilerType::newType(string typeName, Module *module)
{
	if (! isType(typeName, module))
		return NULL;

	return new VuoCompilerType(typeName, module);
}

/**
 * Returns true if the LLVM module contains certain global symbols, indicating that it is
 * intended to be a type definition.
 */
bool VuoCompilerType::isType(string typeName, Module *module)
{
	return (module->getNamedValue(typeName + "_makeFromJson") != NULL);
}

/**
 * Overrides the implementation in @c VuoCompilerModule.
 */
void VuoCompilerType::parse(void)
{
	VuoCompilerModule::parse();

	string typeName = getBase()->getModuleKey();
	makeFromJsonFunction = parser->getFunction(typeName + "_makeFromJson");
	getJsonFunction = parser->getFunction(typeName + "_getJson");
	getInterprocessJsonFunction = parser->getFunction(typeName + "_getInterprocessJson");
	getSummaryFunction = parser->getFunction(typeName + "_getSummary");

	if (! makeFromJsonFunction)
		VUserLog("Error: Couldn't find %s_makeFromJson() function.", typeName.c_str());
	if (! getJsonFunction)
		VUserLog("Error: Couldn't find %s_getJson() function.", typeName.c_str());
	if (! getSummaryFunction)
		VUserLog("Error: Couldn't find %s_getSummaryFunction() function.", typeName.c_str());

	llvmType = VuoCompilerCodeGenUtilities::getParameterTypeBeforeLowering(getJsonFunction, module, typeName);

	parseOrGenerateValueFromStringFunction();
	parseOrGenerateStringFromValueFunction(false);
	if (getInterprocessJsonFunction)
		parseOrGenerateStringFromValueFunction(true);

	parseOrGenerateRetainOrReleaseFunction(true);
	parseOrGenerateRetainOrReleaseFunction(false);
}

/**
 * Overrides the implementation in @c VuoCompilerModule.
 */
set<string> VuoCompilerType::globalsToRename(void)
{
	set<string> globals = VuoCompilerModule::globalsToRename();
	return globals;
}

/**
 * When compiling a type, adds a @c [Type]_makeFromString() function to the type definition.
 *
 * @eg{
 * VuoBoolean VuoBoolean_makeFromString(const char *str)
 * {
 * 	json_object * js = json_tokener_parse(str);
 * 	VuoBoolean value = VuoBoolean_makeFromJson(js);
 * 	json_object_put(js);
 * 	return value;
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that makeFromJsonFunction has been parsed and llvmType has been set.
 */
void VuoCompilerType::parseOrGenerateValueFromStringFunction(void)
{
	string functionName = (getBase()->getModuleKey() + "_makeFromString").c_str();
	Function *function = parser->getFunction(functionName);

	bool isReturnInParam = VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(makeFromJsonFunction);

	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		FunctionType *makeFromJsonFunctionType = makeFromJsonFunction->getFunctionType();

		Type *returnType = makeFromJsonFunctionType->getReturnType();

		vector<Type *> functionParams;
		if (isReturnInParam)
			functionParams.push_back(makeFromJsonFunctionType->getParamType(0));
		functionParams.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		if (isReturnInParam)
			function->addAttribute(1, makeFromJsonFunction->getAttributes().getParamAttributes(1));
	}

	if (function->isDeclaration())
	{
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function);

		Function::arg_iterator args = function->arg_begin();
		Value *result = NULL;
		if (isReturnInParam)
		{
			result = args++;
			result->setName("result");
		}
		Value *str = args++;
		str->setName("str");

		Function *jsonTokenerParseFunction = VuoCompilerCodeGenUtilities::getJsonTokenerParseFunction(module);
		Function *jsonObjectPutFunction = VuoCompilerCodeGenUtilities::getJsonObjectPutFunction(module);

		CallInst *jsonTokenerParseReturn = CallInst::Create(jsonTokenerParseFunction, str, "", block);

		vector<Value *> makeFromJsonArgs;
		if (isReturnInParam)
			makeFromJsonArgs.push_back(result);
		makeFromJsonArgs.push_back(jsonTokenerParseReturn);
		Value *makeFromJsonReturn = CallInst::Create(makeFromJsonFunction, makeFromJsonArgs, "", block);

		CallInst::Create(jsonObjectPutFunction, jsonTokenerParseReturn, "", block);

		Value *returnValue = (isReturnInParam ? NULL : makeFromJsonReturn);
		ReturnInst::Create(module->getContext(), returnValue, block);
	}

	makeFromStringFunction = function;
}

/**
 * When compiling a type, adds a @c [Type]_getString() or @c [Type]_getInterprocessString()
 * function to the type definition.
 *
 * @eg{
 * char * VuoBoolean_getString(const VuoBoolean value)
 * {
 * 	json_object *js = VuoBoolean_getJson(value);
 * 	char *string = strdup(json_object_to_json_string_ext(js,JSON_C_TO_STRING_PLAIN));
 * 	json_object_put(js);
 * 	return string;
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that getJsonFunction or getInterprocessJsonFunction has been parsed.
 */
void VuoCompilerType::parseOrGenerateStringFromValueFunction(bool isInterprocess)
{
	string functionName = (getBase()->getModuleKey() + (isInterprocess ? "_getInterprocessString" : "_getString")).c_str();
	Function *function = parser->getFunction(functionName);

	Function *chosenJsonFromValueFunction = (isInterprocess ? getInterprocessJsonFunction : getJsonFunction);

	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		FunctionType *getJsonFunctionType = chosenJsonFromValueFunction->getFunctionType();

		Type *returnType = pointerToCharType;

		vector<Type *> functionParams;
		for (int i = 0; i < getJsonFunctionType->getNumParams(); ++i)
			functionParams.push_back(getJsonFunctionType->getParamType(i));

		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		for (int i = 0; i < getJsonFunctionType->getNumParams(); ++i)
			function->addAttribute(i+1, chosenJsonFromValueFunction->getAttributes().getParamAttributes(1));
	}

	if (function->isDeclaration())
	{
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function);

		int argIndex = 0;
		for (Function::arg_iterator args = function->arg_begin(); args != function->arg_end(); ++args, ++argIndex)
		{
			Value *value = args;
			ostringstream argName;
			argName << "value" << argIndex;
			value->setName(argName.str().c_str());
		}

		Function *jsonObjectToJsonStringExtFunction = VuoCompilerCodeGenUtilities::getJsonObjectToJsonStringExtFunction(module);
		Function *strdupFunction = VuoCompilerCodeGenUtilities::getStrdupFunction(module);
		Function *jsonObjectPutFunction = VuoCompilerCodeGenUtilities::getJsonObjectPutFunction(module);

		vector<Value *> getJsonArgs;
		for (Function::arg_iterator args = function->arg_begin(); args != function->arg_end(); ++args)
		{
			Value *value = args;
			getJsonArgs.push_back(value);
		}
		CallInst *getJsonReturn = CallInst::Create(chosenJsonFromValueFunction, getJsonArgs, "", block);

		vector<Value *> jsonObjectToJsonStringExtArgs;
		jsonObjectToJsonStringExtArgs.push_back(getJsonReturn);
		jsonObjectToJsonStringExtArgs.push_back(ConstantInt::get(module->getContext(), APInt(32, JSON_C_TO_STRING_PLAIN)));
		CallInst *jsonObjectToJsonStringExtReturn = CallInst::Create(jsonObjectToJsonStringExtFunction, jsonObjectToJsonStringExtArgs, "", block);

		CallInst *strdupReturn = CallInst::Create(strdupFunction, jsonObjectToJsonStringExtReturn, "", block);

		CallInst::Create(jsonObjectPutFunction, getJsonReturn, "", block);

		ReturnInst::Create(module->getContext(), strdupReturn, block);
	}

	if (isInterprocess)
		getInterprocessStringFunction = function;
	else
		getStringFunction = function;
}

/**
 * When compiling a type, adds a @c [Type]_retain() or @c [Type]_release() function to the type definition.
 *
 * @eg{
 * void VuoSceneObject_retain(VuoSceneObject value)
 * {
 *   VuoRetain((void *)value.mesh);
 *   VuoRetain((void *)value.shader);
 *   VuoRetain((void *)value.childObjects);
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that llvmType has been set.
 */
void VuoCompilerType::parseOrGenerateRetainOrReleaseFunction(bool isRetain)
{
	string functionName = (getBase()->getModuleKey() + (isRetain ? "_retain" : "_release")).c_str();
	Function *function = parser->getFunction(functionName);

	if (! function)
	{
		Type *secondParamType = NULL;
		Type *firstParamType = getFunctionParameterType(&secondParamType);
		vector<Type *> functionParams;
		functionParams.push_back(firstParamType);
		if (secondParamType)
			functionParams.push_back(secondParamType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
		function->addAttribute(1, getFunctionParameterAttributes());
	}

	if (function->isDeclaration())
	{
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function);

		Function::arg_iterator args = function->arg_begin();
		Value *arg = args++;
		arg->setName("value");

		arg = VuoCompilerCodeGenUtilities::generateTypeCast(module, block, arg, llvmType);

		if (isRetain)
			VuoCompilerCodeGenUtilities::generateRetainCall(module, block, arg);
		else
			VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, arg);

		ReturnInst::Create(module->getContext(), block);
	}

	if (isRetain)
		retainFunction = function;
	else
		releaseFunction = function;
}

/**
 * Generates a call to @c [Type]_makeFromString().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_makeFromString().
 * @return The return value of @c [Type]_makeFromString().
 */
Value * VuoCompilerType::generateValueFromStringFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	Function *function = declareFunctionInModule(module, makeFromStringFunction);

	if (VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(makeFromStringFunction))
	{
		vector<Value *> functionArgs;
		functionArgs.push_back(arg);
		Value *returnVariable = VuoCompilerCodeGenUtilities::callFunctionWithStructReturn(function, functionArgs, block);

		// Fix return type to match llvmType.
		returnVariable = new BitCastInst(returnVariable, PointerType::get(llvmType, 0), "", block);

		return new LoadInst(returnVariable, "", false, block);
	}
	else
	{
		Value *returnValue = CallInst::Create(function, arg, "", block);
		return VuoCompilerCodeGenUtilities::generateTypeCast(module, block, returnValue, llvmType);
	}
}

/**
 * Generates a call to @c [Type]_getString().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_getString().
 * @return The return value of @c [Type]_getString().
 */
Value * VuoCompilerType::generateStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	return generateFunctionCallWithTypeParameter(module, block, arg, getStringFunction);
}

/**
 * Generates a call to @c [Type]_getInterprocessString().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_getInterprocessString().
 * @return The return value of @c [Type]_getInterprocessString(), or the return value of @c [Type]_getString() if the interprocess function doesn't exist.
 */
Value * VuoCompilerType::generateInterprocessStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	if (getInterprocessStringFunction)
		return generateFunctionCallWithTypeParameter(module, block, arg, getInterprocessStringFunction);
	else
		return generateStringFromValueFunctionCall(module, block, arg);
}

/**
 * Generates a call to @c [Type]_getSummary().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_getSummary().
 * @return The return value of @c [Type]_getSummary().
 */
Value * VuoCompilerType::generateSummaryFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	return generateFunctionCallWithTypeParameter(module, block, arg, getSummaryFunction);
}

/**
 * Generates a call to @c [Type]_retain(), if needed.
 */
void VuoCompilerType::generateRetainCall(Module *module, BasicBlock *block, Value *arg)
{
	if (VuoCompilerCodeGenUtilities::isRetainOrReleaseNeeded(getType()))
		generateFunctionCallWithTypeParameter(module, block, arg, retainFunction);
}

/**
 * Generates a call to @c [Type]_release(), if needed.
 */
void VuoCompilerType::generateReleaseCall(Module *module, BasicBlock *block, Value *arg)
{
	if (VuoCompilerCodeGenUtilities::isRetainOrReleaseNeeded(getType()))
		generateFunctionCallWithTypeParameter(module, block, arg, releaseFunction);
}

/**
 * Generates a call to any of the API functions for the type that takes a value of the type as its only argument.
 */
Value * VuoCompilerType::generateFunctionCallWithTypeParameter(Module *module, BasicBlock *block, Value *arg, Function *sourceFunction)
{
	Function *function = declareFunctionInModule(module, sourceFunction);

	Value *secondArgument = NULL;
	Value **secondArgumentIfNeeded = (function->getFunctionType()->getNumParams() == 2 ? &secondArgument : NULL);
	arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, function, 0, secondArgumentIfNeeded, module, block);

	vector<Value *> args;
	args.push_back(arg);
	if (secondArgument)
		args.push_back(secondArgument);
	return CallInst::Create(function, args, "", block);
}

/**
 * Returns the LLVM type for this Vuo type.
 */
Type * VuoCompilerType::getType(void)
{
	return llvmType;
}

/**
 * Returns the LLVM type for this Vuo type when it appears as a function parameter.
 *
 * This is needed, for example, for struct parameters with the "byval" attribute.
 */
Type * VuoCompilerType::getFunctionParameterType(Type **secondType)
{
	*secondType = (getJsonFunction->getFunctionType()->getNumParams() == 2 ? getJsonFunction->getFunctionType()->getParamType(1) : NULL);
	return getJsonFunction->getFunctionType()->getParamType(0);
}

/**
 * Returns the LLVM attributes for this Vuo type when it appears as a function parameter.
 *
 * This is needed, for example, for struct parameters with the "byval" attribute.
 */
Attributes VuoCompilerType::getFunctionParameterAttributes(void)
{
	return getJsonFunction->getAttributes().getParamAttributes(1);
}

/**
 * Returns true if the type is a list type.
 */
bool VuoCompilerType::isListType(VuoCompilerType *type)
{
	if (! type)
		return false;

	return VuoType::isListTypeName(type->getBase()->getModuleKey());
}

/**
 * Returns true if the type's @c getInterprocessString() function is defined.
 */
bool VuoCompilerType::hasInterprocessStringFunction(void)
{
	return getInterprocessStringFunction != NULL;
}
