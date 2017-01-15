/**
 * @file
 * VuoCompilerCodeGenUtilities interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCODEGENUTILITIES_H
#define VUOCOMPILERCODEGENUTILITIES_H

class VuoCompilerConstantStringCache;
class VuoCompilerTriggerPort;
class VuoCompilerType;
class VuoPort;
class VuoType;

#include "VuoPortClass.hh"

/**
 * Handy methods for generating code.
 */
class VuoCompilerCodeGenUtilities
{
private:
	static PointerType * getDispatchQueueType(Module *module);
	static StructType * getDispatchObjectElementType(Module *module);
	static StructType * getGraphvizGraphType(Module *module);
	static StructType * getJsonObjectType(Module *module);
	static PointerType * getPointerToFileType(Module *module);
	static PointerType * getInstanceDataType(Module *module);

	static void generateSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue, Function *workerFunction, Value *contextValue, bool isSynchronous);

	static Value * generateConversionToDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);

	static Value * generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex);
	static void generateSetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex, Value *value);
	static Value * generateGetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex);
	static Value * generateGetStructPointerElementVariable(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex);

	static Value * generateTypeCastFromIntegerToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromFloatingPointToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToInteger(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToFloatingPoint(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromLoweredTypeToStruct(BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static void generateRetainOrReleaseCall(Module *module, BasicBlock *block, Value *argument, bool isRetain);

	static Function * getNodeFunction(Module *module, string moduleKey, string functionName, bool hasCompositionIdentifierArg, bool hasInstanceDataArg, bool hasInstanceDataReturn, bool hasEventOnlyInputArgs,
									  const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
									  const map<VuoPort *, json_object *> &detailsForPorts, const map<VuoPort *, string> &displayNamesForPorts,
									  const map<VuoPort *, string> &defaultValuesForInputPorts, const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
									  map<VuoPort *, size_t> &indexOfParameter, map<VuoPort *, size_t> &indexOfEventParameter, VuoCompilerConstantStringCache &constantStrings);

	static Function * getVuoRegisterFunction(Module *module);
	static Function * getVuoRetainFunction(Module *module);
	static Function * getVuoReleaseFunction(Module *module);
	static Function * getIsNodeInBothCompositionsFunction(Module *module);

	static Value * generateStderr(Module *module, BasicBlock *block);

	static bool isParameterPassedByValue(Function *function, int parameterIndex);

public:
	static PointerType * getDispatchSemaphoreType(Module *module);
	static PointerType * getDispatchGroupType(Module *module);
	static StructType * getDispatchObjectType(Module *module);
	static StructType * getNodeContextType(Module *module);
	static StructType * getPortContextType(Module *module);

	static Value * generateCreateDispatchSemaphore(Module *module, BasicBlock *block, int initialValue=1);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue, Value *timeoutValue);
	static Value * generateWaitForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable, Value *timeoutValue);
	static void generateSignalForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue);
	static void generateSignalForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable);

	static Value * generateCreateDispatchGroup(Module *module, BasicBlock *block);
	static void generateEnterDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue);
	static void generateLeaveDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue);
	static void generateWaitForDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue, dispatch_time_t timeout=DISPATCH_TIME_FOREVER);

	static Value * generateGetGlobalDispatchQueue(Module *module, BasicBlock *block);
	static Value * generateCreateDispatchQueue(Module *module, BasicBlock *block, string dispatchQueueName);
	static void generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue, Function *workerFunction, Value *contextValue);
	static void generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue, Function *workerFunction, Value *contextValue);

	static void generateRetainForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);
	static void generateFinalizationForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);

	static Value * generateCreateDispatchTime(Module *module, BasicBlock *block, Value *deltaValue);

	static Value * generateCreatePortContext(Module *module, BasicBlock *block, Type *dataType, bool isTrigger, std::string triggerQueueName);
	static void generateSetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue, Value *eventValue);
	static void generateSetPortContextData(Module *module, BasicBlock *block, Value *portContextValue, Value *dataValue);
	static void generateSetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, Value *triggerFunctionValue);
	static Value * generateGetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextData(Module *module, BasicBlock *block, Value *portContextValue, Type *dataType);
	static Value * generateGetPortContextDataVariable(Module *module, BasicBlock *block, Value *portContextValue, Type *dataType);
	static Value * generateGetPortContextDataVariableAsVoidPointer(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerQueue(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerSemaphore(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, FunctionType *functionType);

	static Value * generateCreateNodeContext(Module *module, BasicBlock *block, bool hasInstanceData, bool isComposition, size_t outputEventCount);
	static void generateSetNodeContextPortContexts(Module *module, BasicBlock *block, Value *nodeContextValue, vector<Value *> portContextValues);
	static void generateSetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *instanceDataValue);
	static void generateSetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue, Value *claimingEventIdValue);
	static void generateSetNodeContextExecutingEventId(Module *module, BasicBlock *block, Value *nodeContextValue, Value *executingEventIdValue);
	static void generateSetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index, Value *eventValue);
	static Value * generateGetNodeContextPortContext(Module *module, BasicBlock *block, Value *nodeContextValue, int index);
	static Value * generateGetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType);
	static Value * generateGetNodeContextInstanceDataVariable(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType);
	static Value * generateGetNodeContextSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextExecutingGroup(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextExecutingEventId(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index);
	static void generateResetNodeContextEvents(Module *module, BasicBlock *block, Value *nodeContextValue, size_t portCount);
	static void generateFreeNodeContext(Module *module, BasicBlock *block, Value *nodeContextValue, size_t portCount);

	static Value * generateGetDataForPort(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *portIdentifierValue);
	static Value * generateGetNodeSemaphoreForPort(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *portIdentifierValue);
	static Value * generateGetNodeIndexForPort(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *portIdentifierValue);
	static Value * generateGetTypeIndexForPort(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *portIdentifierValue);
	static void generateAddPortIdentifier(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *portIdentifierValue, Value *portDataVariable, Value *nodeSemaphoreValue, Value *nodeIndexValue, Value *typeIndexValue);

	static void generateSetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex, Value *value);
	static Value * generateGetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex);
	static Value * generatePointerToValue(BasicBlock *block, Value *value);
	static Constant * generatePointerToConstantString(Module *module, string stringValue, string globalVariableName = "");
	static Constant * generatePointerToConstantArrayOfStrings(Module *module, vector<string> stringValues, string globalVariableName = "");
	static void generateStringMatchingCode(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue, map<string, pair<BasicBlock *, BasicBlock *> > blocksForString, VuoCompilerConstantStringCache &constantStrings);
	static void generateIndexMatchingCode(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputIndexValue, vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex);
	static Value * generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues, VuoCompilerConstantStringCache &constantStrings);
	static Value * generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate, VuoCompilerConstantStringCache &constantStrings);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, int elementCount);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, Value *elementCountValue);
	static Value * generateTypeCast(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static void generateAnnotation(Module *module, BasicBlock *block, Value *value, string annotation, string fileName, unsigned int lineNumber, VuoCompilerConstantStringCache &constantStrings);
	static void generateModuleMetadata(Module *module, string metadata, string moduleKey);
	static void generateRegisterCall(Module *module, BasicBlock *block, Value *argument, Function *freeFunction);
	static void generateRetainCall(Module *module, BasicBlock *block, Value *argument);
	static void generateReleaseCall(Module *module, BasicBlock *block, Value *argument);
	static bool isRetainOrReleaseNeeded(Type *type);
	static void generateFreeCall(Module *module, BasicBlock *block, Value *argument);
	static Value * generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize, VuoCompilerConstantStringCache &constantStrings);
	static void generateUnserialization(Module *module, BasicBlock *block, Value *stringToUnserialize, Value *destinationVariable, VuoCompilerConstantStringCache &constantStrings);
	static ICmpInst * generateIsPausedComparison(Module *module, BasicBlock *block);
	static ICmpInst * generateShouldSendDataTelemetryComparison(Module *module, BasicBlock *block, string portIdentifier, VuoCompilerConstantStringCache &constantStrings);
	static void generateIsNodeInBothCompositionsCheck(Module *module, Function *function, string nodeIdentifier, BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock, VuoCompilerConstantStringCache &constantStrings);
	static ConstantInt * generateNoEventIdConstant(Module *module);
	static void generateAddNodeContext(Module *module, BasicBlock *block, Value *nodeIdentifierValue, Value *nodeContextValue);
	static Value * generateGetNodeContext(Module *module, BasicBlock *block, Value *nodeIdentifierValue);
	static void generatePrint(Module *module, BasicBlock *block, string formatString, Value *value=NULL);
	static void generatePrint(Module *module, BasicBlock *block, string formatString, const vector<Value *> &values);

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
	static Function * getCompositionContextInitFunction(Module *module, string moduleKey);
	static Function * getCompositionContextFiniFunction(Module *module, string moduleKey);
	static Function * getSetupFunction(Module *module);
	static Function * getCleanupFunction(Module *module);
	static Function * getInstanceInitFunction(Module *module);
	static Function * getInstanceFiniFunction(Module *module);
	static Function * getInstanceTriggerStartFunction(Module *module);
	static Function * getInstanceTriggerStopFunction(Module *module);
	static Function * getNodeInstanceInitFunction(Module *module, string moduleKey, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantStringCache &constantStrings);
	static Function * getNodeInstanceFiniFunction(Module *module, string moduleKey, VuoCompilerConstantStringCache &constantStrings);
	static Function * getNodeInstanceTriggerStartFunction(Module *module, string moduleKey, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantStringCache &constantStrings);
	static Function * getNodeInstanceTriggerStopFunction(Module *module, string moduleKey, VuoCompilerConstantStringCache &constantStrings);
	static Function * getNodeInstanceTriggerUpdateFunction(Module *module, string moduleKey, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantStringCache &constantStrings);
	static Function * getNodeEventFunction(Module *module, string moduleKey, bool isSubcomposition, bool isStateful, const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
										   const map<VuoPort *, json_object *> &detailsForPorts, const map<VuoPort *, string> &displayNamesForPorts,
										   const map<VuoPort *, string> &defaultValuesForInputPorts, const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
										   map<VuoPort *, size_t> &indexOfParameter, map<VuoPort *, size_t> &indexOfEventParameter, VuoCompilerConstantStringCache &constantStrings);
	static Function * getWaitForNodeFunction(Module *module, string moduleKey);
	static Function * getCompositionGetPortValueFunction(Module *module);
	static Function * getGetPortValueFunction(Module *module);
	static Function * getGetInputPortStringFunction(Module *module);
	static Function * getGetOutputPortStringFunction(Module *module);
	static Function * getSetInputPortValueFunction(Module *module);
	static Function * getCompositionSetPortValueFunction(Module *module);
	static Function * getGetPublishedInputPortValueFunction(Module *module);
	static Function * getGetPublishedOutputPortValueFunction(Module *module);
	static Function * getSetPublishedInputPortValueFunction(Module *module);
	static Function * getSendNodeExecutionStartedFunction(Module *module);
	static Function * getSendNodeExecutionFinishedFunction(Module *module);
	static Function * getSendInputPortsUpdatedFunction(Module *module);
	static Function * getSendOutputPortsUpdatedFunction(Module *module);
	static Function * getSendPublishedOutputPortsUpdatedFunction(Module *module);
	static Function * getSendEventDroppedFunction(Module *module);
	static Function * getGetNextEventIdFunction(Module *module);
	static Function * getTranscodeToGraphvizIdentifierFunction(Module *module);
	static Function * getCompositionSerializeFunction(Module *module, string moduleKey);
	static Function * getCompositionUnserializeFunction(Module *module, string moduleKey);
	static Function * getSerializeFunction(Module *module);
	static Function * getUnserializeFunction(Module *module);
	static Function * getOpenGraphvizGraphFunction(Module *module);
	static Function * getCloseGraphvizGraphFunction(Module *module);
	static Function * getGetConstantValueFromGraphvizFunction(Module *module);

	static GlobalVariable * getIsPausedVariable(Module *module);
	static GlobalVariable * getTopLevelCompositionIdentifierVariable(Module *module);

	static Type * getParameterTypeBeforeLowering(Function *function, Module *module, string typeName);
	static Value * unlowerArgument(VuoCompilerType *unloweredVuoType, Function *function, int parameterIndex, Module *module, BasicBlock *block);
	static Value * convertArgumentToParameterType(Value *argument, Function *function, int parameterIndex, Value **secondArgument, Module *module, BasicBlock *block);
	static Value * convertArgumentToParameterType(Value *argument, FunctionType *functionType, int parameterIndex, bool isPassedByValue, Value **secondArgument, Module *module, BasicBlock *block);
	static Value * callFunctionWithStructReturn(Function *function, vector<Value *> args, BasicBlock *block);
	static bool isFunctionReturningStructViaParameter(Function *function);
	static FunctionType * getFunctionType(Module *module, VuoType *paramType);
};

#endif
