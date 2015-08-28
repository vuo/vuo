/**
 * @file
 * VuoCompilerCodeGenUtilities interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCODEGENUTILITIES_H
#define VUOCOMPILERCODEGENUTILITIES_H

#include "dispatch/dispatch.h"

/**
 * Handy methods for generating code.
 */
class VuoCompilerCodeGenUtilities
{
private:
	static PointerType * getDispatchSemaphoreType(Module *module);
	static PointerType * getDispatchGroupType(Module *module);
	static PointerType * getDispatchQueueType(Module *module);
	static StructType * getDispatchObjectElementType(Module *module);
	static void generateSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable, Function *workerFunction, Value *contextValue, bool isSynchronous);
	static StructType * getGraphvizGraphType(Module *module);
	static StructType * getJsonObjectType(Module *module);
	static PointerType * getPointerToFileType(Module *module);

	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, Value *timeoutValue);

	static Value * generateConversionToDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);

	static Value * generateTypeCastFromIntegerToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromFloatingPointToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToInteger(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToFloatingPoint(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromLoweredTypeToStruct(BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static void generateRetainOrReleaseCall(Module *module, BasicBlock *block, Value *argument, bool isRetain);

	static Function * getVuoRetainFunction(Module *module);
	static Function * getVuoReleaseFunction(Module *module);

	static Value * generateStderr(Module *module, BasicBlock *block);

	static bool isParameterPassedByValue(Function *function, int parameterIndex);

public:
	static StructType * getDispatchObjectType(Module *module);

	static GlobalVariable * generateAllocationForSemaphore(Module *module, string identifier);
	static void generateInitializationForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, int initialValue=1);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, int64_t timeoutInNanoseconds);
	static void generateSignalForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable);

	static AllocaInst * generateAllocationForDispatchGroup(Module *module, BasicBlock *block, string identifier);
	static void generateInitializationForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable);
	static void generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable, Function *workerFunction, Value *contextValue);
	static void generateWaitForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable, dispatch_time_t timeout=DISPATCH_TIME_FOREVER);

	static GlobalVariable * generateAllocationForDispatchQueue(Module *module, string identifier);
	static void generateInitializationForDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable, string dispatchQueueName);
	static void generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable, Function *workerFunction, Value *contextValue);
	static void generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable, Function *workerFunction, Value *contextValue);

	static void generateRetainForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);
	static void generateFinalizationForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);

	static Value * generatePointerToValue(BasicBlock *block, Value *value);
	static Constant * generatePointerToConstantString(Module *module, string stringValue, string globalVariableName = "");
	static Constant * generatePointerToConstantArrayOfStrings(Module *module, vector<string> stringValues, string globalVariableName = "");
	static void generateStringMatchingCode(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue, map<string, pair<BasicBlock *, BasicBlock *> > blocksForString);
	static Value * generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues);
	static Value * generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, Value *elementCountValue);
	static Value * generateTypeCast(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static void generateAnnotation(Module *module, BasicBlock *block, Value *value, string annotation, string fileName, unsigned int lineNumber);
	static void generateRetainCall(Module *module, BasicBlock *block, Value *argument);
	static void generateReleaseCall(Module *module, BasicBlock *block, Value *argument);
	static Value * generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize);
	static void generateUnserialization(Module *module, BasicBlock *block, Value *stringToUnserialize, GlobalVariable *destination);
	static ICmpInst * generateIsPausedComparison(Module *module, BasicBlock *block);
	static void generatePrint(Module *module, BasicBlock *block, string formatString, Value *value=NULL);

	static Function * getStrcatFunction(Module *module);
	static Function * getStrcmpFunction(Module *module);
	static Function * getStrdupFunction(Module *module);
	static Function * getStrlenFunction(Module *module);
	static Function * getSnprintfFunction(Module *module);
	static Function * getSscanfFunction(Module *module);
	static Function * getFprintfFunction(Module *module);
	static Function * getPutsFunction(Module *module);
	static Function * getMallocFunction(Module *module);
	static Function * getFreeFunction(Module *module);
	static Function * getAnnotateFunction(Module *module);
	static Function * getJsonObjectPutFunction(Module *module);
	static Function * getJsonObjectToJsonStringExtFunction(Module *module);
	static Function * getJsonTokenerParseFunction(Module *module);
	static Function * getCallbackStartFunction(Module *module);
	static Function * getCallbackStopFunction(Module *module);
	static Function * getGetInputPortValueFunction(Module *module);
	static Function * getGetInputPortValueThreadUnsafeFunction(Module *module);
	static Function * getGetOutputPortValueFunction(Module *module);
	static Function * getGetOutputPortValueThreadUnsafeFunction(Module *module);
	static Function * getGetInputPortSummaryFunction(Module *module);
	static Function * getGetOutputPortSummaryFunction(Module *module);
	static Function * getSetInputPortValueFunction(Module *module);
	static Function * getGetPublishedInputPortValueFunction(Module *module);
	static Function * getGetPublishedOutputPortValueFunction(Module *module);
	static Function * getSetPublishedInputPortValueFunction(Module *module);
	static Function * getSendNodeExecutionStartedFunction(Module *module);
	static Function * getSendNodeExecutionFinishedFunction(Module *module);
	static Function * getSendInputPortsUpdatedFunction(Module *module);
	static Function * getSendOutputPortsUpdatedFunction(Module *module);
	static Function * getTranscodeToGraphvizIdentifierFunction(Module *module);
	static Function * getSerializeFunction(Module *module);
	static Function * getUnserializeFunction(Module *module);
	static Function * getOpenGraphvizGraphFunction(Module *module);
	static Function * getCloseGraphvizGraphFunction(Module *module);
	static Function * getGetConstantValueFromGraphvizFunction(Module *module);
	static Function * getIsNodeInBothCompositionsFunction(Module *module);

	static GlobalVariable * getIsPausedVariable(Module *module);

	static Type * getParameterTypeBeforeLowering(Function *function, int parameterIndex, Module *module, string typeName);
	static Value * convertArgumentToParameterType(Value *argument, Function *function, int parameterIndex, Value **secondArgument, Module *module, BasicBlock *block);
	static Value * callFunctionWithStructReturn(Function *function, vector<Value *> args, BasicBlock *block);
	static bool isFunctionReturningStructViaParameter(Function *function);
};

#endif
