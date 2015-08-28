/**
 * @file
 * VuoCompilerType implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop
#include <sstream>

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

	valueFromJsonFunction = NULL;
	jsonFromValueFunction = NULL;
	interprocessJsonFromValueFunction = NULL;
	valueFromStringFunction = NULL;
	stringFromValueFunction = NULL;
	interprocessStringFromValueFunction = NULL;
	summaryFromValueFunction = NULL;
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
	return (module->getNamedValue(typeName + "_valueFromJson") != NULL);
}

/**
 * Overrides the implementation in @c VuoCompilerModule.
 */
void VuoCompilerType::parse(void)
{
	VuoCompilerModule::parse();

	string typeName = getBase()->getModuleKey();
	valueFromJsonFunction = parser->getFunction(typeName + "_valueFromJson");
	jsonFromValueFunction = parser->getFunction(typeName + "_jsonFromValue");
	interprocessJsonFromValueFunction = parser->getFunction(typeName + "_interprocessJsonFromValue");
	summaryFromValueFunction = parser->getFunction(typeName + "_summaryFromValue");

	if (! valueFromJsonFunction)
		VLog("Error: Couldn't find %s_valueFromJson() function.", typeName.c_str());
	if (! jsonFromValueFunction)
		VLog("Error: Couldn't find %s_jsonFromValue() function.", typeName.c_str());
	if (! summaryFromValueFunction)
		VLog("Error: Couldn't find %s_summaryFromValueFunction() function.", typeName.c_str());

	llvmType = VuoCompilerCodeGenUtilities::getParameterTypeBeforeLowering(jsonFromValueFunction, 0, module, typeName);

	parseOrGenerateValueFromStringFunction();
	parseOrGenerateStringFromValueFunction(false);
	if (interprocessJsonFromValueFunction)
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
 * When compiling a type, adds a @c [Type]_valueFromString() function to the type definition.
 *
 * @eg{
 * VuoBoolean VuoBoolean_valueFromString(const char *str)
 * {
 * 	json_object * js = json_tokener_parse(str);
 * 	VuoBoolean value = VuoBoolean_valueFromJson(js);
 * 	json_object_put(js);
 * 	return value;
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that valueFromJsonFunction has been parsed and llvmType has been set.
 */
void VuoCompilerType::parseOrGenerateValueFromStringFunction(void)
{
	string functionName = (getBase()->getModuleKey() + "_valueFromString").c_str();
	Function *function = parser->getFunction(functionName);

	bool isReturnInParam = VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(valueFromJsonFunction);

	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		FunctionType *valueFromJsonFunctionType = valueFromJsonFunction->getFunctionType();

		Type *returnType = valueFromJsonFunctionType->getReturnType();

		vector<Type *> functionParams;
		if (isReturnInParam)
			functionParams.push_back(valueFromJsonFunctionType->getParamType(0));
		functionParams.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		if (isReturnInParam)
			function->addAttribute(1, valueFromJsonFunction->getAttributes().getParamAttributes(1));
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

		vector<Value *> valueFromJsonArgs;
		if (isReturnInParam)
			valueFromJsonArgs.push_back(result);
		valueFromJsonArgs.push_back(jsonTokenerParseReturn);
		Value *valueFromJsonReturn = CallInst::Create(valueFromJsonFunction, valueFromJsonArgs, "", block);

		CallInst::Create(jsonObjectPutFunction, jsonTokenerParseReturn, "", block);

		Value *returnValue = (isReturnInParam ? NULL : valueFromJsonReturn);
		ReturnInst::Create(module->getContext(), returnValue, block);
	}

	valueFromStringFunction = function;
}

/**
 * When compiling a type, adds a @c [Type]_stringFromValue() or @c [Type]_interprocessStringFromValue()
 * function to the type definition.
 *
 * @eg{
 * char * VuoBoolean_stringFromValue(const VuoBoolean value)
 * {
 * 	json_object *js = VuoBoolean_jsonFromValue(value);
 * 	char *string = strdup(json_object_to_json_string_ext(js,JSON_C_TO_STRING_PLAIN));
 * 	json_object_put(js);
 * 	return string;
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that jsonFromValueFunction or interprocessJsonFromValueFunction has been parsed.
 */
void VuoCompilerType::parseOrGenerateStringFromValueFunction(bool isInterprocess)
{
	string functionName = (getBase()->getModuleKey() + (isInterprocess ? "_interprocessStringFromValue" : "_stringFromValue")).c_str();
	Function *function = parser->getFunction(functionName);

	Function *chosenJsonFromValueFunction = (isInterprocess ? interprocessJsonFromValueFunction : jsonFromValueFunction);

	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		FunctionType *jsonFromValueFunctionType = chosenJsonFromValueFunction->getFunctionType();

		Type *returnType = pointerToCharType;

		vector<Type *> functionParams;
		for (int i = 0; i < jsonFromValueFunctionType->getNumParams(); ++i)
			functionParams.push_back(jsonFromValueFunctionType->getParamType(i));

		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		for (int i = 0; i < jsonFromValueFunctionType->getNumParams(); ++i)
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

		vector<Value *> jsonFromValueArgs;
		for (Function::arg_iterator args = function->arg_begin(); args != function->arg_end(); ++args)
		{
			Value *value = args;
			jsonFromValueArgs.push_back(value);
		}
		CallInst *jsonFromValueReturn = CallInst::Create(chosenJsonFromValueFunction, jsonFromValueArgs, "", block);

		vector<Value *> jsonObjectToJsonStringExtArgs;
		jsonObjectToJsonStringExtArgs.push_back(jsonFromValueReturn);
		jsonObjectToJsonStringExtArgs.push_back(ConstantInt::get(module->getContext(), APInt(32, JSON_C_TO_STRING_PLAIN)));
		CallInst *jsonObjectToJsonStringExtReturn = CallInst::Create(jsonObjectToJsonStringExtFunction, jsonObjectToJsonStringExtArgs, "", block);

		CallInst *strdupReturn = CallInst::Create(strdupFunction, jsonObjectToJsonStringExtReturn, "", block);

		CallInst::Create(jsonObjectPutFunction, jsonFromValueReturn, "", block);

		ReturnInst::Create(module->getContext(), strdupReturn, block);
	}

	if (isInterprocess)
		interprocessStringFromValueFunction = function;
	else
		stringFromValueFunction = function;
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
 * Generates a call to @c [Type]_valueFromString().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_valueFromString().
 * @return The return value of @c [Type]_valueFromString().
 */
Value * VuoCompilerType::generateValueFromStringFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	Function *function = declareFunctionInModule(module, valueFromStringFunction);

	if (VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(valueFromStringFunction))
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
 * Generates a call to @c [Type]_stringFromValue().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_stringFromValue().
 * @return The return value of @c [Type]_stringFromValue().
 */
Value * VuoCompilerType::generateStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	return generateSerializationFunctionCall(module, block, arg, stringFromValueFunction);
}

/**
 * Generates a call to @c [Type]_interprocessStringFromValue().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_interprocessStringFromValue().
 * @return The return value of @c [Type]_interprocessStringFromValue(), or the return value of @c [Type]_stringFromValue() if the interprocess function doesn't exist.
 */
Value * VuoCompilerType::generateInterprocessStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	if (interprocessStringFromValueFunction)
		return generateSerializationFunctionCall(module, block, arg, interprocessStringFromValueFunction);
	else
		return generateStringFromValueFunctionCall(module, block, arg);
}

/**
 * Generates a call to @c [Type]_summaryFromValue().
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param arg The argument to pass to @c [Type]_summaryFromValue().
 * @return The return value of @c [Type]_summaryFromValue().
 */
Value * VuoCompilerType::generateSummaryFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg)
{
	return generateSerializationFunctionCall(module, block, arg, summaryFromValueFunction);
}

/**
 * Generates a call to @c [Type]_stringFromValue() or @c [Type]_summaryFromValue().
 */
Value * VuoCompilerType::generateSerializationFunctionCall(Module *module, BasicBlock *block, Value *arg, Function *sourceFunction)
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
	*secondType = (jsonFromValueFunction->getFunctionType()->getNumParams() == 2 ? jsonFromValueFunction->getFunctionType()->getParamType(1) : NULL);
	return jsonFromValueFunction->getFunctionType()->getParamType(0);
}

/**
 * Returns the LLVM attributes for this Vuo type when it appears as a function parameter.
 *
 * This is needed, for example, for struct parameters with the "byval" attribute.
 */
Attributes VuoCompilerType::getFunctionParameterAttributes(void)
{
	return jsonFromValueFunction->getAttributes().getParamAttributes(1);
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
