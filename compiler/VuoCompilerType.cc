/**
 * @file
 * VuoCompilerType implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>
#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerType.hh"
#include "VuoType.hh"

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
	getStringFunction = NULL;
	getInterprocessStringFunction = NULL;
	getSummaryFunction = NULL;
	areEqualFunction = NULL;
	isLessThanFunction = NULL;
	retainFunction = NULL;
	releaseFunction = NULL;
	llvmArgumentType              = nullptr;
	llvmSecondArgumentType        = nullptr;
	llvmReturnType                = nullptr;
	isReturnPassedAsArgument      = false;

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
	areEqualFunction = parser->getFunction(typeName + "_areEqual");
	isLessThanFunction = parser->getFunction(typeName + "_isLessThan");

	if (! makeFromJsonFunction)
		VUserLog("Error: Couldn't find %s_makeFromJson() function.", typeName.c_str());
	if (! getJsonFunction)
		VUserLog("Error: Couldn't find %s_getJson() function.", typeName.c_str());
	if (! getSummaryFunction)
		VUserLog("Error: Couldn't find %s_getSummary() function.", typeName.c_str());

	llvmArgumentType = getJsonFunction->getFunctionType()->getParamType(0);
	if (getJsonFunction->getFunctionType()->getNumParams() == 2)
		llvmSecondArgumentType = getJsonFunction->getFunctionType()->getParamType(1);
	else if (getJsonFunction->getFunctionType()->getNumParams() != 1)
	{
		string s;
		raw_string_ostream oss(s);
		oss << "Expected a function with 1 or 2 parameters, got " << getJsonFunction->getFunctionType()->getNumParams() << " for function `";
		getJsonFunction->getFunctionType()->print(oss);
		oss << "`.";
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling composition", "", "Unsupported port data type", oss.str());
		throw VuoCompilerException(issue);
	}

	llvmReturnType = makeFromJsonFunction->getReturnType();
	if (llvmReturnType->isVoidTy())
	{
		isReturnPassedAsArgument = true;
		llvmReturnType = makeFromJsonFunction->arg_begin()->getType();
	}

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

		copyFunctionParameterAttributes(function);
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
 * When compiling a type, generates a `[Type]_retain()` or `[Type]_release()` function,
 * unless the type module already includes implementations of those functions.
 *
 * @eg{
 * void VuoNdiSource_retain(VuoNdiSource value)
 * {
 *   VuoRetain((void *)value.name);
 *   VuoRetain((void *)value.ipAddress);
 * }
 * }
 *
 * When parsing a compiled type, parses this function from the type definition.
 *
 * Assumes that `llvmArgumentType` has been set.
 */
void VuoCompilerType::parseOrGenerateRetainOrReleaseFunction(bool isRetain)
{
	string functionName{getBase()->getModuleKey() + (isRetain ? "_retain" : "_release")};
	Function *function = parser->getFunction(functionName);

	if (! function)
	{
		vector<Type *> functionParams;
		functionParams.push_back(llvmArgumentType);
		if (llvmSecondArgumentType)
			functionParams.push_back(llvmSecondArgumentType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		copyFunctionParameterAttributes(function);
	}

	if (function->isDeclaration())
	{
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function);

		Function::arg_iterator args = function->arg_begin();
		Value *arg = args++;

		if (!llvmSecondArgumentType)
		{
			if (isReturnPassedAsArgument && VuoCompilerCodeGenUtilities::isPointerToStruct(arg->getType()))
			{
				// The data type is a struct that is passed by reference (LLVM's `byval` attribute on x86_64).
				arg = new LoadInst(arg, "unloweredStruct", false, block);
			}

			VuoCompilerCodeGenUtilities::generateRetainOrReleaseCall(module, block, arg, isRetain);
		}
		else
		{
			if (llvmArgumentType->isPointerTy())
			{
				// The data type is a struct with pointer field(s), lowered to 2 arguments, the 1st of which is a pointer.
				VuoCompilerCodeGenUtilities::generateRetainOrReleaseCall(module, block, arg, isRetain);
			}

			if (llvmSecondArgumentType->isPointerTy())
			{
				// The data type is a struct with pointer field(s), lowered to 2 arguments, the 2nd of which is a pointer.
				Value *secondArg = args++;
				VuoCompilerCodeGenUtilities::generateRetainOrReleaseCall(module, block, secondArg, isRetain);
			}
		}

		ReturnInst::Create(module->getContext(), block);
	}

	if (isRetain)
		retainFunction = function;
	else
		releaseFunction = function;
}

/**
 * Generates code that unserializes data from a string.
 *
 * @eg{
 * json_object *js = json_tokener_parse(str);
 * VuoImage value = VuoImage_makeFromJson(js);
 * VuoRetain(value);
 * json_object_put(js);
 * }
 *
 * @param module The destination LLVM module (i.e., generated code).
 * @param block The LLVM block to which to append the function call.
 * @param stringValue The serialized data, which will be passed to `json_tokener_parse()`.
 * @return The unserialized port data (the same data type as `portContext->data`).
 */
Value * VuoCompilerType::generateRetainedValueFromString(Module *module, BasicBlock *block, Value *stringValue)
{
	Function *makeFromJsonFunctionInModule = declareFunctionInModule(module, makeFromJsonFunction);

	Function *jsonTokenerParseFunction = VuoCompilerCodeGenUtilities::getJsonTokenerParseFunction(module);
	Value *jsonValue = CallInst::Create(jsonTokenerParseFunction, stringValue, "valueAsJson", block);

	vector<Value *> makeFromJsonArgs;
	int jsonParamIndex = makeFromJsonFunctionInModule->getFunctionType()->getNumParams() - 1;
	Type *jsonParamType = static_cast<PointerType *>(makeFromJsonFunctionInModule->getFunctionType()->getParamType(jsonParamIndex));
	Value *jsonArg = new BitCastInst(jsonValue, jsonParamType, "", block);
	makeFromJsonArgs.push_back(jsonArg);

	Value *dataPointer = nullptr;
	if (VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(makeFromJsonFunctionInModule))
	{
		Value *makeFromJsonReturn = VuoCompilerCodeGenUtilities::callFunctionWithStructReturn(makeFromJsonFunctionInModule, makeFromJsonArgs, block);
		dataPointer = new BitCastInst(makeFromJsonReturn, llvmReturnType, "valueFromString", block);
	}
	else
	{
		Value *makeFromJsonReturn = CallInst::Create(makeFromJsonFunctionInModule, makeFromJsonArgs, "valueFromString", block);
		dataPointer = VuoCompilerCodeGenUtilities::generatePointerToValue(block, makeFromJsonReturn);
	}

	generateRetainCall(module, block, dataPointer);

	Function *jsonObjectPutFunction = VuoCompilerCodeGenUtilities::getJsonObjectPutFunction(module);
	CallInst::Create(jsonObjectPutFunction, jsonValue, "", block);

	return dataPointer;
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
	generateFunctionCallWithTypeParameter(module, block, arg, retainFunction);
}

/**
 * Generates a call to @c [Type]_release(), if needed.
 */
void VuoCompilerType::generateReleaseCall(Module *module, BasicBlock *block, Value *arg)
{
	generateFunctionCallWithTypeParameter(module, block, arg, releaseFunction);
}

/**
 * Generates an argument or arguments representing the port data, lowered for the C ABI.
 *
 * `arg` should be the same data type as `portContext->data`.
 */
vector<Value *> VuoCompilerType::convertPortDataToArgs(Module *module, BasicBlock *block, Value *arg, FunctionType *functionType, int parameterIndex,
													   bool isUnloweredStructPointerParameter)
{
	vector<Value *> args;
	if (!llvmSecondArgumentType)
	{
		Type *parameterType = functionType->getParamType(parameterIndex);

		if (!isReturnPassedAsArgument)
		{
			bool isLoweredAsUsual = true;
			if (parameterType->isPointerTy() && !llvmArgumentType->isPointerTy())
				isLoweredAsUsual = false;

			if (isLoweredAsUsual)
			{
				// The data type is either not lowered or lowered to 1 parameter.
				arg = new BitCastInst(arg, PointerType::getUnqual(parameterType), "dataPointerCasted", block);
				arg = new LoadInst(arg, "data", false, block);
			}
			else
			{
				// The data type is normally lowered to 1 parameter, but here is passed by reference ('byval').
				arg = new BitCastInst(arg, parameterType, "dataCasted", block);
			}
		}
		else
		{
			// The data type is a struct that is passed by reference (LLVM's `byval` attribute on x86_64).
			arg = new BitCastInst(arg, parameterType, "dataCasted", block);
		}

		args.push_back(arg);
	}
	else
	{
		if (! isUnloweredStructPointerParameter)
		{
			// The data type is lowered to 2 parameters.
			arg = new BitCastInst(arg, PointerType::getUnqual(llvmReturnType), "dataStructCasted", block);
			for (int i = 0; i < 2; ++i)
			{
				Value *currArg = VuoCompilerCodeGenUtilities::generateGetStructPointerElement(module, block, arg, i);
				if (currArg->getType()->isPointerTy())
				{
					Type *parameterType = functionType->getParamType(parameterIndex + i);
					currArg = new BitCastInst(currArg, parameterType, "dataCasted", block);
				}
				args.push_back(currArg);
			}
		}
		else
		{
			// The data type is normally lowered to 2 parameters, but here is a struct passed by reference (`byval`).
			Type *parameterType = functionType->getParamType(parameterIndex);
			arg = new BitCastInst(arg, parameterType, "dataCasted", block);
			args.push_back(arg);
		}
	}

	return args;
}

/**
 * Unlowers the function argument at @a parameterIndex (and the one after, if this data type is lowered to 2 arguments)
 * into port data (the same data type as `portContext->data`).
 */
Value * VuoCompilerType::convertArgsToPortData(Module *module, BasicBlock *block, Function *function, int parameterIndex)
{
	vector<Value *> args;
	args.push_back( VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, parameterIndex) );
	if (llvmSecondArgumentType)
		args.push_back( VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, parameterIndex+1) );

	if (!llvmSecondArgumentType)
	{
		if (!isReturnPassedAsArgument)
		{
			Value *argPointer = VuoCompilerCodeGenUtilities::generatePointerToValue(block, args[0]);
			return new BitCastInst(argPointer, PointerType::getUnqual(llvmReturnType), "dataPointerCasted", block);
		}
		else
			return args[0];
	}
	else
	{
		size_t totalArgByteCount = 0;
		for (auto arg : args)
			totalArgByteCount += module->getDataLayout().getTypeStoreSize(arg->getType());

		Value *dataPointer = new AllocaInst(llvmReturnType, 0, "dataPointer", block);
		Value *dataPointerAsBytePointer = new BitCastInst(dataPointer, PointerType::getUnqual(IntegerType::get(module->getContext(), 8)), "dataBytePointer", block);

		size_t argByteOffset = 0;
		for (auto arg : args)
		{
			Value *argPointer        = VuoCompilerCodeGenUtilities::generatePointerToValue(block, arg);
			Value *offsetValue       = ConstantInt::get(module->getContext(), APInt(64, argByteOffset));
			Value *offsetDataPointer = GetElementPtrInst::Create(nullptr, dataPointerAsBytePointer, offsetValue, "", block);
			size_t storeByteCount    = module->getDataLayout().getTypeStoreSize(arg->getType());
			size_t allocByteCount    = module->getDataLayout().getTypeAllocSize(arg->getType());
			VuoCompilerCodeGenUtilities::generateMemoryCopy(module, block, argPointer, offsetDataPointer, storeByteCount);
			argByteOffset += allocByteCount;
		}

		return dataPointer;
	}
}

/**
 * Generates a call to any of the API functions for the type that takes a value of the type as its only argument.
 *
 * `arg` should be `portContext->data`.
 *
 * @throw VuoCompilerException `arg` couldn't be converted to `sourceFunction`'s parameter type.
 */
Value * VuoCompilerType::generateFunctionCallWithTypeParameter(Module *module, BasicBlock *block, Value *arg, Function *sourceFunction)
{
	Function *function = declareFunctionInModule(module, sourceFunction);
	vector<Value *> args = convertPortDataToArgs(module, block, arg, function->getFunctionType(), 0, false);
	return CallInst::Create(function, args, "", block);
}

/**
 * Returns the offset in bytes between successive objects of the type, including alignment padding.
 */
size_t VuoCompilerType::getAllocationSize(Module *module)
{
	Type *type = llvmReturnType;
	if (isReturnPassedAsArgument)
		type = static_cast<PointerType *>(llvmReturnType)->getElementType();

	return module->getDataLayout().getTypeAllocSize(type);
}

/**
 * Returns the maximum number of bytes that may be overwritten by storing the type.
 */
size_t VuoCompilerType::getStorageSize(Module *module)
{
	Type *type = llvmReturnType;
	if (isReturnPassedAsArgument)
		type = static_cast<PointerType *>(llvmReturnType)->getElementType();

	return module->getDataLayout().getTypeStoreSize(type);
}

/**
 * Casts a `void *` to a pointer to this type's storage.
 */
Value * VuoCompilerType::convertToPortData(BasicBlock *block, Value *voidPointer)
{
	Type *type = PointerType::getUnqual(llvmReturnType);
	if (isReturnPassedAsArgument)
		type = llvmReturnType;

	return new BitCastInst(voidPointer, type, "", block);
}

/**
 * Returns the LLVM type(s) for when this Vuo type is passed to a function.
 */
vector<Type *> VuoCompilerType::getFunctionParameterTypes(void)
{
	vector<Type *> types;
	types.push_back(llvmArgumentType);
	if (llvmSecondArgumentType)
		types.push_back(llvmSecondArgumentType);
	return types;
}

/**
 * Returns the LLVM type for when a pointer to this Vuo type is passed to a function.
 */
PointerType * VuoCompilerType::getFunctionParameterPointerType(void)
{
	return PointerType::getUnqual(llvmReturnType);
}

/**
 * Returns the LLVM attributes for this Vuo type when it appears as a function parameter.
 *
 * This is needed, for example, for struct parameters with the "byval" attribute.
 */
AttributeList VuoCompilerType::getFunctionAttributes(void)
{
	return getJsonFunction->getAttributes();
}

/**
 * Copies the LLVM attributes for this Vuo type as a function parameter onto the parameter(s) of @a dstFunction.
 */
void VuoCompilerType::copyFunctionParameterAttributes(Function *dstFunction)
{
	VuoCompilerCodeGenUtilities::copyParameterAttributes(getJsonFunction, dstFunction);
}

/**
 * Copies the LLVM attributes for this Vuo type as a function parameter onto the parameter(s) of @a dstCall.
 */
void VuoCompilerType::copyFunctionParameterAttributes(Module *module, CallInst *dstCall)
{
	VuoCompilerCodeGenUtilities::copyParameterAttributes(module, getJsonFunction, dstCall);
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

/**
 * Returns true if the type's `areEqual` and `isLessThan` functions are defined.
 */
bool VuoCompilerType::supportsComparison(void)
{
	return areEqualFunction != NULL && isLessThanFunction != NULL;
}
