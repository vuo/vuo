/**
 * @file
 * VuoCompilerCodeGenUtilities interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompilerConstantsCache;
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
	static PointerType * getPointerToFileType(Module *module);
	static PointerType * getVuoShaderType(Module *module);
	static PointerType * getVuoImageType(Module *module);
	static Type * getVuoImageColorDepthType(Module *module);

	static void generateSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue, Function *workerFunction, Value *contextValue, bool isSynchronous);

	static Value * generateConversionToDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable);

	static Value * generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex);
	static Value * generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, Value *elementIndexValue);
	static Value * generateGetStructPointerElementVariable(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex);

	static Value * generateTypeCastFromIntegerToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromFloatingPointToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToInteger(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromPointerToFloatingPoint(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromLoweredTypeToStruct(BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromLoweredTypeToVector(BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static Value * generateTypeCastFromLoweredTypeToArray(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);

	static Function * getNodeFunction(Module *module, string moduleKey, string functionName, bool hasCompositionStateArg, bool hasInstanceDataArg, bool hasInstanceDataReturn, bool hasEventArgs,
									  Type *instanceDataType,
									  const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
									  const map<VuoPort *, json_object *> &detailsForPorts, const map<VuoPort *, string> &displayNamesForPorts,
									  const map<VuoPort *, string> &defaultValuesForInputPorts, const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
									  map<VuoPort *, size_t> &indexOfParameter, map<VuoPort *, size_t> &indexOfEventParameter, VuoCompilerConstantsCache *constantsCache);

	static Function * getVuoRegisterFunction(Module *module);
	static Function * getVuoRetainFunction(Module *module);
	static Function * getVuoReleaseFunction(Module *module);

	static Value * generateStderr(Module *module, BasicBlock *block);

public:
	static PointerType * getDispatchSemaphoreType(Module *module);
	static PointerType * getDispatchGroupType(Module *module);
	static StructType * getDispatchObjectType(Module *module);
	static StructType * getNodeContextType(Module *module);
	static StructType * getPortContextType(Module *module);
	static StructType * getCompositionStateType(Module *module);
	static StructType * getJsonObjectType(Module *module);
	static PointerType * getCompositionInstanceDataType(Module *module);

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

	static Value * generateCreatePortContext(Module *module, BasicBlock *block, VuoCompilerType *dataType, bool isTrigger, std::string triggerQueueName);
	static void generateSetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue, Value *eventValue);
	static void generateSetPortContextData(Module *module, BasicBlock *block, Value *portContextValue, Value *dataValue, VuoCompilerType *dataType);
	static void generateSetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, Value *triggerFunctionValue);
	static Value * generateGetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextDataVariable(Module *module, BasicBlock *block, Value *portContextValue, VuoCompilerType *dataType);
	static Value * generateGetPortContextDataVariableAsVoidPointer(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerQueue(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerSemaphore(Module *module, BasicBlock *block, Value *portContextValue);
	static Value * generateGetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, FunctionType *functionType);
	static void generateRetainPortContextData(Module *module, BasicBlock *block, Value *portContextValue);

	static Value * generateCreateNodeContext(Module *module, BasicBlock *block, bool hasInstanceData, bool isComposition, size_t outputEventCount);
	static void generateSetNodeContextPortContexts(Module *module, BasicBlock *block, Value *nodeContextValue, vector<Value *> portContextValues);
	static void generateSetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *instanceDataValue);
	static void generateSetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue, Value *claimingEventIdValue);
	static void generateSetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index, Value *eventValue);
	static Value * generateGetNodeContextPortContext(Module *module, BasicBlock *block, Value *nodeContextValue, int index);
	static Value * generateGetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType);
	static Value * generateGetNodeContextInstanceDataVariable(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType);
	static Value * generateGetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextExecutingGroup(Module *module, BasicBlock *block, Value *nodeContextValue);
	static Value * generateGetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index);
	static void generateResetNodeContextEvents(Module *module, BasicBlock *block, Value *nodeContextValue);
	static void generateStartedExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue);
	static void generateSpunOffExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue);
	static Value * generateFinishedExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue);
	static Value * generateGetOneExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue);

	static Value * generateCreateCompositionState(Module *module, BasicBlock *block, Value *runtimeStateValue, Value *compositionIdentifierValue);
	static Value * generateGetCompositionStateRuntimeState(Module *module, BasicBlock *block, Value *compositionStateValue);
	static Value * generateGetCompositionStateCompositionIdentifier(Module *module, BasicBlock *block, Value *compositionStateValue);
	static void generateFreeCompositionState(Module *module, BasicBlock *block, Value *compositionStateValue);

	static Value * generateGetDataForPort(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue);
	static Value * generateGetNodeIndexForPort(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue);
	static Value * generateGetTypeIndexForPort(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue);

	static void generateScheduleTriggerWorker(Module *module, BasicBlock *block, Value *queueValue, Value *contextValue, Value *workerFunctionValue,  int minThreadsNeeded, int maxThreadsNeeded, Value *eventIdValue, Value *compositionStateValue, int chainCount);
	static void generateScheduleChainWorker(Module *module, BasicBlock *block, Value *queueValue, Value *contextValue, Value *workerFunctionValue, int minThreadsNeeded, int maxThreadsNeeded, Value *eventIdValue, Value *compositionStateValue, size_t chainIndex, vector<size_t> upstreamChainIndices);
	static void generateGrantThreadsToChain(Module *module, BasicBlock *block, int minThreadsNeeded, int maxThreadsNeeded, Value *eventIdValue, Value *compositionStateValue, size_t chainIndex);
	static void generateGrantThreadsToSubcomposition(Module *module, BasicBlock *block, Value *eventIdValue, Value *compositionStateValue, Value *chainIndexValue, Value *subcompositionIdentifierValue);
	static void generateReturnThreadsForTriggerWorker(Module *module, BasicBlock *block, Value *eventIdValue, Value *compositionStateValue);
	static void generateReturnThreadsForChainWorker(Module *module, BasicBlock *block, Value *eventIdValue, Value *compositionStateValue, Value *chainIndexValue);

	static void generateLockNodes(Module *module, BasicBlock *&block, Value *compositionStateValue, const vector<size_t> &nodeIndices, Value *eventIdValue, VuoCompilerConstantsCache *constantsCache);
	static void generateLockNode(Module *module, BasicBlock *&block, Value *compositionStateValue, size_t nodeIndex, Value *eventIdValue);
	static void generateLockNode(Module *module, BasicBlock *&block, Value *compositionStateValue, Value *nodeIndexValue, Value *eventIdValue);
	static void generateUnlockNodes(Module *module, BasicBlock *block, Value *compositionStateValue, const vector<size_t> &nodeIndices, VuoCompilerConstantsCache *constantsCache);
	static void generateUnlockNode(Module *module, BasicBlock *block, Value *compositionStateValue, size_t nodeIndex);
	static void generateUnlockNode(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeIndexValue);

	static void generateSetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex, Value *value);
	static Value * generateGetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex);
	static Value * generateGetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, Value *elementIndexValue);
	static void generateSetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex, Value *value);
	static Value * generateGetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex);
	static Value * generatePointerToValue(BasicBlock *block, Value *value);
	static Constant * generatePointerToConstantString(Module *module, string stringValue, string globalVariableName = "");
	static Constant * generatePointerToConstantArrayOfStrings(Module *module, vector<string> stringValues, string globalVariableName = "");
	static Constant * generatePointerToConstantArrayOfUnsignedLongs(Module *module, const vector<unsigned long> &values, string globalVariableName = "");
	static void generateStringMatchingCode(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue, map<string, pair<BasicBlock *, BasicBlock *> > blocksForString, VuoCompilerConstantsCache *constantsCache);
	static void generateIndexMatchingCode(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputIndexValue, vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex);
	static Value * generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues, VuoCompilerConstantsCache *constantsCache);
	static Value * generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate, VuoCompilerConstantsCache *constantsCache);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, int elementCount);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, Value *elementCountValue);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, size_t bytes);
	static Value * generateMemoryAllocation(Module *module, BasicBlock *block, VuoCompilerType *type);
	static void generateMemoryCopy(Module *module, BasicBlock *block, Value *sourceAddress, Value *destAddress, size_t bytes);
	static void generateMemoryCopy(Module *module, BasicBlock *block, Value *sourceAddress, Value *destAddress, VuoCompilerType *type);
	static Value * generateTypeCast(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo);
	static void generateAnnotation(Module *module, BasicBlock *block, Value *value, string annotation, string fileName, unsigned int lineNumber, VuoCompilerConstantsCache *constantsCache);
	static void generateModuleMetadata(Module *module, string metadata, string moduleKey);
	static void generateRegisterCall(Module *module, BasicBlock *block, Value *argument, Function *freeFunction);
	static void generateRetainCall(Module *module, BasicBlock *block, Value *argument);
	static void generateReleaseCall(Module *module, BasicBlock *block, Value *argument);
	static void generateRetainOrReleaseCall(Module *module, BasicBlock *block, Value *argument, bool isRetain);
	static void generateFreeCall(Module *module, BasicBlock *block, Value *argument);
	static void generateJsonObjectPut(Module *module, BasicBlock *block, Value *jsonObjectValue);
	static void generateNullCheck(Module *module, Function *function, Value *valueToCheck, BasicBlock *initialBlock, BasicBlock *&nullBlock, BasicBlock *&notNullBlock);
	static Value * generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize, VuoCompilerConstantsCache *constantsCache);
	static void generateUnserialization(Module *module, BasicBlock *block, Value *stringToUnserialize, Value *destinationVariable, VuoCompilerConstantsCache *constantsCache);
	static ICmpInst * generateIsPausedComparison(Module *module, BasicBlock *block, Value *compositionStateValue);
	static void generateSendNodeExecutionStarted(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeIdentifierValue);
	static void generateSendNodeExecutionFinished(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeIdentifierValue);
	static void generateSendInputPortsUpdated(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, bool receivedEvent, bool receivedData, Value *portDataSummaryValue);
	static void generateSendInputPortsUpdated(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *receivedEventValue, Value *receivedDataValue, Value *portDataSummaryValue);
	static void generateSendOutputPortsUpdated(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *sentEventValue, Value *sentDataValue, Value *portDataSummaryValue);
	static void generateSendPublishedOutputPortsUpdated(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *sentDataValue, Value *portDataSummaryValue);
	static void generateSendEventFinished(Module *module, BasicBlock *block, Value *compositionStateValue, Value *eventIdValue);
	static void generateSendEventDropped(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue);
	static ICmpInst * generateShouldSendDataTelemetryComparison(Module *module, BasicBlock *block, string portIdentifier, Value *compositionStateValue, VuoCompilerConstantsCache *constantsCache);
	static void generateIsNodeBeingRemovedOrReplacedCheck(Module *module, Function *function, string nodeIdentifier, Value *compositionStateValue, BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock, VuoCompilerConstantsCache *constantsCache, Value *&replacementJsonValue);
	static ICmpInst * generateIsNodeBeingAddedOrReplacedCheck(Module *module, Function *function, string nodeIdentifier, Value *compositionStateValue, BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock, VuoCompilerConstantsCache *constantsCache, Value *&replacementJsonValue);
	static ConstantInt * generateNoEventIdConstant(Module *module);
	static Value * generateGetNodeContext(Module *module, BasicBlock *block, Value *compositionStateValue, size_t nodeIndex);
	static Value * generateGetNodeContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeIndexValue);
	static Value * generateGetCompositionContext(Module *module, BasicBlock *block, Value *compositionStateValue);
	static void generateAddNodeMetadata(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeIdentifierValue, Function *compositionCreateContextForNodeFunction, Function *compositionSetPortValueFunction, Function *compositionGetPortValueFunction, Function *compositionFireTriggerPortEventFunction, Function *compositionReleasePortDataFunction);
	static void generateAddPortMetadata(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *portNameValue, size_t typeIndex, Value *initialValueValue);
	static void generateInitContextForTopLevelComposition(Module *module, BasicBlock *block, Value *compositionStateValue, bool isStatefulComposition, size_t publishedOutputPortCount);
	static void generateFiniContextForTopLevelComposition(Module *module, BasicBlock *block, Value *compositionStateValue);
	static Value * getTriggerWorkersScheduledValue(Module *module, BasicBlock *block, Value *compositionStateValue);
	static Value * generateGetInputPortString(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *interprocessSerializationValue);
	static Value * generateGetOutputPortString(Module *module, BasicBlock *block, Value *compositionStateValue, Value *portIdentifierValue, Value *interprocessSerializationValue);
	static Value * generateRuntimeStateValue(Module *module, BasicBlock *block);
	static Value * generateGetNextEventId(Module *module, BasicBlock *block, Value *compositionStateValue);
	static Value * generateCreateTriggerWorkerContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *dataCopyValue, Value *eventIdCopyValue);
	static void generateFreeTriggerWorkerContext(Module *module, BasicBlock *block, Value *contextValue);
	static Value * generateCreatePublishedInputWorkerContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *inputPortIdentifierValue, Value *valueAsStringValue, Value *isCompositionRunningValue);
	static void generateAddCompositionStateToThreadLocalStorage(Module *module, BasicBlock *block, Value *compositionStateValue);
	static void generateRemoveCompositionStateFromThreadLocalStorage(Module *module, BasicBlock *block);
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
	static Function * getMemcpyFunction(Module *module);
	static Function * getFreeFunction(Module *module);
	static Function * getAnnotateFunction(Module *module);
	static Function * getJsonObjectPutFunction(Module *module);
	static Function * getJsonObjectToJsonStringExtFunction(Module *module);
	static Function * getJsonTokenerParseFunction(Module *module);
	static Function * getVuoShaderMakeFunction(Module *module);
	static Function * getVuoShaderAddSourceFunction(Module *module);
	static Function * getVuoShaderSetTransparentFunction(Module *module);
	static Function * getVuoShaderSetUniformFunction(Module *module, VuoCompilerType *type);
	static Function * getVuoSamplerRectCoordinatesFromNormalizedCoordinatesFunction(Module *module);
	static Function * getVuoImageGetColorDepthFunction(Module *module);
	static Function * getVuoImageRendererRenderFunction(Module *module);
	static Function * getCompositionAddNodeMetadataFunction(Module *module);
	static Function * getCompositionCreateContextForNodeFunction(Module *module);
	static Function * getCompositionPerformDataOnlyTransmissionsFunction(Module *module);
	static Function * getCompositionReleasePortDataFunction(Module *module);
	static Function * getSetupFunction(Module *module);
	static Function * getCleanupFunction(Module *module);
	static Function * getInstanceInitFunction(Module *module);
	static Function * getInstanceFiniFunction(Module *module);
	static Function * getInstanceTriggerStartFunction(Module *module);
	static Function * getInstanceTriggerStopFunction(Module *module);
	static Function * getNodeInstanceInitFunction(Module *module, string moduleKey, bool isSubcomposition, Type *instanceDataType, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantsCache *constantsCache);
	static Function * getNodeInstanceFiniFunction(Module *module, string moduleKey, Type *instanceDataType, VuoCompilerConstantsCache *constantsCache);
	static Function * getNodeInstanceTriggerStartFunction(Module *module, string moduleKey, Type *instanceDataType, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantsCache *constantsCache);
	static Function * getNodeInstanceTriggerStopFunction(Module *module, string moduleKey, Type *instanceDataType, VuoCompilerConstantsCache *constantsCache);
	static Function * getNodeInstanceTriggerUpdateFunction(Module *module, string moduleKey, Type *instanceDataType, const vector<VuoPort *> &modelInputPorts, map<VuoPort *, size_t> &indexOfParameter, VuoCompilerConstantsCache *constantsCache);
	static Function * getNodeEventFunction(Module *module, string moduleKey, bool isSubcomposition, bool isStateful,
										   Type *instanceDataType, const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
										   const map<VuoPort *, json_object *> &detailsForPorts, const map<VuoPort *, string> &displayNamesForPorts,
										   const map<VuoPort *, string> &defaultValuesForInputPorts, const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
										   map<VuoPort *, size_t> &indexOfParameter, map<VuoPort *, size_t> &indexOfEventParameter, VuoCompilerConstantsCache *constantsCache);
	static Function * getCompositionGetPortValueFunction(Module *module);
	static Function * getSetInputPortValueFunction(Module *module);
	static Function * getCompositionSetPortValueFunction(Module *module);
	static Function * getCompositionFireTriggerPortEventFunction(Module *module);
	static Function * getGetPublishedInputPortValueFunction(Module *module);
	static Function * getGetPublishedOutputPortValueFunction(Module *module);
	static Function * getCompositionSetPublishedInputPortValueFunction(Module *module);
	static Function * getSetPublishedInputPortValueFunction(Module *module);

	static bool isPointerToStruct(Type *type, StructType **structType = nullptr);
	static Value * callFunctionWithStructReturn(Function *function, vector<Value *> args, BasicBlock *block);
	static bool isFunctionReturningStructViaParameter(Function *function);
	static void copyParameterAttributes(Function *srcFunction, Function *dstFunction);
	static void copyParameterAttributes(Module *module, Function *srcFunction, CallInst *dstCall);
	static void copyParameterAttributes(Module *module, const AttributeList &srcAttributes, size_t srcStartParam, size_t srcNumParams, Function *dstFunction, size_t dstStartParam);
	static void copyParameterAttributes(Module *module, const AttributeList &srcAttributes, size_t srcStartParam, size_t srcNumParams, CallInst *dstCall, size_t dstStartParam);
	static FunctionType * getFunctionType(Module *module, VuoType *paramType);
	static Value * getArgumentAtIndex(Function *function, size_t index);
};
