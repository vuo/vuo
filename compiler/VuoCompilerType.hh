/**
 * @file
 * VuoCompilerType interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoCompilerModule.hh"

class VuoType;

/**
 * The compiler detail class for @c VuoType.
 */
class VuoCompilerType : public VuoBaseDetail<VuoType>, public VuoCompilerModule  // Order matters: VuoCompilerModule after VuoBaseDetail - https://stackoverflow.com/a/2254306/238387
{
private:
	Function *makeFromJsonFunction;
	Function *getJsonFunction;
	Function *getInterprocessJsonFunction;
	Function *getStringFunction;
	Function *getInterprocessStringFunction;
	Function *getSummaryFunction;
	Function *areEqualFunction;
	Function *isLessThanFunction;
	Function *retainFunction;
	Function *releaseFunction;

	Type *llvmArgumentType;  ///< This VuoType's corresponding LLVM type when the C ABI lowers it to be passed as a function argument.
	Type *llvmSecondArgumentType;  ///< This VuoType's corresponding LLVM type when the C ABI lowers it to be passed as a function argument (second argument, if needed).
	Type *llvmReturnType;  ///< This VuoType's corresponding LLVM type when the C ABI lowers it to a return value.  Depending on @ref VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter, this may need to be used as a function argument rather than return value.
	bool isReturnPassedAsArgument;

	static bool isType(string typeName, Module *module);
	void parse(void);
	set<string> globalsToRename(void);
	void parseOrGenerateStringFromValueFunction(bool isInterprocess);
	void parseOrGenerateRetainOrReleaseFunction(bool isRetain);
	Value * generateFunctionCallWithTypeParameter(Module *module, BasicBlock *block, Value *arg, Function *sourceFunction);

	friend class TestVuoCompilerType;
	friend class TestTypes;

protected:
	VuoCompilerType(string typeName, Module *module);

public:
	static VuoCompilerType * newType(string typeName, Module *module);
	static bool isListType(VuoCompilerType *type);

	Value * generateRetainedValueFromString(Module *module, BasicBlock *block, Value *stringValue);
	Value * generateStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	Value * generateInterprocessStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	Value * generateSummaryFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	void generateRetainCall(Module *module, BasicBlock *block, Value *arg);
	void generateReleaseCall(Module *module, BasicBlock *block, Value *arg);
	vector<Value *> convertPortDataToArgs(Module *module, BasicBlock *block, Value *arg, FunctionType *functionType, int parameterIndex, bool isUnloweredStructPointerParameter);
	Value * convertArgsToPortData(Module *module, BasicBlock *block, Function *function, int parameterIndex);
	size_t getAllocationSize(Module *module);
	size_t getStorageSize(Module *module);
	Value * convertToPortData(BasicBlock *block, Value *voidPointer);
	vector<Type *> getFunctionParameterTypes(void);
	PointerType * getFunctionParameterPointerType(void);
	AttributeList getFunctionAttributes(void);
	void copyFunctionParameterAttributes(Function *dstFunction);
	void copyFunctionParameterAttributes(Module *module, CallInst *dstCall);
	bool hasInterprocessStringFunction(void);
	bool supportsComparison(void);
};
