/**
 * @file
 * VuoCompilerCodeGenUtilities implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantsCache.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

/**
 * Generates code that constructs a dispatch_semaphore_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param initialValue The argument to pass to `dispatch_semaphore_create()`.
 * @return The return value of `dispatch_semaphore_create()`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateDispatchSemaphore(Module *module, BasicBlock *block, int initialValue)
{
	PointerType *dispatch_semaphore_t_type = getDispatchSemaphoreType(module);

	Function *dispatch_semaphore_create_function = module->getFunction("dispatch_semaphore_create");
	if (! dispatch_semaphore_create_function)
	{
		vector<Type *> dispatch_semaphore_create_functionParams;
		dispatch_semaphore_create_functionParams.push_back(IntegerType::get(module->getContext(), 64));
		FunctionType *dispatch_semaphore_create_functionType = FunctionType::get(dispatch_semaphore_t_type,
																				 dispatch_semaphore_create_functionParams,
																				 false);
		dispatch_semaphore_create_function = Function::Create(dispatch_semaphore_create_functionType,
															  GlobalValue::ExternalLinkage,
															  "dispatch_semaphore_create",
															  module);
	}

	ConstantInt *initialValueConst = ConstantInt::get(module->getContext(), APInt(64, initialValue));
	return CallInst::Create(dispatch_semaphore_create_function, initialValueConst, "", block);
}

/**
 * Generates code that waits for and claims a dispatch_semaphore_t, with a timeout of @c DISPATCH_TIME_FOREVER.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param semaphoreVariable The semaphore.
 * @return The return value of dispatch_semaphore_wait().
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable)
{
	LoadInst *semaphoreValue = new LoadInst(semaphoreVariable, "", false, block);
	return generateWaitForSemaphore(module, block, semaphoreValue);
}

/**
 * Generates code that waits for and possibly claims a dispatch_semaphore_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param semaphoreVariable The semaphore.
 * @param timeoutValue A value of type dispatch_time_t to pass to dispatch_semaphore_wait().
 * @return The return value of dispatch_semaphore_wait().
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable, Value *timeoutValue)
{
	LoadInst *semaphoreValue = new LoadInst(semaphoreVariable, "", false, block);
	return generateWaitForSemaphore(module, block, semaphoreValue, timeoutValue);
}

/**
 * Generates code that waits for and claims a dispatch_semaphore_t, with a timeout of @c DISPATCH_TIME_FOREVER.
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue)
{
	IntegerType *dispatch_time_t_type = IntegerType::get(module->getContext(), 64);
	Value *timeoutValue = ConstantInt::get(dispatch_time_t_type, DISPATCH_TIME_FOREVER);
	return generateWaitForSemaphore(module, block, semaphoreValue, timeoutValue);
}

/**
 * Generates code that waits for and possibly claims a dispatch_semaphore_t.
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue, Value *timeoutValue)
{
	PointerType *dispatch_semaphore_t_type = getDispatchSemaphoreType(module);
	IntegerType *dispatch_time_t_type = IntegerType::get(module->getContext(), 64);

	vector<Type *> dispatch_semaphore_wait_functionParams;
	dispatch_semaphore_wait_functionParams.push_back(dispatch_semaphore_t_type);
	dispatch_semaphore_wait_functionParams.push_back(dispatch_time_t_type);
	FunctionType *dispatch_semaphore_wait_functionType = FunctionType::get(IntegerType::get(module->getContext(), 64),
																		   dispatch_semaphore_wait_functionParams,
																		   false);

	Function *dispatch_semaphore_wait_function = module->getFunction("dispatch_semaphore_wait");
	if (! dispatch_semaphore_wait_function) {
		dispatch_semaphore_wait_function = Function::Create(dispatch_semaphore_wait_functionType,
															GlobalValue::ExternalLinkage,
															"dispatch_semaphore_wait",
															module);
	}

	vector<Value *> args;
	args.push_back(semaphoreValue);
	args.push_back(timeoutValue);
	return CallInst::Create(dispatch_semaphore_wait_function, args, "", block);
}

/**
 * Generates code that signals a dispatch_semaphore_t.
 */
void VuoCompilerCodeGenUtilities::generateSignalForSemaphore(Module *module, BasicBlock *block, AllocaInst *semaphoreVariable)
{
	LoadInst *semaphoreValue = new LoadInst(semaphoreVariable, "", false, block);
	return generateSignalForSemaphore(module, block, semaphoreValue);
}

/**
 * Generates code that signals a dispatch_semaphore_t.
 */
void VuoCompilerCodeGenUtilities::generateSignalForSemaphore(Module *module, BasicBlock *block, Value *semaphoreValue)
{
	PointerType *dispatch_semaphore_t_type = getDispatchSemaphoreType(module);

	vector<Type *> dispatch_semaphore_signal_functionParams;
	dispatch_semaphore_signal_functionParams.push_back(dispatch_semaphore_t_type);
	FunctionType *dispatch_semaphore_signal_functionType = FunctionType::get(IntegerType::get(module->getContext(), 64),
																			 dispatch_semaphore_signal_functionParams,
																			 false);

	Function *dispatch_semaphore_signal_function = module->getFunction("dispatch_semaphore_signal");
	if (! dispatch_semaphore_signal_function) {
		dispatch_semaphore_signal_function = Function::Create(dispatch_semaphore_signal_functionType,
															  GlobalValue::ExternalLinkage,
															  "dispatch_semaphore_signal",
															  module);
	}

	CallInst::Create(dispatch_semaphore_signal_function, semaphoreValue, "", block);
}

/**
 * Generates code that constructs a dispatch_group_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @return The return value of `dispatch_group_create()`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateDispatchGroup(Module *module, BasicBlock *block)
{
	Function *dispatch_group_create_function = module->getFunction("dispatch_group_create");
	if (! dispatch_group_create_function)
	{
		vector<Type *> dispatch_group_create_functionParams;
		FunctionType *dispatch_group_create_functionType = FunctionType::get(getDispatchGroupType(module),
																			 dispatch_group_create_functionParams,
																			 false);
		dispatch_group_create_function = Function::Create(dispatch_group_create_functionType,
														  GlobalValue::ExternalLinkage,
														  "dispatch_group_create", module);
	}

	return CallInst::Create(dispatch_group_create_function, "", block);
}

/**
 * Generates code that increments the block count of a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateEnterDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue)
{
	Function *dispatch_group_enter_function = module->getFunction("dispatch_group_enter");
	if (! dispatch_group_enter_function)
	{
		PointerType *dispatch_group_t_type = getDispatchGroupType(module);
		FunctionType *dispatch_group_enter_functionType = FunctionType::get(Type::getVoidTy(module->getContext()),
																		   dispatch_group_t_type,
																		   false);
		dispatch_group_enter_function = Function::Create(dispatch_group_enter_functionType,
														GlobalValue::ExternalLinkage,
														"dispatch_group_enter",
														module);
	}

	CallInst::Create(dispatch_group_enter_function, dispatchGroupValue, "", block);
}

/**
 * Generates code that decrements the block count of a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue)
{
	Function *dispatch_group_leave_function = module->getFunction("dispatch_group_leave");
	if (! dispatch_group_leave_function)
	{
		PointerType *dispatch_group_t_type = getDispatchGroupType(module);
		FunctionType *dispatch_group_leave_functionType = FunctionType::get(Type::getVoidTy(module->getContext()),
																		   dispatch_group_t_type,
																		   false);
		dispatch_group_leave_function = Function::Create(dispatch_group_leave_functionType,
														GlobalValue::ExternalLinkage,
														"dispatch_group_leave",
														module);
	}

	CallInst::Create(dispatch_group_leave_function, dispatchGroupValue, "", block);
}

/**
 * Generates code that waits on a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(Module *module, BasicBlock *block, Value *dispatchGroupValue,
															   dispatch_time_t timeout)
{
	Function *dispatch_group_wait_function = module->getFunction("dispatch_group_wait");
	if (! dispatch_group_wait_function)
	{
		PointerType *dispatch_group_t_type = getDispatchGroupType(module);

		vector<Type *> dispatch_group_wait_functionParams;
		dispatch_group_wait_functionParams.push_back(dispatch_group_t_type);
		dispatch_group_wait_functionParams.push_back(IntegerType::get(module->getContext(), 64));
		FunctionType *dispatch_group_wait_functionType = FunctionType::get(IntegerType::get(module->getContext(), 64),
																		   dispatch_group_wait_functionParams,
																		   false);

		dispatch_group_wait_function = Function::Create(dispatch_group_wait_functionType,
														GlobalValue::ExternalLinkage,
														"dispatch_group_wait",
														module);
	}

	ConstantInt *timeout_value = ConstantInt::get(module->getContext(), APInt(64, timeout, true));

	vector<Value *> args;
	args.push_back(dispatchGroupValue);
	args.push_back(timeout_value);
	CallInst::Create(dispatch_group_wait_function, args, "", block);
}

/**
 * Generates code that retrieves the global dispatch queue.
 */
Value * VuoCompilerCodeGenUtilities::generateGetGlobalDispatchQueue(Module *module, BasicBlock *block)
{
	PointerType *dispatch_queue_t_type = getDispatchQueueType(module);

	vector<Type *> dispatch_get_global_queue_functionParams;
	dispatch_get_global_queue_functionParams.push_back(IntegerType::get(module->getContext(), 64));
	dispatch_get_global_queue_functionParams.push_back(IntegerType::get(module->getContext(), 64));
	FunctionType *dispatch_get_global_queue_functionType = FunctionType::get(dispatch_queue_t_type,
																			 dispatch_get_global_queue_functionParams,
																			 false);

	Function *dispatch_get_global_queue_function = module->getFunction("dispatch_get_global_queue");
	if (! dispatch_get_global_queue_function) {
		dispatch_get_global_queue_function = Function::Create(dispatch_get_global_queue_functionType,
															  GlobalValue::ExternalLinkage,
															  "dispatch_get_global_queue",
															  module);
	}

	Constant *zeroValue = ConstantInt::get(dispatch_get_global_queue_functionType->getParamType(0), 0);

	vector<Value *> args;
	args.push_back(zeroValue);
	args.push_back(zeroValue);
	return CallInst::Create(dispatch_get_global_queue_function, args, "", block);
}

/**
 * Generates code that constructs a dispatch_queue_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param dispatchQueueName The argument to pass to `dispatch_queue_create()`.
 * @return The return value of `dispatch_queue_create()`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateDispatchQueue(Module *module, BasicBlock *block, string dispatchQueueName)
{
	PointerType *dispatch_queue_t_type = getDispatchQueueType(module);
	PointerType *pointerToi8 = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	StructType *dispatch_queue_attr_s_type = module->getTypeByName("struct.dispatch_queue_attr_s");
	if (! dispatch_queue_attr_s_type)
		dispatch_queue_attr_s_type = StructType::create(module->getContext(), "struct.dispatch_queue_attr_s");
	PointerType *dispatch_queue_attr_t_type = PointerType::get(dispatch_queue_attr_s_type, 0);

	vector<Type *> dispatch_queue_create_functionParams;
	dispatch_queue_create_functionParams.push_back(pointerToi8);
	dispatch_queue_create_functionParams.push_back(dispatch_queue_attr_t_type);
	FunctionType *dispatch_queue_create_functionType = FunctionType::get(dispatch_queue_t_type,
																		 dispatch_queue_create_functionParams,
																		 false);

	Function *dispatch_queue_create_function = module->getFunction("dispatch_queue_create");
	if (! dispatch_queue_create_function)
	{
		dispatch_queue_create_function = Function::Create(dispatch_queue_create_functionType,
														  GlobalValue::ExternalLinkage,
														  "dispatch_queue_create",
														  module);
	}

	ArrayType *dispatchQueueNameType = ArrayType::get(IntegerType::get(module->getContext(), 8), dispatchQueueName.length() + 1);
	GlobalVariable *dispatchQueueNameVariable = new GlobalVariable(*module,
																   dispatchQueueNameType,
																   true,
																   GlobalValue::InternalLinkage,
																   0,
																   ".str");
	Constant *dispatchQueueNameValue = ConstantDataArray::getString(module->getContext(), dispatchQueueName, true);
	dispatchQueueNameVariable->setInitializer(dispatchQueueNameValue);

	ConstantInt *zeroi64Value = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantPointerNull *nullValue = ConstantPointerNull::get(dispatch_queue_attr_t_type);

	std::vector<Constant*> dispatchQueueLabel_indices;
	dispatchQueueLabel_indices.push_back(zeroi64Value);
	dispatchQueueLabel_indices.push_back(zeroi64Value);
	Constant *dispatchQueueLabel = ConstantExpr::getGetElementPtr(dispatchQueueNameVariable, dispatchQueueLabel_indices);

	vector<Value *> args;
	args.push_back(dispatchQueueLabel);
	args.push_back(nullValue);
	return CallInst::Create(dispatch_queue_create_function, args, "", block);
}

/**
 * Generates code that submits a function for asynchronous execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue,
																				Function *workerFunction, Value *contextValue)
{
	generateSubmissionToDispatchQueue(module, block, dispatchQueueValue, workerFunction, contextValue, false);
}

/**
 * Generates code that submits a function for synchronous execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue,
																			   Function *workerFunction, Value *contextValue)
{
	generateSubmissionToDispatchQueue(module, block, dispatchQueueValue, workerFunction, contextValue, true);
}

/**
 * Generates code that submits a function for execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *dispatchQueueValue,
																	Function *workerFunction, Value *contextValue, bool isSynchronous)
{
	PointerType *dispatch_queue_t_type = getDispatchQueueType(module);
	PointerType *pointerToi8 = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> dispatchSubmitFunctionParams;
	dispatchSubmitFunctionParams.push_back(dispatch_queue_t_type);
	dispatchSubmitFunctionParams.push_back(pointerToi8);
	dispatchSubmitFunctionParams.push_back(workerFunction->getType());
	FunctionType *dispatchSubmitFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()),
																 dispatchSubmitFunctionParams,
																 false);

	const char *dispatchSubmitFunctionName = (isSynchronous ? "dispatch_sync_f" : "dispatch_async_f");
	Function *dispatchSubmitFunction = module->getFunction(dispatchSubmitFunctionName);
	if (! dispatchSubmitFunction) {
		dispatchSubmitFunction = Function::Create(dispatchSubmitFunctionType,
												  GlobalValue::ExternalLinkage,
												  dispatchSubmitFunctionName,
												  module);
	}

	vector<Value *> args;
	args.push_back(dispatchQueueValue);
	args.push_back(contextValue);
	args.push_back(workerFunction);
	CallInst::Create(dispatchSubmitFunction, args, "", block);
}

/**
 * Converts dispatchObjectVariable to an actual @c dispatch_object_t.
 */
Value * VuoCompilerCodeGenUtilities::generateConversionToDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable)
{
	PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	PointerType *dispatchObjectOriginalType = cast<PointerType>(dispatchObjectVariable->getType());
	StructType *dispatch_object_t_type = getDispatchObjectType(module);

	vector<Type *> llvm_memcpy_p0i8_p0i8_i64_functionParams;
	llvm_memcpy_p0i8_p0i8_i64_functionParams.push_back(pointerToi8Type);
	llvm_memcpy_p0i8_p0i8_i64_functionParams.push_back(pointerToi8Type);
	llvm_memcpy_p0i8_p0i8_i64_functionParams.push_back(IntegerType::get(module->getContext(), 64));
	llvm_memcpy_p0i8_p0i8_i64_functionParams.push_back(IntegerType::get(module->getContext(), 32));
	llvm_memcpy_p0i8_p0i8_i64_functionParams.push_back(IntegerType::get(module->getContext(), 1));
	FunctionType *llvm_memcpy_p0i8_p0i8_i64_functionType = FunctionType::get(Type::getVoidTy(module->getContext()), llvm_memcpy_p0i8_p0i8_i64_functionParams, false);
	Function *llvm_memcpy_p0i8_p0i8_i64_function = module->getFunction("llvm.memcpy.p0i8.p0i8.i64");
	if (! llvm_memcpy_p0i8_p0i8_i64_function)
		llvm_memcpy_p0i8_p0i8_i64_function = Function::Create(llvm_memcpy_p0i8_p0i8_i64_functionType, GlobalValue::ExternalLinkage, "llvm.memcpy.p0i8.p0i8.i64", module);

	ConstantInt *zeroValue32 = ConstantInt::get(module->getContext(), APInt(32, 0));
	ConstantInt *zeroValue1 = ConstantInt::get(module->getContext(), APInt(1, 0));
	ConstantInt *eightValue64 = ConstantInt::get(module->getContext(), APInt(64, 8));
	ConstantInt *eightValue32 = ConstantInt::get(module->getContext(), APInt(32, 8));

	AllocaInst *dispatchObjectUnion = new AllocaInst(dispatch_object_t_type, "", block);
	AllocaInst *compoundLiteral = new AllocaInst(dispatch_object_t_type, "", block);

	CastInst *compoundLiteralAsDispatchObjectOriginalType = new BitCastInst(compoundLiteral, dispatchObjectOriginalType, "", block);
	LoadInst *dispatchObjectValue = new LoadInst(dispatchObjectVariable, "", false, block);
	new StoreInst(dispatchObjectValue, compoundLiteralAsDispatchObjectOriginalType, false, block);

	CastInst *dispatchObjectUnionAsPointerToi8 = new BitCastInst(dispatchObjectUnion, pointerToi8Type, "", block);
	CastInst *compoundLiteralAsPointerToi8 = new BitCastInst(compoundLiteral, pointerToi8Type, "", block);

	std::vector<Value *> llvm_memcpy_p0i8_p0i8_i64_functionArgs;
	llvm_memcpy_p0i8_p0i8_i64_functionArgs.push_back(dispatchObjectUnionAsPointerToi8);
	llvm_memcpy_p0i8_p0i8_i64_functionArgs.push_back(compoundLiteralAsPointerToi8);
	llvm_memcpy_p0i8_p0i8_i64_functionArgs.push_back(eightValue64);
	llvm_memcpy_p0i8_p0i8_i64_functionArgs.push_back(eightValue32);
	llvm_memcpy_p0i8_p0i8_i64_functionArgs.push_back(zeroValue1);
	CallInst::Create(llvm_memcpy_p0i8_p0i8_i64_function, llvm_memcpy_p0i8_p0i8_i64_functionArgs, "", block);

	vector<Value *> gepIndices;
	gepIndices.push_back(zeroValue32);
	gepIndices.push_back(zeroValue32);
	Instruction *dispatchObjectUnionMember = GetElementPtrInst::Create(dispatchObjectUnion, gepIndices, "", block);
	return new LoadInst(dispatchObjectUnionMember, "", false, block);
}

/**
 * Generates code that retains a dispatch_object_t (dispatch_queue_t, dispatch_group_t, dispatch_semaphore_t, etc.).
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param dispatchObjectVariable A pointer to the dispatch_object_t (e.g. a @c GlobalVariable or @c AllocaInst).
 */
void VuoCompilerCodeGenUtilities::generateRetainForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable)
{
	Value *dispatchObjectValueAsDispatchObject = generateConversionToDispatchObject(module, block, dispatchObjectVariable);

	StructType *dispatch_object_s_type = getDispatchObjectElementType(module);
	PointerType *pointerTo_dispatch_object_s_type = PointerType::get(dispatch_object_s_type, 0);

	vector<Type*> dispatchRetainFunctionParams;
	dispatchRetainFunctionParams.push_back(pointerTo_dispatch_object_s_type);
	FunctionType *dispatchRetainFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), dispatchRetainFunctionParams, false);
	Function *dispatchRetainFunction = module->getFunction("dispatch_retain");
	if (! dispatchRetainFunction)
		dispatchRetainFunction = Function::Create(dispatchRetainFunctionType, GlobalValue::ExternalLinkage, "dispatch_retain", module);

	CallInst::Create(dispatchRetainFunction, dispatchObjectValueAsDispatchObject, "", block);
}

/**
 * Generates code that releases a dispatch_object_t (dispatch_queue_t, dispatch_group_t, dispatch_semaphore_t, etc.).
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param dispatchObjectVariable A pointer to the dispatch_object_t (e.g. a @c GlobalVariable or @c AllocaInst).
 */
void VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(Module *module, BasicBlock *block, Value *dispatchObjectVariable)
{
	Value *dispatchObjectValueAsDispatchObject = generateConversionToDispatchObject(module, block, dispatchObjectVariable);

	StructType *dispatch_object_s_type = getDispatchObjectElementType(module);
	PointerType *pointerTo_dispatch_object_s_type = PointerType::get(dispatch_object_s_type, 0);

	vector<Type*> dispatchReleaseFunctionParams;
	dispatchReleaseFunctionParams.push_back(pointerTo_dispatch_object_s_type);
	FunctionType *dispatchReleaseFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), dispatchReleaseFunctionParams, false);
	Function *dispatchReleaseFunction = module->getFunction("dispatch_release");
	if (! dispatchReleaseFunction)
		dispatchReleaseFunction = Function::Create(dispatchReleaseFunctionType, GlobalValue::ExternalLinkage, "dispatch_release", module);

	CallInst::Create(dispatchReleaseFunction, dispatchObjectValueAsDispatchObject, "", block);
}

/**
 * Generates code that creates a dispatch_time_t by calling `dispatch_time_create(DISPATCH_TIME_NOW, ...)`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param deltaValue The 2nd argument to `dispatch_time_create`.
 * @return The created dispatch_time_t.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateDispatchTime(Module *module, BasicBlock *block, Value *deltaValue)
{
	Type *dispatch_time_t_type = IntegerType::get(module->getContext(), 64);

	vector<Type *> dispatch_time_functionParams;
	dispatch_time_functionParams.push_back(dispatch_time_t_type);
	dispatch_time_functionParams.push_back(IntegerType::get(module->getContext(), 64));
	FunctionType *dispatch_time_functionType = FunctionType::get(dispatch_time_t_type,
																 dispatch_time_functionParams,
																 false);

	Function *dispatch_time_function = module->getFunction("dispatch_time");
	if (! dispatch_time_function) {
		dispatch_time_function = Function::Create(dispatch_time_functionType,
												  GlobalValue::ExternalLinkage,
												  "dispatch_time",
												  module);
	}

	ConstantInt *whenValue = ConstantInt::get(module->getContext(), APInt(64, DISPATCH_TIME_NOW));

	vector<Value *> args;
	args.push_back(whenValue);
	args.push_back(deltaValue);
	return CallInst::Create(dispatch_time_function, args, "", block);
}

/**
 * Generates code that allocates a PortContext on the heap and initializes it with default values.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param dataType The type of the port's data, or null if the port is event-only.
 * @param isTrigger Whether the port is a trigger.
 * @param triggerQueueName If the port is a trigger, the name to use when creating its dispatch queue.
 * @return A value of type `PortContext *`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreatePortContext(Module *module, BasicBlock *block, Type *dataType,
															   bool isTrigger, string triggerQueueName)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 64);
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	const char *functionName = "vuoCreatePortContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToPortContext = PointerType::get(getPortContextType(module), 0);

		vector<Type *> params;
		params.push_back(voidPointerType);
		params.push_back(boolType);
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(pointerToPortContext, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *pointerToDataAsVoidPointer;
	if (dataType)
	{
		Value *pointerToData = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, dataType, 1);
		pointerToDataAsVoidPointer = new BitCastInst(pointerToData, voidPointerType, "", block);
	}
	else
		pointerToDataAsVoidPointer = ConstantPointerNull::get(voidPointerType);

	vector<Value *> args;
	args.push_back(pointerToDataAsVoidPointer);
	args.push_back(ConstantInt::get(boolType, isTrigger));
	args.push_back(generatePointerToConstantString(module, triggerQueueName));
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the `event` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param eventValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue, Value *eventValue)
{
	const char *functionName = "vuoSetPortContextEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *boolType = portContextType->getElementType(0);

		vector<Type *> params;
		params.push_back(pointerToPortContext);
		params.push_back(boolType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(portContextValue);
	args.push_back(eventValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the `data` field of a PortContext to a heap-allocated copy of @a dataValue.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param dataValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetPortContextData(Module *module, BasicBlock *block, Value *portContextValue, Value *dataValue)
{
	Value *pointerToData = generateGetPortContextDataVariable(module, block, portContextValue, dataValue->getType());
	new StoreInst(dataValue, pointerToData, false, block);
}

/**
 * Generates code that sets the `triggerFunction` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param triggerFunctionValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, Value *triggerFunctionValue)
{
	const char *functionName = "vuoSetPortContextTriggerFunction";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *triggerFunctionType = portContextType->getElementType(5);

		vector<Type *> params;
		params.push_back(pointerToPortContext);
		params.push_back(triggerFunctionType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *triggerFunctionAsVoidPointer = new BitCastInst(triggerFunctionValue, voidPointerType, "", block);

	vector<Value *> args;
	args.push_back(portContextValue);
	args.push_back(triggerFunctionAsVoidPointer);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the `event` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextEvent(Module *module, BasicBlock *block, Value *portContextValue)
{
	const char *functionName = "vuoGetPortContextEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *boolType = portContextType->getElementType(0);

		FunctionType *functionType = FunctionType::get(boolType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, portContextValue, "", block);
}

/**
 * Generates code that gets the `data` field of a PortContext (and dereferences it to extract the actual data).
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param dataType The type that the data should be converted to before returning.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextData(Module *module, BasicBlock *block, Value *portContextValue, Type *dataType)
{
	Value *pointerToData = generateGetPortContextDataVariable(module, block, portContextValue, dataType);
	return new LoadInst(pointerToData, "", false, block);
}

/**
 * Generates code that gets the address of the `data` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param dataType The type that the data should be converted to before returning.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextDataVariable(Module *module, BasicBlock *block, Value *portContextValue, Type *dataType)
{
	Value *pointerToDataAsVoidPointer = generateGetPortContextDataVariableAsVoidPointer(module, block, portContextValue);
	return new BitCastInst(pointerToDataAsVoidPointer, PointerType::get(dataType, 0), "", block);
}

/**
 * Generates code that gets the address of the `data` field of a PortContext as a void pointer.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextDataVariableAsVoidPointer(Module *module, BasicBlock *block, Value *portContextValue)
{
	const char *functionName = "vuoGetPortContextData";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *voidPointerType = portContextType->getElementType(1);

		FunctionType *functionType = FunctionType::get(voidPointerType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, portContextValue, "", block);
}

/**
 * Generates code that gets the `triggerQueue` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextTriggerQueue(Module *module, BasicBlock *block, Value *portContextValue)
{
	const char *functionName = "vuoGetPortContextTriggerQueue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *dispatchQueueType = portContextType->getElementType(3);

		FunctionType *functionType = FunctionType::get(dispatchQueueType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, portContextValue, "", block);
}

/**
 * Generates code that gets the `triggerSemaphore` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextTriggerSemaphore(Module *module, BasicBlock *block, Value *portContextValue)
{
	const char *functionName = "vuoGetPortContextTriggerSemaphore";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *dispatchSemaphoreType = portContextType->getElementType(4);

		FunctionType *functionType = FunctionType::get(dispatchSemaphoreType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, portContextValue, "", block);
}

/**
 * Generates code that gets the `triggerFunction` field of a PortContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 * @param functionType The type that the function value should be converted to before returning.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetPortContextTriggerFunction(Module *module, BasicBlock *block, Value *portContextValue, FunctionType *functionType)
{
	const char *functionName = "vuoGetPortContextTriggerFunction";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);
		Type *triggerFunctionType = portContextType->getElementType(5);

		FunctionType *functionType = FunctionType::get(triggerFunctionType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *triggerFunctionAsVoidPointer = CallInst::Create(function, portContextValue, "", block);
	PointerType *pointerToTriggerFunctionType = PointerType::get(functionType, 0);
	return new BitCastInst(triggerFunctionAsVoidPointer, pointerToTriggerFunctionType, "", block);
}

/**
 * Generates code that calls `vuoRetainPortContextData()`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param portContextValue A value of type `PortContext *`.
 */
void VuoCompilerCodeGenUtilities::generateRetainPortContextData(Module *module, BasicBlock *block, Value *portContextValue)
{
	const char *functionName = "vuoRetainPortContextData";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);

		vector<Type *> params;
		params.push_back(pointerToPortContext);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(portContextValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that allocates a NodeContext on the heap and initializes it with default values.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param hasInstanceData Whether this node has instance data.
 * @param isComposition Whether this node is a subcomposition.
 * @param outputEventCount The number of output ports, used to create the `outputEvents` field for subcompositions.
 * @return A value of type `NodeContext *`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateNodeContext(Module *module, BasicBlock *block, bool hasInstanceData,
															   bool isComposition, size_t outputEventCount)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 64);
	IntegerType *sizeType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoCreateNodeContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContext = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> params;
		params.push_back(boolType);
		params.push_back(boolType);
		params.push_back(sizeType);

		FunctionType *functionType = FunctionType::get(pointerToNodeContext, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(ConstantInt::get(boolType, hasInstanceData));
	args.push_back(ConstantInt::get(boolType, isComposition));
	args.push_back(ConstantInt::get(sizeType, outputEventCount));
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the `portContexts` and `portContextCount` fields of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param portContextValues The values to be placed in an array and stored in the field.
 */
void VuoCompilerCodeGenUtilities::generateSetNodeContextPortContexts(Module *module, BasicBlock *block, Value *nodeContextValue, vector<Value *> portContextValues)
{
	PointerType *pointerToPortContext = PointerType::get(getPortContextType(module), 0);
	PointerType *pointerToPointerToPortContext = PointerType::get(pointerToPortContext, 0);
	IntegerType *sizeType = IntegerType::get(module->getContext(), 64);

	Value *portContextsArrayValue = generateMemoryAllocation(module, block, pointerToPortContext, portContextValues.size());
	for (size_t i = 0; i < portContextValues.size(); ++i)
		generateSetArrayElement(module, block, portContextsArrayValue, i, portContextValues[i]);

	const char *functionName = "vuoSetNodeContextPortContexts";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(pointerToPointerToPortContext);
		params.push_back(sizeType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(portContextsArrayValue);
	args.push_back(ConstantInt::get(sizeType, portContextValues.size()));
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the `instanceData` field of a NodeContext (to a heap-allocated copy of @a instanceDataValue) and frees the old value.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param instanceDataValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *instanceDataValue)
{
	StructType *nodeContextType = getNodeContextType(module);
	Type *voidPointerType = nodeContextType->getElementType(2);

	const char *functionName = "vuoSetNodeContextInstanceData";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(voidPointerType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *pointerToInstanceData = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, instanceDataValue->getType(), 1);
	new StoreInst(instanceDataValue, pointerToInstanceData, false, block);
	Value *pointerToInstanceDataAsVoidPointer = new BitCastInst(pointerToInstanceData, voidPointerType, "", block);

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(pointerToInstanceDataAsVoidPointer);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the `claimingEventId` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param claimingEventIdValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue, Value *claimingEventIdValue)
{
	const char *functionName = "vuoSetNodeContextClaimingEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *eventIdType = nodeContextType->getElementType(4);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(eventIdType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(claimingEventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets an element of the `outputEvents` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param index The index of the event in the node context's array of output events.
 * @param eventValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index, Value *eventValue)
{
	Type *sizeType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoSetNodeContextOutputEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *boolType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(sizeType);
		params.push_back(boolType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	eventValue = new ZExtInst(eventValue, sizeType, "", block);

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(ConstantInt::get(sizeType, index));
	args.push_back(eventValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets an element of the `portContexts` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param index The index of the port context in the node context's array of port contexts.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(Module *module, BasicBlock *block, Value *nodeContextValue, int index)
{
	Type *indexType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoGetNodeContextPortContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		StructType *portContextType = getPortContextType(module);
		PointerType *pointerToPortContext = PointerType::get(portContextType, 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(indexType);

		FunctionType *functionType = FunctionType::get(pointerToPortContext, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(ConstantInt::get(indexType, index));
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the `instanceData` field of a NodeContext (and dereference it to get the actual instance data).
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param instanceDataType The type that the `instanceData` field should be converted to before returning.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextInstanceData(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType)
{
	Value *pointerToInstanceData = generateGetNodeContextInstanceDataVariable(module, block, nodeContextValue, instanceDataType);
	return new LoadInst(pointerToInstanceData, "", false, block);
}

/**
 * Generates code that gets the address of the `instanceData` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param instanceDataType The type that the `instanceData` field should be converted to before returning.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextInstanceDataVariable(Module *module, BasicBlock *block, Value *nodeContextValue, Type *instanceDataType)
{
	const char *functionName = "vuoGetNodeContextInstanceData";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *voidPointerType = nodeContextType->getElementType(2);

		FunctionType *functionType = FunctionType::get(voidPointerType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *pointerToInstanceDataAsVoidPointer = CallInst::Create(function, nodeContextValue, "", block);
	return new BitCastInst(pointerToInstanceDataAsVoidPointer, PointerType::get(instanceDataType, 0), "", block);
}

/**
 * Generates code that gets the `claimingEventId` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextClaimingEventId(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoGetNodeContextClaimingEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *eventIdType = nodeContextType->getElementType(4);

		FunctionType *functionType = FunctionType::get(eventIdType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, nodeContextValue, "", block);
}

/**
 * Generates code that gets the `executingGroup` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoGetNodeContextExecutingGroup";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *dispatchGroupType = nodeContextType->getElementType(5);

		FunctionType *functionType = FunctionType::get(dispatchGroupType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, nodeContextValue, "", block);
}

/**
 * Generates code that gets an element of the `outputEvents` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param index The index of the event in the node context's array of output events.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextOutputEvent(Module *module, BasicBlock *block, Value *nodeContextValue, size_t index)
{
	Type *sizeType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoGetNodeContextOutputEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *boolType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(sizeType);
		FunctionType *functionType = FunctionType::get(boolType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(ConstantInt::get(sizeType, index));
	Value *eventValue = CallInst::Create(function, args, "", block);

	return new TruncInst(eventValue, IntegerType::get(module->getContext(), 1), "", block);
}

/**
 * Generates code that sets the `event` field to false for all PortContexts in a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 */
void VuoCompilerCodeGenUtilities::generateResetNodeContextEvents(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoResetNodeContextEvents";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoStartedExecutingEvent`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue The composition's node context, a value of type `NodeContext *`.
 * @param eventIdValue A value of type `unsigned long`.
 */
void VuoCompilerCodeGenUtilities::generateStartedExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue)
{
	const char *functionName = "vuoStartedExecutingEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSpunOffExecutingEvent`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue The composition's node context, a value of type `NodeContext *`.
 * @param eventIdValue A value of type `unsigned long`.
 */
void VuoCompilerCodeGenUtilities::generateSpunOffExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue)
{
	const char *functionName = "vuoSpunOffExecutingEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoFinishedExecutingEvent`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue The composition's node context, a value of type `NodeContext *`.
 * @param eventIdValue A value of type `unsigned long`.
 * @return A value of type `bool`.
 */
Value * VuoCompilerCodeGenUtilities::generateFinishedExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventIdValue)
{
	const char *functionName = "vuoFinishedExecutingEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);
		Type *eventIdType = generateNoEventIdConstant(module)->getType();
		Type *boolType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(boolType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(eventIdValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetOneExecutingEvent`.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue The composition's node context, a value of type `NodeContext *`.
 * @return A value of type `unsigned long`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetOneExecutingEvent(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoGetOneExecutingEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);

		FunctionType *functionType = FunctionType::get(eventIdType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that creates a `VuoCompositionState *` from the given `void *runtimeState` and `char *compositionIdentifier`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateCompositionState(Module *module, BasicBlock *block, Value *runtimeStateValue, Value *compositionIdentifierValue)
{
	const char *functionName = "vuoCreateCompositionState";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> params;
		params.push_back(voidPointer);
		params.push_back(pointerToChar);

		FunctionType *functionType = FunctionType::get(pointerToCompositionState, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(runtimeStateValue);
	args.push_back(compositionIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the `runtimeState` field of a `VuoCompositionState *`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetCompositionStateRuntimeState(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoGetCompositionStateRuntimeState";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);

		FunctionType *functionType = FunctionType::get(voidPointer, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the `compositionIdentifier` field of a `VuoCompositionState *`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetCompositionStateCompositionIdentifier(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoGetCompositionStateCompositionIdentifier";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);

		FunctionType *functionType = FunctionType::get(pointerToChar, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that frees a `VuoCompositionState *` (but not its fields).
 */
void VuoCompilerCodeGenUtilities::generateFreeCompositionState(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoFreeCompositionState";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrives the `data` field in a port's context, given the port's identifier.
 */
Value * VuoCompilerCodeGenUtilities::generateGetDataForPort(Module *module, BasicBlock *block,
															Value *compositionStateValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetDataForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(voidPointerType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrieves the index (in VuoCompilerBitcodeGenerator::orderedNodes) of a node, given the identifier of a port on the node.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeIndexForPort(Module *module, BasicBlock *block,
																 Value *compositionStateValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetNodeIndexForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(unsignedLongType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrieves the index (in VuoCompilerBitcodeGenerator::orderedTypes) of a port's type, given the port's identifier.
 */
Value * VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(Module *module, BasicBlock *block,
																 Value *compositionStateValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetTypeIndexForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(unsignedLongType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoScheduleTriggerWorker`.
 */
void VuoCompilerCodeGenUtilities::generateScheduleTriggerWorker(Module *module, BasicBlock *block,
																Value *queueValue, Value *contextValue, Value *workerFunctionValue,
																int minThreadsNeeded, int maxThreadsNeeded,
																Value *eventIdValue, Value *compositionStateValue,
																int chainCount)
{
	Type *intType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoScheduleTriggerWorker";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *dispatchQueueType = getDispatchQueueType(module);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *workerFunctionType = workerFunctionValue->getType();
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(dispatchQueueType);
		params.push_back(voidPointerType);
		params.push_back(workerFunctionType);
		params.push_back(intType);
		params.push_back(intType);
		params.push_back(eventIdType);
		params.push_back(intType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *minThreadsNeededValue = ConstantInt::get(intType, minThreadsNeeded);
	Value *maxThreadsNeededValue = ConstantInt::get(intType, maxThreadsNeeded);
	Value *chainCountValue = ConstantInt::get(intType, chainCount);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(queueValue);
	args.push_back(contextValue);
	args.push_back(workerFunctionValue);
	args.push_back(minThreadsNeededValue);
	args.push_back(maxThreadsNeededValue);
	args.push_back(eventIdValue);
	args.push_back(chainCountValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoScheduleChainWorker`.
 */
void VuoCompilerCodeGenUtilities::generateScheduleChainWorker(Module *module, BasicBlock *block,
															  Value *queueValue, Value *contextValue, Value *workerFunctionValue,
															  int minThreadsNeeded, int maxThreadsNeeded,
															  Value *eventIdValue, Value *compositionStateValue,
															  size_t chainIndex, vector<size_t> upstreamChainIndices)
{
	Type *intType = IntegerType::get(module->getContext(), 64);
	Type *eventIdType = generateNoEventIdConstant(module)->getType();

	const char *functionName = "vuoScheduleChainWorker";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *dispatchQueueType = getDispatchQueueType(module);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *workerFunctionType = workerFunctionValue->getType();
		PointerType *unsignedLongPointerType = PointerType::get(eventIdType, 0);

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(dispatchQueueType);
		params.push_back(voidPointerType);
		params.push_back(workerFunctionType);
		params.push_back(intType);
		params.push_back(intType);
		params.push_back(eventIdType);
		params.push_back(eventIdType);
		params.push_back(unsignedLongPointerType);
		params.push_back(intType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *minThreadsNeededValue = ConstantInt::get(intType, minThreadsNeeded);
	Value *maxThreadsNeededValue = ConstantInt::get(intType, maxThreadsNeeded);
	Value *chainIndexValue = ConstantInt::get(eventIdType, chainIndex);

	Value *upstreamChainIndicesCountValue = ConstantInt::get(intType, (int)upstreamChainIndices.size());
	Value *upstreamChainIndicesValue = generateMemoryAllocation(module, block, eventIdType, upstreamChainIndices.size());
	for (size_t i = 0; i < upstreamChainIndices.size(); ++i)
	{
		Value *upstreamChainIndexValue = ConstantInt::get(eventIdType, upstreamChainIndices[i]);
		generateSetArrayElement(module, block, upstreamChainIndicesValue, i, upstreamChainIndexValue);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(queueValue);
	args.push_back(contextValue);
	args.push_back(workerFunctionValue);
	args.push_back(minThreadsNeededValue);
	args.push_back(maxThreadsNeededValue);
	args.push_back(eventIdValue);
	args.push_back(chainIndexValue);
	args.push_back(upstreamChainIndicesValue);
	args.push_back(upstreamChainIndicesCountValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGrantThreadsToChain`.
 */
void VuoCompilerCodeGenUtilities::generateGrantThreadsToChain(Module *module, BasicBlock *block,
															  int minThreadsNeeded, int maxThreadsNeeded,
															  Value *eventIdValue, Value *compositionStateValue,
															  size_t chainIndex)
{
	Type *intType = IntegerType::get(module->getContext(), 64);
	Type *eventIdType = generateNoEventIdConstant(module)->getType();

	const char *functionName = "vuoGrantThreadsToChain";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(intType);
		params.push_back(intType);
		params.push_back(eventIdType);
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *minThreadsNeededValue = ConstantInt::get(intType, minThreadsNeeded);
	Value *maxThreadsNeededValue = ConstantInt::get(intType, maxThreadsNeeded);
	Value *chainIndexValue = ConstantInt::get(eventIdType, chainIndex);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(minThreadsNeededValue);
	args.push_back(maxThreadsNeededValue);
	args.push_back(eventIdValue);
	args.push_back(chainIndexValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGrantThreadsToSubcomposition`.
 */
void VuoCompilerCodeGenUtilities::generateGrantThreadsToSubcomposition(Module *module, BasicBlock *block,
																	   Value *eventIdValue, Value *compositionStateValue,
																	   Value *chainIndexValue, Value *subcompositionIdentifierValue)
{
	const char *functionName = "vuoGrantThreadsToSubcomposition";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *eventIdType = generateNoEventIdConstant(module)->getType();
		PointerType *charPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(eventIdType);
		params.push_back(eventIdType);
		params.push_back(charPointerType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(eventIdValue);
	args.push_back(chainIndexValue);
	args.push_back(subcompositionIdentifierValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoReturnThreadsForTriggerWorker`.
 */
void VuoCompilerCodeGenUtilities::generateReturnThreadsForTriggerWorker(Module *module, BasicBlock *block,
																		Value *eventIdValue, Value *compositionStateValue)
{
	const char *functionName = "vuoReturnThreadsForTriggerWorker";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoReturnThreadsForChainWorker`.
 */
void VuoCompilerCodeGenUtilities::generateReturnThreadsForChainWorker(Module *module, BasicBlock *block,
																	  Value *eventIdValue, Value *compositionStateValue,
																	  Value *chainIndexValue)
{
	const char *functionName = "vuoReturnThreadsForChainWorker";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> params;
		params.push_back(compositionStateValue->getType());
		params.push_back(eventIdType);
		params.push_back(eventIdType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(eventIdValue);
	args.push_back(chainIndexValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoLockNodes()`.
 */
void VuoCompilerCodeGenUtilities::generateLockNodes(Module *module, BasicBlock *&block,
													Value *compositionStateValue, const vector<size_t> &nodeIndices, Value *eventIdValue,
													VuoCompilerConstantsCache *constantsCache)
{
	string functionName = "vuoLockNodes";
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		StructType *compositionStateType = getCompositionStateType(module);
		PointerType *pointerToCompositionState = PointerType::get(compositionStateType, 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
		PointerType *pointerToUnsignedLong = PointerType::get(unsignedLongType, 0);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);
		params.push_back(pointerToUnsignedLong);
		params.push_back(unsignedLongType);
		params.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Constant *nodeCountValue = ConstantInt::get(module->getContext(), APInt(64, nodeIndices.size()));
	Constant *nodeIndicesValue = constantsCache->get(nodeIndices);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIndicesValue);
	args.push_back(nodeCountValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoLockNode()`.
 */
void VuoCompilerCodeGenUtilities::generateLockNode(Module *module, BasicBlock *&block,
												   Value *compositionStateValue, size_t nodeIndex, Value *eventIdValue)
{
	Constant *nodeIndexValue = ConstantInt::get(module->getContext(), APInt(64, nodeIndex));
	generateLockNode(module, block, compositionStateValue, nodeIndexValue, eventIdValue);
}

/**
 * Generates a call to `vuoLockNode()`.
 */
void VuoCompilerCodeGenUtilities::generateLockNode(Module *module, BasicBlock *&block,
													  Value *compositionStateValue, Value *nodeIndexValue, Value *eventIdValue)
{
	string functionName = "vuoLockNode";
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		StructType *compositionStateType = getCompositionStateType(module);
		PointerType *pointerToCompositionState = PointerType::get(compositionStateType, 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);
		params.push_back(unsignedLongType);
		params.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIndexValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoUnlockNodes()`.
 */
void VuoCompilerCodeGenUtilities::generateUnlockNodes(Module *module, BasicBlock *block,
													  Value *compositionStateValue, const vector<size_t> &nodeIndices,
													  VuoCompilerConstantsCache *constantsCache)
{
	string functionName = "vuoUnlockNodes";
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		StructType *compositionStateType = getCompositionStateType(module);
		PointerType *pointerToCompositionState = PointerType::get(compositionStateType, 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
		PointerType *pointerToUnsignedLong = PointerType::get(unsignedLongType, 0);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);
		params.push_back(pointerToUnsignedLong);
		params.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Constant *nodeCountValue = ConstantInt::get(module->getContext(), APInt(64, nodeIndices.size()));
	Constant *nodeIndicesValue = constantsCache->get(nodeIndices);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIndicesValue);
	args.push_back(nodeCountValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoUnlockNode()`.
 */
void VuoCompilerCodeGenUtilities::generateUnlockNode(Module *module, BasicBlock *block,
													 Value *compositionStateValue, size_t nodeIndex)
{
	Constant *nodeIndexValue = ConstantInt::get(module->getContext(), APInt(64, nodeIndex));
	generateUnlockNode(module, block, compositionStateValue, nodeIndexValue);
}

/**
 * Generates a call to `vuoUnlockNode()`.
 */
void VuoCompilerCodeGenUtilities::generateUnlockNode(Module *module, BasicBlock *block,
													 Value *compositionStateValue, Value *nodeIndexValue)
{
	string functionName = "vuoUnlockNode";
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		StructType *compositionStateType = getCompositionStateType(module);
		PointerType *pointerToCompositionState = PointerType::get(compositionStateType, 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToCompositionState);
		params.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIndexValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that sets the array element at the given index.
 */
void VuoCompilerCodeGenUtilities::generateSetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex, Value *value)
{
	Value *memberPointer = generateGetArrayElementVariable(module, block, arrayValue, elementIndex);
	new StoreInst(value, memberPointer, false, block);
}

/**
 * Generates code that gets the array element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex)
{
	Value *memberPointer = generateGetArrayElementVariable(module, block, arrayValue, elementIndex);
	return new LoadInst(memberPointer, "", block);
}

/**
 * Generates code that gets the array element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetArrayElement(Module *module, BasicBlock *block, Value *arrayValue, Value *elementIndexValue)
{
	Value *memberPointer = generateGetArrayElementVariable(module, block, arrayValue, elementIndexValue);
	return new LoadInst(memberPointer, "", block);
}

/**
 * Generates code that gets the address of the array element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex)
{
	ConstantInt *indexValue = ConstantInt::get(module->getContext(), APInt(32, elementIndex));
	return generateGetArrayElementVariable(module, block, arrayValue, indexValue);
}

/**
 * Generates code that gets the address of the array element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, Value *elementIndexValue)
{
	return GetElementPtrInst::Create(arrayValue, elementIndexValue, "", block);
}

/**
 * Generates code that sets the struct element at the given index.
 */
void VuoCompilerCodeGenUtilities::generateSetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex, Value *value)
{
	Value *memberPointer = generateGetStructPointerElementVariable(module, block, structPointer, elementIndex);
	new StoreInst(value, memberPointer, false, block);
}

/**
 * Generates code that gets the struct element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetStructPointerElement(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex)
{
	Value *memberPointer = generateGetStructPointerElementVariable(module, block, structPointer, elementIndex);
	return new LoadInst(memberPointer, "", block);
}

/**
 * Generates code that gets the address of the struct element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetStructPointerElementVariable(Module *module, BasicBlock *block, Value *structPointer, size_t elementIndex)
{
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
	ConstantInt *indexValue = ConstantInt::get(module->getContext(), APInt(32, elementIndex));

	vector<Value *> memberIndices;
	memberIndices.push_back(zeroValue);
	memberIndices.push_back(indexValue);
	return GetElementPtrInst::Create(structPointer, memberIndices, "", block);
}

/**
 * Generates code that creates a pointer to @c value (on the stack), and returns the pointer.
 */
Value * VuoCompilerCodeGenUtilities::generatePointerToValue(BasicBlock *block, Value *value)
{
	AllocaInst *variable = new AllocaInst(value->getType(), "", block);
	new StoreInst(value, variable, false, block);
	return variable;
}

/**
 * Generates code that creates a global string variable.
 *
 * @param module The module in which to generate code.
 * @param stringValue The string initializer for the global variable.
 * @param globalVariableName The name to give to the global variable.
 * @return The address of the string.
 */
Constant * VuoCompilerCodeGenUtilities::generatePointerToConstantString(Module *module, string stringValue, string globalVariableName)
{
	Constant *array = ConstantDataArray::getString(module->getContext(), stringValue);
	ArrayType *arrayType = ArrayType::get(IntegerType::get(module->getContext(), 8), stringValue.length() + 1);
	GlobalVariable *global = new GlobalVariable(*module, arrayType, true, GlobalValue::InternalLinkage, 0, "");
	global->setInitializer(array);
	global->setName(globalVariableName);

	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
	vector<Constant *> pointerIndices;
	pointerIndices.push_back(zeroValue);
	pointerIndices.push_back(zeroValue);
	Constant *pointer = ConstantExpr::getGetElementPtr(global, pointerIndices);

	return pointer;
}

/**
 * Generates code that creates a global array-of-strings variable.
 *
 * @param module The module in which to generate code.
 * @param stringValues The strings to be placed in the array.
 * @param globalVariableName The name to give to the global variable.
 * @return The address of the array.
 */
Constant * VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfStrings(Module *module, vector<string> stringValues, string globalVariableName)
{
	vector<Constant *> arrayElements;
	for (vector<string>::iterator i = stringValues.begin(); i != stringValues.end(); ++i)
	{
		Constant *stringPointer = generatePointerToConstantString(module, *i, ".str");
		arrayElements.push_back(stringPointer);
	}

	ArrayType *arrayType = ArrayType::get(PointerType::get(IntegerType::get(module->getContext(), 8), 0), arrayElements.size());
	GlobalVariable *global = new GlobalVariable(*module, arrayType, false, GlobalValue::ExternalLinkage, 0, globalVariableName);
	Constant *arrayConstant = ConstantArray::get(arrayType, arrayElements);
	global->setInitializer(arrayConstant);

	ConstantInt *zeroi64Constant = ConstantInt::get(module->getContext(), APInt(64, 0));
	vector<Constant *> pointerIndices;
	pointerIndices.push_back(zeroi64Constant);
	pointerIndices.push_back(zeroi64Constant);
	Constant *pointer = ConstantExpr::getGetElementPtr(global, pointerIndices);

	return pointer;
}

/**
 * Generates a global array-of-unsigned-longs variable.
 *
 * @param module The module in which to generate code.
 * @param values The values to be placed in the array.
 * @param globalVariableName The name to give to the global variable.
 * @return The address of the array.
 */
Constant * VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfUnsignedLongs(Module *module, const vector<unsigned long> &values, string globalVariableName)
{
	vector<Constant *> arrayElements;
	for (unsigned long value : values)
	{
		Constant *valueConstant = ConstantInt::get(module->getContext(), APInt(64, value));
		arrayElements.push_back(valueConstant);
	}

	ArrayType *arrayType = ArrayType::get(IntegerType::get(module->getContext(), 64), arrayElements.size());
	GlobalVariable *global = new GlobalVariable(*module, arrayType, false, GlobalValue::ExternalLinkage, 0, globalVariableName);
	Constant *arrayConstant = ConstantArray::get(arrayType, arrayElements);
	global->setInitializer(arrayConstant);

	ConstantInt *zeroi64Constant = ConstantInt::get(module->getContext(), APInt(64, 0));
	vector<Constant *> pointerIndices;
	pointerIndices.push_back(zeroi64Constant);
	pointerIndices.push_back(zeroi64Constant);
	Constant *pointer = ConstantExpr::getGetElementPtr(global, pointerIndices);

	return pointer;
}

/**
 * Generates a series of if-else statements for testing if an input string is equal to any of a
 * set of constant strings, and executing the corresponding block of code if it is.
 *
 * Assumes that none of the blocks passed to this function contain branch instructions.
 * (This function appends branch instructions to all but the final block.)
 *
 * Example:
 *
 * if (! strcmp(inputString, string0))
 *   // blockForString[string0]
 * else if (! strcmp(inputString, string1))
 *   // blockForString[string1]
 * ...
 *
 * @param module The module in which to generate code.
 * @param function The function in which to generate code.
 * @param initialBlock The block to which the first if-statement will be appended.
 * @param finalBlock The block following the if-else statements.
 * @param inputStringValue The string to compare in each if-statement.
 * @param blocksForString For each key string, the first block and last block to execute if the input string matches that string.
 *		The caller is responsible for branching (directly or indirectly) from the first to the last block.
 * @param constantsCache The cache of LLVM constants used to generate string values.
 */
void VuoCompilerCodeGenUtilities::generateStringMatchingCode(Module *module, Function *function,
															 BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue,
															 map<string, pair<BasicBlock *, BasicBlock *> > blocksForString,
															 VuoCompilerConstantsCache *constantsCache)
{
	Function *strcmpFunction = getStrcmpFunction(module);
	BasicBlock *currentBlock = initialBlock;

	for (map<string, pair<BasicBlock *, BasicBlock *> >::iterator i = blocksForString.begin(); i != blocksForString.end(); ++i)
	{
		string currentString = i->first;
		BasicBlock *firstTrueBlock = i->second.first;
		BasicBlock *lastTrueBlock = i->second.second;

		Constant *currentStringValue = constantsCache->get(currentString);

		vector<Value *> strcmpArgs;
		strcmpArgs.push_back(inputStringValue);
		strcmpArgs.push_back(currentStringValue);
		CallInst *strcmpCall = CallInst::Create(strcmpFunction, strcmpArgs, "", currentBlock);

		ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
		ICmpInst *strcmpEqualsZero = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, strcmpCall, zeroValue, "");
		BasicBlock *falseBlock = BasicBlock::Create(module->getContext(), string("strcmp-") + currentString, function, 0);
		BranchInst::Create(firstTrueBlock, falseBlock, strcmpEqualsZero, currentBlock);

		BranchInst::Create(finalBlock, lastTrueBlock);

		currentBlock = falseBlock;
	}

	BranchInst::Create(finalBlock, currentBlock);
}

/**
 * Generates a series of if-else statements for testing if an input index is equal to any of a
 * set of indices, and executing the corresponding block of code if it is.
 *
 * Assumes that none of the blocks passed to this function contain branch instructions.
 * (This function appends branch instructions to all but the final block.)
 *
 * Example:
 *
 * if (inputIndex == 0)
 *   // blockForIndex[0]
 * else if (inputIndex == 1)
 *   // blockForIndex[1]
 * ...
 *
 * @param module The module in which to generate code.
 * @param function The function in which to generate code.
 * @param initialBlock The block to which the first if-statement will be appended.
 * @param finalBlock The block following the if-else statements.
 * @param inputIndexValue The index to compare in each if-statement.
 * @param blocksForIndex For each index, the first block and last block to execute if the input index matches that index.
 *		The caller is responsible for branching (directly or indirectly) from the first to the last block.
 */
void VuoCompilerCodeGenUtilities::generateIndexMatchingCode(Module *module, Function *function,
															BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputIndexValue,
															vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex)
{
	BasicBlock *currentBlock = initialBlock;

	for (size_t i = 0; i < blocksForIndex.size(); ++i)
	{
		BasicBlock *firstTrueBlock = blocksForIndex[i].first;
		BasicBlock *lastTrueBlock = blocksForIndex[i].second;

		Constant *currentIndexValue = ConstantInt::get(inputIndexValue->getType(), i);
		ICmpInst *indexEqualsCurrent = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, inputIndexValue, currentIndexValue, "");
		BasicBlock *falseBlock = BasicBlock::Create(module->getContext(), "", function, 0);
		BranchInst::Create(firstTrueBlock, falseBlock, indexEqualsCurrent, currentBlock);

		BranchInst::Create(finalBlock, lastTrueBlock);

		currentBlock = falseBlock;
	}

	BranchInst::Create(finalBlock, currentBlock);
}

/**
 * Generates code that allocates a buffer to hold the composite string, then calls @c snprintf to combine the format string
 * and replacement values into the composite string.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param formatString The format string to be passed to @c snprintf.
 * @param replacementValues The replacement values to be passed to @c snprintf.
 * @param constantsCache The cache of LLVM constants used to generate string values.
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues,
															 VuoCompilerConstantsCache *constantsCache)
{
	Function *snprintfFunction = getSnprintfFunction(module);

	Type *charType = IntegerType::get(module->getContext(), 8);
	PointerType *pointerToCharType = PointerType::get(charType, 0);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToCharType);
	ConstantInt *zeroValue64 = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *oneValue64 = ConstantInt::get(module->getContext(), APInt(64, 1));

	// int bufferLength = snprintf(NULL, 0, format, ...) + 1;
	Constant *formatStringValue = constantsCache->get(formatString);
	vector<Value *> snprintfArgs;
	snprintfArgs.push_back(nullValue);
	snprintfArgs.push_back(zeroValue64);
	snprintfArgs.push_back(formatStringValue);
	for (vector<Value *>::iterator i = replacementValues.begin(); i != replacementValues.end(); ++i)
		snprintfArgs.push_back(*i);
	Value *bufferLengthValue32 = CallInst::Create(snprintfFunction, snprintfArgs, "", block);
	Value *bufferLengthValue = new SExtInst(bufferLengthValue32, IntegerType::get(module->getContext(), 64), "", block);
	bufferLengthValue = BinaryOperator::Create(Instruction::Add, bufferLengthValue, oneValue64, "", block);

	// char *buffer = malloc(bufferLength);
	AllocaInst *bufferVariable = new AllocaInst(pointerToCharType, "buffer", block);
	Value *bufferValue = generateMemoryAllocation(module, block, charType, bufferLengthValue);
	new StoreInst(bufferValue, bufferVariable, false, block);

	// snprintf(buffer, bufferLength, format, ...);
	snprintfArgs[0] = bufferValue;
	snprintfArgs[1] = bufferLengthValue;
	CallInst::Create(snprintfFunction, snprintfArgs, "", block);
	return bufferValue;
}

/**
 * Generates code that allocates a buffer to hold the composite string, then concatenates each member of @c stringsToConcatenate
 * into the composite string.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param stringsToConcatenate The strings to concatenate. Each element should be a value of type pointer-to-char.
 * @param constantsCache The cache of LLVM constants used to generate string values.
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate,
																 VuoCompilerConstantsCache *constantsCache)
{
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	if (stringsToConcatenate.empty())
	{
		return constantsCache->get("");
	}
	else if (stringsToConcatenate.size() == 2)
	{
		const char *functionName = "vuoConcatenateStrings2";
		Function *function = module->getFunction(functionName);
		if (! function)
		{
			vector<Type *> params;
			params.push_back(pointerToCharType);
			params.push_back(pointerToCharType);

			FunctionType *functionType = FunctionType::get(pointerToCharType, params, false);
			function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
		}

		vector<Value *> args;
		args.push_back(stringsToConcatenate[0]);
		args.push_back(stringsToConcatenate[1]);
		return CallInst::Create(function, args, "", block);
	}
	else if (stringsToConcatenate.size() == 3)
	{
		const char *functionName = "vuoConcatenateStrings3";
		Function *function = module->getFunction(functionName);
		if (! function)
		{
			vector<Type *> params;
			params.push_back(pointerToCharType);
			params.push_back(pointerToCharType);
			params.push_back(pointerToCharType);

			FunctionType *functionType = FunctionType::get(pointerToCharType, params, false);
			function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
		}

		vector<Value *> args;
		args.push_back(stringsToConcatenate[0]);
		args.push_back(stringsToConcatenate[1]);
		args.push_back(stringsToConcatenate[2]);
		return CallInst::Create(function, args, "", block);
	}
	else
	{
		IntegerType *sizeType = IntegerType::get(module->getContext(), 64);

		const char *functionName = "vuoConcatenateStrings";
		Function *function = module->getFunction(functionName);
		if (! function)
		{
			vector<Type *> params;
			params.push_back(PointerType::get(pointerToCharType, 0));
			params.push_back(sizeType);

			FunctionType *functionType = FunctionType::get(pointerToCharType, params, false);
			function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
		}

		Value *stringsValue = generateMemoryAllocation(module, block, pointerToCharType, stringsToConcatenate.size());
		for (size_t i = 0; i < stringsToConcatenate.size(); ++i)
			generateSetArrayElement(module, block, stringsValue, i, stringsToConcatenate[i]);

		vector<Value *> args;
		args.push_back(stringsValue);
		args.push_back(ConstantInt::get(sizeType, stringsToConcatenate.size()));
		Value *concatenatedStringValue = CallInst::Create(function, args, "", block);

		generateFreeCall(module, block, stringsValue);

		return concatenatedStringValue;
	}
}

/**
 * Generates code that dynamically allocates memory for an array.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param elementType The type of each array element.
 * @param elementCount The number of array elements.
 * @return A value pointing to the address of the memory.
 */
Value * VuoCompilerCodeGenUtilities::generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, int elementCount)
{
	Value *elementCountValue = ConstantInt::get(module->getContext(), APInt(64, elementCount));
	return generateMemoryAllocation(module, block, elementType, elementCountValue);
}

/**
 * Generates code that dynamically allocates memory for an array.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param elementType The type of each array element.
 * @param elementCountValue The number of array elements. Assumed to have type integer.
 * @return A value pointing to the address of the memory.
 */
Value * VuoCompilerCodeGenUtilities::generateMemoryAllocation(Module *module, BasicBlock *block, Type *elementType, Value *elementCountValue)
{
	Constant *elementBytesValue = ConstantExpr::getSizeOf(elementType);
	Value *elementCountValue64;
	if (static_cast<IntegerType *>(elementCountValue->getType())->getBitWidth() < 64)
		elementCountValue64 = new SExtInst(elementCountValue, IntegerType::get(module->getContext(), 64), "", block);
	else
		elementCountValue64 = elementCountValue;

	BinaryOperator *bytesValue = BinaryOperator::Create(Instruction::Mul, elementBytesValue, elementCountValue64, "", block);

	Function *mallocFunction = VuoCompilerCodeGenUtilities::getMallocFunction(module);
	CallInst *mallocReturnValue = CallInst::Create(mallocFunction, bytesValue, "", block);

	Type *elementPointerType = PointerType::get(elementType, 0);
	CastInst *mallocReturnValueCasted = new BitCastInst(mallocReturnValue, elementPointerType, "", block);

	return mallocReturnValueCasted;
}

/**
 * Generates code to cast the value to the desired type (if it doesn't already have that type).
 *
 * @return A value of type @c typeToCastTo (which may be @c valueToCast).
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCast(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	if (valueToCast->getType() == typeToCastTo)
		return valueToCast;

	if (valueToCast->getType()->isIntegerTy() && typeToCastTo->isIntegerTy())
		return CastInst::CreateIntegerCast(valueToCast, typeToCastTo, true, "", block);
	else if (valueToCast->getType()->isIntegerTy() && typeToCastTo->isPointerTy())
		return generateTypeCastFromIntegerToPointer(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isFloatingPointTy() && typeToCastTo->isPointerTy())
		return generateTypeCastFromFloatingPointToPointer(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isPointerTy() && typeToCastTo->isIntegerTy())
		return generateTypeCastFromPointerToInteger(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isPointerTy() && typeToCastTo->isFloatingPointTy())
		return generateTypeCastFromPointerToFloatingPoint(module, block, valueToCast, typeToCastTo);
	else if (typeToCastTo->isStructTy())
		return generateTypeCastFromLoweredTypeToStruct(block, valueToCast, typeToCastTo);
	else if (typeToCastTo->isVectorTy())
		return generateTypeCastFromLoweredTypeToVector(block, valueToCast, typeToCastTo);
	else
		return new BitCastInst(valueToCast, typeToCastTo, "", block);
}

/**
 * Generates a call to @c llvm.var.annotation, which annotates a value with a string.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param value The value to be annotated.
 * @param annotation The annotation to be associated with the value.
 * @param fileName The name of the file for the module (or an empty string if no file).
 * @param lineNumber The line number in the file for the module (or 0 if no file).
 * @param constantsCache The cache of LLVM constants used to generate string values.
 */
void VuoCompilerCodeGenUtilities::generateAnnotation(Module *module, BasicBlock *block, Value *value,
													 string annotation, string fileName, unsigned int lineNumber,
													 VuoCompilerConstantsCache *constantsCache)
{
	// Set variable names expected by VuoCompilerBitcodeParser::getAnnotatedArguments().
	string valueName = value->getName();
	string variableName = valueName + ".addr";
	string valueAsVoidPointerName = valueName + ".addr1";
	string annotationVariableName = valueName + "__annotation";

	Function *annotateFunction = getAnnotateFunction(module);
	vector<Value *> annotateFunctionArgs;

	Value *variable = generatePointerToValue(block, value);
	variable->setName(variableName);
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *valueAsVoidPointer = generateTypeCast(module, block, variable, voidPointerType);
	valueAsVoidPointer->setName(valueAsVoidPointerName);
	annotateFunctionArgs.push_back(valueAsVoidPointer);

	Constant *annotationPointer = generatePointerToConstantString(module, annotation, annotationVariableName);
	annotateFunctionArgs.push_back(annotationPointer);

	Constant *fileNamePointer = constantsCache->get(fileName);
	annotateFunctionArgs.push_back(fileNamePointer);

	ConstantInt *lineNumberValue = ConstantInt::get(module->getContext(), APInt(32, lineNumber));
	annotateFunctionArgs.push_back(lineNumberValue);

	CallInst::Create(annotateFunction, annotateFunctionArgs, "", block);
}

/**
 * Generates code equivalent to the VuoModuleMetadata macro.
 */
void VuoCompilerCodeGenUtilities::generateModuleMetadata(Module *module, string metadata, string moduleKey)
{
	// const char *moduleDetails = ...;
	Constant *moduleDetailsValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, metadata, ".str");  // VuoCompilerBitcodeParser::resolveGlobalToConst requires that the variable have a name
	Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	string variableName = "moduleDetails";
	if (! moduleKey.empty())
		variableName = VuoStringUtilities::prefixSymbolName("moduleDetails", moduleKey);
	GlobalVariable *moduleDetailsVariable = new GlobalVariable(*module, pointerToCharType, false, GlobalValue::ExternalLinkage, 0, variableName);
	moduleDetailsVariable->setInitializer(moduleDetailsValue);
}

/**
 * Generates code to cast the integer value to the desired pointer type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromIntegerToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	unsigned pointerBitWidth = 64;  /// @todo Get pointer size for target architecture. - https://b33p.net/kosada/node/3916
	IntegerType *integerWithPointerBitWidth = IntegerType::get(module->getContext(), pointerBitWidth);
	CastInst *valueAsIntegerWithPointerBitWidth = CastInst::CreateIntegerCast(valueToCast, integerWithPointerBitWidth, true, "", block);
	return new IntToPtrInst(valueAsIntegerWithPointerBitWidth, typeToCastTo, "", block);
}

/**
 * Generates code to cast the floating-point value to the desired pointer type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromFloatingPointToPointer(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	unsigned pointerBitWidth = 64;  /// @todo Get pointer size for target architecture. - https://b33p.net/kosada/node/3916
	Type *floatingPointWithPointerBitWidth = Type::getDoubleTy(module->getContext());
	IntegerType *integerWithPointerBitWidth = IntegerType::get(module->getContext(), pointerBitWidth);
	CastInst *valueAsFloatingPointWithPointerBitWidth = CastInst::CreateFPCast(valueToCast, floatingPointWithPointerBitWidth, "", block);
	Value *valueAsIntegerWithPointerBitWidth = new BitCastInst(valueAsFloatingPointWithPointerBitWidth, integerWithPointerBitWidth, "", block);
	return generateTypeCastFromIntegerToPointer(module, block, valueAsIntegerWithPointerBitWidth, typeToCastTo);
}

/**
 * Generates code to cast the pointer value to the desired integer type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromPointerToInteger(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	unsigned pointerBitWidth = 64;  /// @todo Get pointer size for target architecture. - https://b33p.net/kosada/node/3916
	IntegerType *integerWithPointerBitWidth = IntegerType::get(module->getContext(), pointerBitWidth);
	CastInst *valueAsIntegerWithPointerBitWidth = new PtrToIntInst(valueToCast, integerWithPointerBitWidth, "", block);
	return CastInst::CreateIntegerCast(valueAsIntegerWithPointerBitWidth, typeToCastTo, true, "", block);
}

/**
 * Generates code to cast the pointer value to the desired floating-point type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromPointerToFloatingPoint(Module *module, BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	unsigned pointerBitWidth = 64;  /// @todo Get pointer size for target architecture. - https://b33p.net/kosada/node/3916
	Type *floatingPointWithPointerBitWidth = Type::getDoubleTy(module->getContext());
	IntegerType *integerWithPointerBitWidth = IntegerType::get(module->getContext(), pointerBitWidth);
	Value *valueAsIntegerWithPointerBitWidth = generateTypeCastFromPointerToInteger(module, block, valueToCast, integerWithPointerBitWidth);
	CastInst *valueAsFloatingPointWithPointerBitWidth = new BitCastInst(valueAsIntegerWithPointerBitWidth, floatingPointWithPointerBitWidth, "", block);
	return CastInst::CreateFPCast(valueAsFloatingPointWithPointerBitWidth, typeToCastTo, "", block);
}

/**
 * Generates code to cast the value, whose type was lowered from a struct, to its original struct type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromLoweredTypeToStruct(BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	Value *originalValueToCast = valueToCast;

	if (valueToCast->getType()->isVectorTy() || valueToCast->getType()->isDoubleTy())
	{
		// Struct was lowered to a vector or double.
		PointerType *pointerToLoweredType = PointerType::get(valueToCast->getType(), 0);

		AllocaInst *structVariable = new AllocaInst(typeToCastTo, "", block);
		CastInst *structVariableAsLoweredType = new BitCastInst(structVariable, pointerToLoweredType, "", block);
		new StoreInst(valueToCast, structVariableAsLoweredType, false, block);
		return new LoadInst(structVariable, "", false, block);
	}

	if (valueToCast->getType()->isPointerTy())
	{
		// Struct was passed by value.
		valueToCast = new LoadInst(valueToCast, "", false, block);
		if (valueToCast->getType() == typeToCastTo)
			return valueToCast;
	}

	if (valueToCast->getType()->isStructTy())
	{
		// Struct types don't match because they were loaded from different modules.
		StructType *structTypeOfValueToCast = static_cast<StructType *>(valueToCast->getType());
		PointerType *pointerToTypeToCastTo = PointerType::get(typeToCastTo, 0);

		AllocaInst *otherStructVariable = new AllocaInst(structTypeOfValueToCast, "", block);
		new StoreInst(valueToCast, otherStructVariable, false, block);
		CastInst *otherStructAsTypeToCastTo = new BitCastInst(otherStructVariable, pointerToTypeToCastTo, "", block);
		return new LoadInst(otherStructAsTypeToCastTo, "", false, block);
	}

	VUserLog("Error: Couldn't cast from lowered type to struct.");
	originalValueToCast->getType()->dump();  fprintf(stderr, "\n");
	typeToCastTo->dump();  fprintf(stderr, "\n");
	return originalValueToCast;
}

/**
 * Generates code to cast the value, whose type was lowered from a vector, to its original vector type.
 *
 * @return A value of type @c typeToCastTo.
 */
Value * VuoCompilerCodeGenUtilities::generateTypeCastFromLoweredTypeToVector(BasicBlock *block, Value *valueToCast, Type *typeToCastTo)
{
	if (typeToCastTo->isVectorTy() && static_cast<VectorType *>(typeToCastTo)->getElementType()->isFloatTy())
	{
		uint64_t elementCount = static_cast<VectorType *>(typeToCastTo)->getNumElements();
		if (elementCount == 2 && valueToCast->getType()->isDoubleTy())
		{
			// VuoPoint2d — Vector of 2 floats was lowered to a double.
			PointerType *pointerToDoubleType = PointerType::get(valueToCast->getType(), 0);

			AllocaInst *vectorVariable = new AllocaInst(typeToCastTo, "", block);
			CastInst *dstVectorVariableAsDouble = new BitCastInst(vectorVariable, pointerToDoubleType, "", block);
			new StoreInst(valueToCast, dstVectorVariableAsDouble, false, block);
			return new LoadInst(vectorVariable, "", false, block);
		}
		else if (elementCount == 3 && valueToCast->getType()->isVectorTy() &&
				 static_cast<VectorType *>(valueToCast->getType())->getNumElements() == 2 &&
				 static_cast<VectorType *>(valueToCast->getType())->getElementType()->isDoubleTy())
		{
			// VuoPoint3d — Vector of 3 floats was lowered to a vector of 2 doubles.
			PointerType *pointerToDoubleVectorType = PointerType::get(valueToCast->getType(), 0);

			AllocaInst *floatVectorVariable = new AllocaInst(typeToCastTo, "", block);
			CastInst *floatVectorVariableAsDoubleVector = new BitCastInst(floatVectorVariable, pointerToDoubleVectorType, "", block);
			new StoreInst(valueToCast, floatVectorVariableAsDoubleVector, false, block);
			return new LoadInst(floatVectorVariable, "", false, block);
		}
	}

	VUserLog("Error: Couldn't cast from lowered type to vector.");
	valueToCast->getType()->dump();  fprintf(stderr, "\n");
	typeToCastTo->dump();  fprintf(stderr, "\n");
	return valueToCast;
}

/**
 * Generates code to register @a argument.
 */
void VuoCompilerCodeGenUtilities::generateRegisterCall(Module *module, BasicBlock *block, Value *argument, Function *freeFunction)
{
	Function *function = getVuoRegisterFunction(module);

	Type *voidPointerType = function->getFunctionType()->getParamType(0);
	Type *pointerToCharType = function->getFunctionType()->getParamType(2);
	Type *intType = function->getFunctionType()->getParamType(3);

	ConstantPointerNull *nullValue = ConstantPointerNull::get( static_cast<PointerType *>(pointerToCharType) );
	Constant *zeroValue = ConstantInt::get(intType, 0);

	vector<Value *> args;

	BitCastInst *argumentAsVoidPointer = new BitCastInst(argument, voidPointerType, "", block);
	args.push_back(argumentAsVoidPointer);

	args.push_back(freeFunction);

	args.push_back(nullValue);
	args.push_back(zeroValue);
	args.push_back(nullValue);
	args.push_back(nullValue);

	CallInst::Create(function, args, "", block);
}

/**
 * Generates any code needed to retain @c argument.
 */
void VuoCompilerCodeGenUtilities::generateRetainCall(Module *module, BasicBlock *block, Value *argument)
{
	generateRetainOrReleaseCall(module, block, argument, true);
}

/**
 * Generates any code needed to release @c argument.
 */
void VuoCompilerCodeGenUtilities::generateReleaseCall(Module *module, BasicBlock *block, Value *argument)
{
	generateRetainOrReleaseCall(module, block, argument, false);
}

/**
 * Generates any code needed to retain or release @c argument.
 *
 * If @c argument has a primitive, non-pointer type, no code is generated.
 *
 * If @c argument has a pointer type, @c VuoRetain or @c VuoRelease is called on @c argument.
 *
 * If @c argument has a struct type, this function is called recursively on each member.
 */
void VuoCompilerCodeGenUtilities::generateRetainOrReleaseCall(Module *module, BasicBlock *block, Value *argument, bool isRetain)
{
	if (argument->getType()->isPointerTy())
	{
		Function *function = (isRetain ? getVuoRetainFunction(module) : getVuoReleaseFunction(module));
		Type *voidPointerType = function->getFunctionType()->getParamType(0);

		vector<Value *> retainOrReleaseArgs;
		retainOrReleaseArgs.push_back(VuoCompilerCodeGenUtilities::generateTypeCast(module, block, argument, voidPointerType));
		CallInst::Create(function, retainOrReleaseArgs, "", block);
	}
	else if (argument->getType()->isStructTy())
	{
		AllocaInst *structPointer = new AllocaInst(argument->getType(), "", block);
		new StoreInst(argument, structPointer, false, block);

		int numElements = static_cast<StructType *>(argument->getType())->getNumElements();
		for (unsigned i = 0; i < numElements; ++i)
		{
			Value *member = generateGetStructPointerElement(module, block, structPointer, i);
			generateRetainOrReleaseCall(module, block, member, isRetain);
		}
	}
}

/**
 * Returns true if VuoCompilerCodeGenUtilities::generateRetainCall and VuoCompilerCodeGenUtilities::generateReleaseCall
 * would generate any code for @a type.
 */
bool VuoCompilerCodeGenUtilities::isRetainOrReleaseNeeded(Type *type)
{
	return type->isPointerTy() || type->isStructTy();
}

/**
 * Generates a call to free @a argument (after casting it to a void pointer).
 */
void VuoCompilerCodeGenUtilities::generateFreeCall(Module *module, BasicBlock *block, Value *argument)
{
	Function *freeFunction = getFreeFunction(module);
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *argumentAsVoidPointer = new BitCastInst(argument, voidPointerType, "", block);
	CallInst::Create(freeFunction, argumentAsVoidPointer, "", block);
}

/**
 * Generates a call to `json_object_put()`.
 */
void VuoCompilerCodeGenUtilities::generateJsonObjectPut(Module *module, BasicBlock *block, Value *jsonObjectValue)
{
	Function *function = getJsonObjectPutFunction(module);
	CallInst::Create(function, jsonObjectValue, "", block);
}

/**
 * Generates code that checks if @a valueToCheck (assumed to have a pointer type) is null.
 * This function initializes @a nullBlock and @a notNullBlock and sets them up to be the blocks executed depending on the result.
 */
void VuoCompilerCodeGenUtilities::generateNullCheck(Module *module, Function *function, Value *valueToCheck,
													BasicBlock *initialBlock, BasicBlock *&nullBlock, BasicBlock *&notNullBlock)
{
	nullBlock = BasicBlock::Create(module->getContext(), "null", function, NULL);
	notNullBlock = BasicBlock::Create(module->getContext(), "notNull", function, NULL);

	Value *nullValue = ConstantPointerNull::get( static_cast<PointerType *>(valueToCheck->getType()) );
	ICmpInst *isNotNull = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, valueToCheck, nullValue, "");
	BranchInst::Create(notNullBlock, nullBlock, isNotNull, initialBlock);
}

/**
 * Generates code that creates a string representation of the given value.
 */
Value * VuoCompilerCodeGenUtilities::generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize,
														   VuoCompilerConstantsCache *constantsCache)
{
	if (valueToSerialize->getType()->isPointerTy())
	{
		vector<Value *> replacementValues;
		replacementValues.push_back(valueToSerialize);
		return generateFormattedString(module, block, "%lx", replacementValues, constantsCache);
	}
	else
	{
		/// @todo Handle other primitive types and structs (https://b33p.net/kosada/node/3942)
		VUserLog("Error: Couldn't serialize non-pointer value.");
		return NULL;
	}
}

/**
 * Generates code that creates a value of the given type from the given string representation,
 * and stores it in the destination variable.
 */
void VuoCompilerCodeGenUtilities::generateUnserialization(Module *module, BasicBlock *block, Value *stringToUnserialize, Value *destinationVariable,
														  VuoCompilerConstantsCache *constantsCache)
{
	if (destinationVariable->getType()->isPointerTy())
	{
		// sscanf(stringToUnserialize, "%lx", destinationPointer);
		Value *formatString = constantsCache->get("%lx");
		Function *sscanfFunction = getSscanfFunction(module);
		vector<Value *> sscanfArgs;
		sscanfArgs.push_back(stringToUnserialize);
		sscanfArgs.push_back(formatString);
		sscanfArgs.push_back(destinationVariable);
		CallInst::Create(sscanfFunction, sscanfArgs, "", block);
	}
	else
	{
		/// @todo Handle other primitive types and structs (https://b33p.net/kosada/node/3942)
	}
}

/**
 * Generates code that gets the return value of `vuoIsPaused()` as a comparison value.
 */
ICmpInst * VuoCompilerCodeGenUtilities::generateIsPausedComparison(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	Type *boolType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoIsPaused";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(boolType, compositionStateValue->getType(), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	CallInst *isPausedValue = CallInst::Create(function, compositionStateValue, "", block);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	return new ICmpInst(*block, ICmpInst::ICMP_NE, isPausedValue, falseValue, "");
}

/**
 * Generates a call to `vuoSendNodeExecutionStarted()`.
 */
void VuoCompilerCodeGenUtilities::generateSendNodeExecutionStarted(Module *module, BasicBlock *block,
																   Value *compositionStateValue, Value *nodeIdentifierValue)
{
	const char *functionName = "vuoSendNodeExecutionStarted";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIdentifierValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendNodeExecutionFinished()`.
 */
void VuoCompilerCodeGenUtilities::generateSendNodeExecutionFinished(Module *module, BasicBlock *block,
																	Value *compositionStateValue, Value *nodeIdentifierValue)
{
	const char *functionName = "vuoSendNodeExecutionFinished";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIdentifierValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendInputPortsUpdated()`.
 */
void VuoCompilerCodeGenUtilities::generateSendInputPortsUpdated(Module *module, BasicBlock *block,
																Value *compositionStateValue, Value *portIdentifierValue,
																bool receivedEvent, bool receivedData,
																Value *portDataSummaryValue)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 1);
	Value *receivedEventValue = ConstantInt::get(boolType, receivedEvent ? 1 : 0);
	Value *receivedDataValue = ConstantInt::get(boolType, receivedData ? 1 : 0);
	generateSendInputPortsUpdated(module, block, compositionStateValue, portIdentifierValue, receivedEventValue, receivedDataValue, portDataSummaryValue);
}

/**
 * Generates a call to `vuoSendInputPortsUpdated()`.
 */
void VuoCompilerCodeGenUtilities::generateSendInputPortsUpdated(Module *module, BasicBlock *block,
																Value *compositionStateValue, Value *portIdentifierValue,
																Value *receivedEventValue, Value *receivedDataValue,
																Value *portDataSummaryValue)
{
	const char *functionName = "vuoSendInputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(receivedEventValue);
	args.push_back(receivedDataValue);
	args.push_back(portDataSummaryValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendOutputPortsUpdated()`.
 */
void VuoCompilerCodeGenUtilities::generateSendOutputPortsUpdated(Module *module, BasicBlock *block,
																 Value *compositionStateValue, Value *portIdentifierValue,
																 Value *sentEventValue, Value *sentDataValue,
																 Value *portDataSummaryValue)
{
	const char *functionName = "vuoSendOutputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(sentEventValue);
	args.push_back(sentDataValue);
	args.push_back(portDataSummaryValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendPublishedOutputPortsUpdated()`.
 */
void VuoCompilerCodeGenUtilities::generateSendPublishedOutputPortsUpdated(Module *module, BasicBlock *block,
																		  Value *compositionStateValue, Value *portIdentifierValue,
																		  Value *sentDataValue, Value *portDataSummaryValue)
{
	const char *functionName = "vuoSendPublishedOutputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(sentDataValue);
	args.push_back(portDataSummaryValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendEventFinished()`.
 */
void VuoCompilerCodeGenUtilities::generateSendEventFinished(Module *module, BasicBlock *block,
															Value *compositionStateValue, Value *eventIdValue)
{
	const char *functionName = "vuoSendEventFinished";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(eventIdType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(eventIdValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoSendEventDropped()`.
 */
void VuoCompilerCodeGenUtilities::generateSendEventDropped(Module *module, BasicBlock *block,
														   Value *compositionStateValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoSendEventDropped";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the return value of the `vuoShouldSendPortDataTelemetry()` function as a comparison value.
 */
ICmpInst * VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(Module *module, BasicBlock *block, string portIdentifier,
																				  Value *compositionStateValue,
																				  VuoCompilerConstantsCache *constantsCache)
{
	const char *functionName = "vuoShouldSendPortDataTelemetry";
	Function *shouldSendTelemetryFunction = module->getFunction(functionName);
	if (! shouldSendTelemetryFunction)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		shouldSendTelemetryFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Constant *portIdentifierValue = constantsCache->get(portIdentifier);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	CallInst *retValue = CallInst::Create(shouldSendTelemetryFunction, args, "", block);

	Constant *zeroValue = ConstantInt::get(retValue->getType(), 0);
	return new ICmpInst(*block, ICmpInst::ICMP_NE, retValue, zeroValue, "");
}

/**
 * Generates code that checks if `vuoIsNodeBeingRemovedOrReplaced()` returns true for @a nodeIdentifier.
 * This function initializes @a trueBlock and @a falseBlock and sets them up to be the blocks executed depending on the return value.
 */
void VuoCompilerCodeGenUtilities::generateIsNodeBeingRemovedOrReplacedCheck(Module *module, Function *function, string nodeIdentifier,
																			Value *compositionStateValue,
																			BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock,
																			VuoCompilerConstantsCache *constantsCache,
																			Value *&replacementJsonValue)
{
	Type *boolType = IntegerType::get(module->getContext(), 64);
	PointerType *pointerToJsonObjectType = PointerType::get(getJsonObjectType(module), 0);

	const char *functionName = "vuoIsNodeBeingRemovedOrReplaced";
	Function *calledFunction = module->getFunction(functionName);
	if (! calledFunction)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToPointerToJsonObjectType = PointerType::get(pointerToJsonObjectType, 0);

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToPointerToJsonObjectType);
		FunctionType *functionType = FunctionType::get(boolType, functionParams, false);
		calledFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *nodeIdentifierValue = constantsCache->get(nodeIdentifier);

	AllocaInst *replacementJsonVariable = new AllocaInst(pointerToJsonObjectType, "", initialBlock);
	new StoreInst(ConstantPointerNull::get(pointerToJsonObjectType), replacementJsonVariable, false, initialBlock);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIdentifierValue);
	args.push_back(replacementJsonVariable);
	CallInst *retValue = CallInst::Create(calledFunction, args, "", initialBlock);

	replacementJsonValue = new LoadInst(replacementJsonVariable, "", false, initialBlock);

	trueBlock = BasicBlock::Create(module->getContext(), "removedReplaced", function, NULL);
	falseBlock = BasicBlock::Create(module->getContext(), "notRemovedReplaced", function, NULL);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	ICmpInst *retIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, retValue, falseValue, "");
	BranchInst::Create(trueBlock, falseBlock, retIsTrue, initialBlock);
}

/**
 * Generates code that checks if `vuoIsNodeBeingAddedOrReplaced()` returns true for @a nodeIdentifier.
 * This function initializes @a trueBlock and @a falseBlock and sets them up to be the blocks executed depending on the return value.
 */
ICmpInst * VuoCompilerCodeGenUtilities::generateIsNodeBeingAddedOrReplacedCheck(Module *module, Function *function, string nodeIdentifier,
																				Value *compositionStateValue,
																				BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock,
																				VuoCompilerConstantsCache *constantsCache,
																				Value *&replacementJsonValue)
{
	Type *boolType = IntegerType::get(module->getContext(), 64);
	PointerType *pointerToJsonObjectType = PointerType::get(getJsonObjectType(module), 0);

	const char *functionName = "vuoIsNodeBeingAddedOrReplaced";
	Function *calledFunction = module->getFunction(functionName);
	if (! calledFunction)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToPointerToJsonObjectType = PointerType::get(pointerToJsonObjectType, 0);

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToPointerToJsonObjectType);
		FunctionType *functionType = FunctionType::get(boolType, functionParams, false);
		calledFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *nodeIdentifierValue = constantsCache->get(nodeIdentifier);

	AllocaInst *replacementJsonVariable = new AllocaInst(pointerToJsonObjectType, "", initialBlock);
	new StoreInst(ConstantPointerNull::get(pointerToJsonObjectType), replacementJsonVariable, false, initialBlock);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIdentifierValue);
	args.push_back(replacementJsonVariable);
	CallInst *retValue = CallInst::Create(calledFunction, args, "", initialBlock);

	replacementJsonValue = new LoadInst(replacementJsonVariable, "", false, initialBlock);

	trueBlock = BasicBlock::Create(module->getContext(), "addedReplaced", function, NULL);
	falseBlock = BasicBlock::Create(module->getContext(), "notAddedReplaced", function, NULL);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	ICmpInst *retIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, retValue, falseValue, "");
	BranchInst::Create(trueBlock, falseBlock, retIsTrue, initialBlock);

	return retIsTrue;
}

/**
 * Generates a dummy event ID to represent that no event is claiming a node.
 */
ConstantInt * VuoCompilerCodeGenUtilities::generateNoEventIdConstant(Module *module)
{
	return ConstantInt::get(module->getContext(), APInt(64, 0));
}

/**
 * Generates a call to `vuoGetNodeContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContext(Module *module, BasicBlock *block,
															Value *compositionStateValue, size_t nodeIndex)
{
	IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
	Value *nodeIndexValue = ConstantInt::get(unsignedLongType, nodeIndex);
	return generateGetNodeContext(module, block, compositionStateValue, nodeIndexValue);
}

/**
 * Generates a call to `vuoGetNodeContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContext(Module *module, BasicBlock *block,
															Value *compositionStateValue, Value *nodeIndexValue)
{
	const char *functionName = "vuoGetNodeContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		functionParams.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(pointerToNodeContextType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIndexValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetCompositionContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetCompositionContext(Module *module, BasicBlock *block,
																   Value *compositionStateValue)
{
	const char *functionName = "vuoGetCompositionContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		FunctionType *functionType = FunctionType::get(pointerToNodeContextType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoAddNodeMetadata()`.
 */
void VuoCompilerCodeGenUtilities::generateAddNodeMetadata(Module *module, BasicBlock *block,
														  Value *compositionStateValue, Value *nodeIdentifierValue,
														  Function *compositionCreateContextForNodeFunction,
														  Function *compositionSetPortValueFunction,
														  Function *compositionGetPortValueFunction,
														  Function *compositionFireTriggerPortEventFunction,
														  Function *compositionReleasePortDataFunction)
{
	const char *functionName = "vuoAddNodeMetadata";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *compositionCreateContextForNodeFunctionType = PointerType::getUnqual(getCompositionCreateContextForNodeFunction(module)->getFunctionType());
		PointerType *compositionSetPortValueFunctionType = PointerType::getUnqual(getCompositionSetPortValueFunction(module)->getFunctionType());
		PointerType *compositionGetPortValueFunctionType = PointerType::getUnqual(getCompositionGetPortValueFunction(module)->getFunctionType());
		PointerType *compositionFireTriggerPortEventFunctionType = PointerType::getUnqual(getCompositionFireTriggerPortEventFunction(module)->getFunctionType());
		PointerType *compositionReleasePortDataFunctionType = PointerType::getUnqual(getCompositionReleasePortDataFunction(module)->getFunctionType());

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(compositionCreateContextForNodeFunctionType);
		functionParams.push_back(compositionSetPortValueFunctionType);
		functionParams.push_back(compositionGetPortValueFunctionType);
		functionParams.push_back(compositionFireTriggerPortEventFunctionType);
		functionParams.push_back(compositionReleasePortDataFunctionType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(nodeIdentifierValue);
	args.push_back(compositionCreateContextForNodeFunction);
	args.push_back(compositionSetPortValueFunction);
	args.push_back(compositionGetPortValueFunction);
	args.push_back(compositionFireTriggerPortEventFunction);
	args.push_back(compositionReleasePortDataFunction);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoAddPortMetadata()`.
 */
void VuoCompilerCodeGenUtilities::generateAddPortMetadata(Module *module, BasicBlock *block,
														  Value *compositionStateValue, Value *portIdentifierValue,
														  Value *portNameValue, size_t typeIndex, Value *initialValueValue)
{
	IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoAddPortMetadata";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(compositionStateValue->getType());
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(unsignedLongType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *typeIndexValue = ConstantInt::get(unsignedLongType, typeIndex);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(portNameValue);
	args.push_back(typeIndexValue);
	args.push_back(initialValueValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoInitContextForTopLevelComposition()`.
 */
void VuoCompilerCodeGenUtilities::generateInitContextForTopLevelComposition(Module *module, BasicBlock *block, Value *compositionStateValue,
																			bool isStatefulComposition, size_t publishedOutputPortCount)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 64);
	IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoInitContextForTopLevelComposition";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(boolType);
		functionParams.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *isStatefulCompositionValue = ConstantInt::get(boolType, isStatefulComposition);
	Value *publishedOutputPortCountValue = ConstantInt::get(unsignedLongType, publishedOutputPortCount);

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(isStatefulCompositionValue);
	args.push_back(publishedOutputPortCountValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoFiniContextForTopLevelComposition()`.
 */
void VuoCompilerCodeGenUtilities::generateFiniContextForTopLevelComposition(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoFiniContextForTopLevelComposition";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetTriggerWorkersScheduled()`.
 */
Value * VuoCompilerCodeGenUtilities::getTriggerWorkersScheduledValue(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoGetTriggerWorkersScheduled";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		Type *dispatchGroupType = getDispatchGroupType(module);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(dispatchGroupType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetInputPortString()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetInputPortString(Module *module, BasicBlock *block, Value *compositionStateValue,
																Value *portIdentifierValue, Value *interprocessSerializationValue)
{
	const char *functionName = "vuoGetInputPortString";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(interprocessSerializationValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetOutputPortString()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetOutputPortString(Module *module, BasicBlock *block, Value *compositionStateValue,
																 Value *portIdentifierValue, Value *interprocessSerializationValue)
{
	const char *functionName = "vuoGetOutputPortString";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(portIdentifierValue);
	args.push_back(interprocessSerializationValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the value of the `vuoRuntimeState` global variable.
 */
Value * VuoCompilerCodeGenUtilities::generateRuntimeStateValue(Module *module, BasicBlock *block)
{
	const char *variableName = "vuoRuntimeState";
	GlobalVariable *variable = module->getNamedGlobal(variableName);
	if (! variable)
	{
		PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		variable = new GlobalVariable(*module, voidPointer, false, GlobalValue::ExternalLinkage, 0, variableName);
	}

	return new LoadInst(variable, "", false, block);
}

/**
 * Generates a call to `vuoGetNextEventId()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNextEventId(Module *module, BasicBlock *block, Value *compositionStateValue)
{
	const char *functionName = "vuoGetNextEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		Type *eventIdType = IntegerType::get(module->getContext(), 64);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(eventIdType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoCreateTriggerWorkerContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreateTriggerWorkerContext(Module *module, BasicBlock *block,
																		Value *compositionStateValue, Value *dataCopyValue,
																		Value *eventIdCopyValue)
{
	const char *functionName = "vuoCreateTriggerWorkerContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToEventIdType = PointerType::get(generateNoEventIdConstant(module)->getType(), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(voidPointerType);
		functionParams.push_back(pointerToEventIdType);
		FunctionType *functionType = FunctionType::get(voidPointerType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(dataCopyValue);
	args.push_back(eventIdCopyValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoFreeTriggerWorkerContext()`.
 */
void VuoCompilerCodeGenUtilities::generateFreeTriggerWorkerContext(Module *module, BasicBlock *block, Value *contextValue)
{
	const char *functionName = "vuoFreeTriggerWorkerContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(voidPointerType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(contextValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoCreatePublishedInputWorkerContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateCreatePublishedInputWorkerContext(Module *module, BasicBlock *block,
																			   Value *compositionStateValue, Value *inputPortIdentifierValue,
																			   Value *valueAsStringValue, Value *isCompositionRunningValue)
{
	const char *functionName = "vuoCreatePublishedInputWorkerContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *boolType = IntegerType::get(module->getContext(), 64);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToChar);
		functionParams.push_back(pointerToChar);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(voidPointerType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	args.push_back(inputPortIdentifierValue);
	args.push_back(valueAsStringValue);
	args.push_back(isCompositionRunningValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoAddCompositionStateToThreadLocalStorage()`.
 */
void VuoCompilerCodeGenUtilities::generateAddCompositionStateToThreadLocalStorage(Module *module, BasicBlock *block,
																				  Value *compositionStateValue)
{
	const char *functionName = "vuoAddCompositionStateToThreadLocalStorage";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionStateValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoRemoveCompositionStateFromThreadLocalStorage()`.
 */
void VuoCompilerCodeGenUtilities::generateRemoveCompositionStateFromThreadLocalStorage(Module *module, BasicBlock *block)
{
	const char *functionName = "vuoRemoveCompositionStateFromThreadLocalStorage";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that gets the value of the @c stderr global variable.
 */
Value * VuoCompilerCodeGenUtilities::generateStderr(Module *module, BasicBlock *block)
{
	PointerType *pointerToFileType = getPointerToFileType(module);

	string variableName = "__stderrp";
	GlobalVariable *stderrVariable = module->getNamedGlobal(variableName);
	if (! stderrVariable)
		stderrVariable = new GlobalVariable(*module, pointerToFileType, false, GlobalValue::ExternalLinkage, 0, variableName);

	return new LoadInst(stderrVariable, "", false, block);
}

/**
 * Generates code that prints to stderr either a string literal or a formatted string with a value. (Useful for debugging.)
 */
void VuoCompilerCodeGenUtilities::generatePrint(Module *module, BasicBlock *block, string formatString, Value *value)
{
	vector<Value *> values;
	if (value)
		values.push_back(value);
	generatePrint(module, block, formatString, values);
}

/**
 * Generates code that prints to stderr either a string literal or a formatted string with a value. (Useful for debugging.)
 */
void VuoCompilerCodeGenUtilities::generatePrint(Module *module, BasicBlock *block, string formatString, const vector<Value *> &values)
{
	Value *formatStringValue = generatePointerToConstantString(module, formatString);
	Value *stderrValue = generateStderr(module, block);

	Function *fprintfFunction = getFprintfFunction(module);
	vector<Value *> fprintfArgs;
	fprintfArgs.push_back(stderrValue);
	fprintfArgs.push_back(formatStringValue);
	for (vector<Value *>::const_iterator i = values.begin(); i != values.end(); ++i)
		fprintfArgs.push_back(*i);
	CallInst::Create(fprintfFunction, fprintfArgs, "", block);
}

///@{
/**
 * Returns a Type reference, generating code for the declaration if needed.
 */
PointerType * VuoCompilerCodeGenUtilities::getDispatchSemaphoreType(Module *module)
{
	StructType *dispatch_semaphore_s_type = module->getTypeByName("struct.dispatch_semaphore_s");
	if (! dispatch_semaphore_s_type)
		dispatch_semaphore_s_type = StructType::create(module->getContext(), "struct.dispatch_semaphore_s");

	PointerType *dispatch_semaphore_t_type = PointerType::get(dispatch_semaphore_s_type, 0);
	return dispatch_semaphore_t_type;
}

PointerType * VuoCompilerCodeGenUtilities::getDispatchGroupType(Module *module)
{
	StructType *dispatch_group_s_type = module->getTypeByName("struct.dispatch_group_s");
	if (! dispatch_group_s_type)
		dispatch_group_s_type = StructType::create(module->getContext(), "struct.dispatch_group_s");

	PointerType *dispatch_group_t_type = PointerType::get(dispatch_group_s_type, 0);
	return dispatch_group_t_type;
}

PointerType * VuoCompilerCodeGenUtilities::getDispatchQueueType(Module *module)
{
	StructType *dispatch_queue_s_type = module->getTypeByName("struct.dispatch_queue_s");
	if (! dispatch_queue_s_type)
		dispatch_queue_s_type = StructType::create(module->getContext(), "struct.dispatch_queue_s");

	PointerType *dispatch_queue_t_type = PointerType::get(dispatch_queue_s_type, 0);
	return dispatch_queue_t_type;
}

StructType * VuoCompilerCodeGenUtilities::getDispatchObjectElementType(Module *module)
{
	StructType *dispatch_object_s_type = module->getTypeByName("struct.dispatch_object_s");
	if (! dispatch_object_s_type)
		dispatch_object_s_type = StructType::create(module->getContext(), "struct.dispatch_object_s");
	vector<Type *> dispatch_object_s_fields;
	if (dispatch_object_s_type->isOpaque())
		dispatch_object_s_type->setBody(dispatch_object_s_fields, false);
	return dispatch_object_s_type;
}

StructType * VuoCompilerCodeGenUtilities::getDispatchObjectType(Module *module)
{
	StructType *dispatch_object_s_type = getDispatchObjectElementType(module);
	PointerType *pointerTo_dispatch_object_s_type = PointerType::get(dispatch_object_s_type, 0);

	StructType *dispatch_object_t_type = module->getTypeByName("union.dispatch_object_t");
	if (! dispatch_object_t_type)
		dispatch_object_t_type = StructType::create(module->getContext(), "union.dispatch_object_t");
	vector<Type *> dispatch_object_t_fields;
	dispatch_object_t_fields.push_back(pointerTo_dispatch_object_s_type);
	if (dispatch_object_t_type->isOpaque())
		dispatch_object_t_type->setBody(dispatch_object_t_fields, false);
	return dispatch_object_t_type;
}

StructType * VuoCompilerCodeGenUtilities::getNodeContextType(Module *module)
{
	StructType *nodeContextType = module->getTypeByName("struct.NodeContext");
	if (! nodeContextType)
		nodeContextType = StructType::create(module->getContext(), "struct.NodeContext");

	if (nodeContextType->isOpaque())
	{
		PointerType *pointerToPointerToPortContextType = PointerType::get(PointerType::get(getPortContextType(module), 0), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *longType = IntegerType::get(module->getContext(), 64);

		vector<Type *> fields;
		fields.push_back(pointerToPointerToPortContextType);
		fields.push_back(longType);
		fields.push_back(voidPointerType);
		fields.push_back(getDispatchSemaphoreType(module));
		fields.push_back(longType);
		fields.push_back(getDispatchGroupType(module));
		fields.push_back(longType);
		nodeContextType->setBody(fields, false);
	}

	return nodeContextType;
}

StructType * VuoCompilerCodeGenUtilities::getPortContextType(Module *module)
{
	StructType *portContextType = module->getTypeByName("struct.PortContext");
	if (! portContextType)
		portContextType = StructType::create(module->getContext(), "struct.PortContext");

	if (portContextType->isOpaque())
	{
		Type *boolType = IntegerType::get(module->getContext(), 64);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> fields;
		fields.push_back(boolType);
		fields.push_back(voidPointerType);
		fields.push_back(boolType);
		fields.push_back(getDispatchQueueType(module));
		fields.push_back(getDispatchSemaphoreType(module));
		fields.push_back(voidPointerType);
		portContextType->setBody(fields, false);
	}

	return portContextType;
}

StructType * VuoCompilerCodeGenUtilities::getCompositionStateType(Module *module)
{
	StructType *compositionStateType = module->getTypeByName("struct.VuoCompositionState");
	if (! compositionStateType)
		compositionStateType = StructType::create(module->getContext(), "struct.VuoCompositionState");

	if (compositionStateType->isOpaque())
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> fields;
		fields.push_back(voidPointerType);
		fields.push_back(pointerToCharType);
		compositionStateType->setBody(fields, false);
	}

	return compositionStateType;
}

StructType * VuoCompilerCodeGenUtilities::getGraphvizGraphType(Module *module)
{
	StructType *graphType = module->getTypeByName("struct.Agraph_t");
	if (! graphType)
		graphType = StructType::create(module->getContext(), "struct.Agraph_t");

	return graphType;
}

StructType * VuoCompilerCodeGenUtilities::getJsonObjectType(Module *module)
{
	StructType *jsonObjectType = module->getTypeByName("struct.json_object");
	if (! jsonObjectType)
		jsonObjectType = StructType::create(module->getContext(), "struct.json_object");

	return jsonObjectType;
}

PointerType * VuoCompilerCodeGenUtilities::getPointerToFileType(Module *module)
{
	StructType *fileType = module->getTypeByName("struct.__sFILE");
	if (! fileType)
		fileType = StructType::create(module->getContext(), "struct.__sFILE");

	return PointerType::get(fileType, 0);
}

PointerType * VuoCompilerCodeGenUtilities::getVuoShaderType(Module *module)
{
	StructType *shaderStructType = module->getTypeByName("struct._VuoShader");
	if (! shaderStructType)
		shaderStructType = StructType::create(module->getContext(), "struct._VuoShader");

	return PointerType::get(shaderStructType, 0);
}

PointerType * VuoCompilerCodeGenUtilities::getVuoImageType(Module *module)
{
	StructType *imageStructType = module->getTypeByName("struct._VuoImage");
	if (! imageStructType)
		imageStructType = StructType::create(module->getContext(), "struct._VuoImage");

	return PointerType::get(imageStructType, 0);
}

Type * VuoCompilerCodeGenUtilities::getVuoImageColorDepthType(Module *module)
{
	return IntegerType::get(module->getContext(), 32);
}

PointerType * VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(Module *module)
{
	return PointerType::get(IntegerType::get(module->getContext(), 8), 0);
}
///@}


///@{
/**
 * Returns a Function reference, generating code for the declaration if needed.
 */
Function * VuoCompilerCodeGenUtilities::getStrcatFunction(Module *module)
{
	const char *functionName = "strcat";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getStrcmpFunction(Module *module)
{
	const char *functionName = "strcmp";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		vector<Type *> functionParams;
		functionParams.push_back(pointerToi8Type);
		functionParams.push_back(pointerToi8Type);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getStrdupFunction(Module *module)
{
	const char *functionName = "strdup";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getStrlenFunction(Module *module)
{
	const char *functionName = "strlen";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 64), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSnprintfFunction(Module *module)
{
	const char *functionName = "snprintf";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(IntegerType::get(module->getContext(), 64));
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, true);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSscanfFunction(Module *module)
{
	const char *functionName = "sscanf";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, true);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getFprintfFunction(Module *module)
{
	const char *functionName = "fprintf";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back( getPointerToFileType(module) );
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, true);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getPutsFunction(Module *module)
{
	const char *functionName = "puts";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getMallocFunction(Module *module)
{
	const char *functionName = "malloc";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(IntegerType::get(module->getContext(), sizeof(size_t)*CHAR_BIT));
		FunctionType *functionType = FunctionType::get(pointerToi8Type, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getFreeFunction(Module *module)
{
	const char *functionName = "free";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToi8Type);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getAnnotateFunction(Module *module)
{
	const char *functionName = "llvm.var.annotation";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToi8Type);
		functionParams.push_back(pointerToi8Type);
		functionParams.push_back(pointerToi8Type);
		functionParams.push_back(IntegerType::get(module->getContext(), 32));
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getJsonObjectPutFunction(Module *module)
{
	const char *functionName = "json_object_put";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToJsonObjectType = PointerType::get(getJsonObjectType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToJsonObjectType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getJsonObjectToJsonStringExtFunction(Module *module)
{
	const char *functionName = "json_object_to_json_string_ext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToJsonObjectType = PointerType::get(getJsonObjectType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToJsonObjectType);
		functionParams.push_back(IntegerType::get(module->getContext(), 32));
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getJsonTokenerParseFunction(Module *module)
{
	const char *functionName = "json_tokener_parse";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToJsonObjectType = PointerType::get(getJsonObjectType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToJsonObjectType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoShaderMakeFunction(Module *module)
{
	const char *functionName = "VuoShader_make";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *shaderType = getVuoShaderType(module);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(shaderType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoShaderAddSourceFunction(Module *module)
{
	const char *functionName = "VuoShader_addSource";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *shaderType = getVuoShaderType(module);
		Type *elementAsseblyMethodType = IntegerType::get(module->getContext(), 64);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(shaderType);
		functionParams.push_back(elementAsseblyMethodType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function *VuoCompilerCodeGenUtilities::getVuoShaderSetTransparentFunction(Module *module)
{
	const char *functionName = "VuoShader_setTransparent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *shaderType = getVuoShaderType(module);
		Type *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(shaderType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoShaderSetUniformFunction(Module *module, VuoCompilerType *type)
{
	string functionName = "VuoShader_setUniform_" + type->getBase()->getModuleKey();
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *shaderType = getVuoShaderType(module);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		Type *dataSecondParam = nullptr;
		Type *dataParam = type->getFunctionParameterType(&dataSecondParam);

		vector<Type *> functionParams;
		functionParams.push_back(shaderType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(dataParam);
		if (dataSecondParam)
			functionParams.push_back(dataSecondParam);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoSamplerRectCoordinatesFromNormalizedCoordinatesFunction(Module *module)
{
	string functionName = "VuoShader_samplerRectCoordinatesFromNormalizedCoordinates";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *loweredPoint2dType = Type::getDoubleTy(module->getContext());
		Type *intType = IntegerType::get(module->getContext(), 64);

		vector<Type *> functionParams;
		functionParams.push_back(loweredPoint2dType);
		functionParams.push_back(intType);
		functionParams.push_back(intType);
		FunctionType *functionType = FunctionType::get(loweredPoint2dType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoImageGetColorDepthFunction(Module *module)
{
	const char *functionName = "VuoImage_getColorDepth";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *imageType = getVuoImageType(module);
		Type *imageColorDepthType = getVuoImageColorDepthType(module);

		vector<Type *> functionParams;
		functionParams.push_back(imageType);
		FunctionType *functionType = FunctionType::get(imageColorDepthType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoImageRendererRenderFunction(Module *module)
{
	const char *functionName = "VuoImageRenderer_render";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		Type *shaderType = getVuoShaderType(module);
		Type *intType = IntegerType::get(module->getContext(), 64);
		Type *imageColorDepthType = getVuoImageColorDepthType(module);
		Type *imageType = getVuoImageType(module);

		vector<Type *> functionParams;
		functionParams.push_back(shaderType);
		functionParams.push_back(intType);
		functionParams.push_back(intType);
		functionParams.push_back(imageColorDepthType);
		FunctionType *functionType = FunctionType::get(imageType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionAddNodeMetadataFunction(Module *module)
{
	string functionName = "compositionAddNodeMetadata";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionCreateContextForNodeFunction(Module *module)
{
	string functionName = "compositionCreateContextForNode";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(pointerToNodeContextType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionPerformDataOnlyTransmissionsFunction(Module *module)
{
	string functionName = "compositionPerformDataOnlyTransmissions";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionReleasePortDataFunction(Module *module)
{
	string functionName = "compositionReleasePortData";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> functionParams;
		functionParams.push_back(voidPointer);
		functionParams.push_back(unsignedLongType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSetupFunction(Module *module)
{
	const char *functionName = "vuoSetup";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCleanupFunction(Module *module)
{
	const char *functionName = "vuoCleanup";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getInstanceInitFunction(Module *module)
{
	const char *functionName = "vuoInstanceInit";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getInstanceFiniFunction(Module *module)
{
	const char *functionName = "vuoInstanceFini";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getInstanceTriggerStartFunction(Module *module)
{
	const char *functionName = "vuoInstanceTriggerStart";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), PointerType::get(getCompositionInstanceDataType(module), 0), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getInstanceTriggerStopFunction(Module *module)
{
	const char *functionName = "vuoInstanceTriggerStop";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), PointerType::get(getCompositionInstanceDataType(module), 0), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(Module *module, string moduleKey, bool isSubcomposition,
																	Type *instanceDataType,
																	const vector<VuoPort *> &modelInputPorts,
																	map<VuoPort *, size_t> &indexOfParameter,
																	VuoCompilerConstantsCache *constantsCache)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceInit", isSubcomposition, false, true, false, instanceDataType, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(Module *module, string moduleKey,
																	Type *instanceDataType,
																	VuoCompilerConstantsCache *constantsCache)
{
	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceFini", true, true, false, false, instanceDataType, vector<VuoPort *>(), vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStartFunction(Module *module, string moduleKey,
																			Type *instanceDataType,
																			const vector<VuoPort *> &modelInputPorts,
																			map<VuoPort *, size_t> &indexOfParameter,
																			VuoCompilerConstantsCache *constantsCache)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerStart", true, true, false, false, instanceDataType, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(),  map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStopFunction(Module *module, string moduleKey,
																		   Type *instanceDataType,
																		   VuoCompilerConstantsCache *constantsCache)
{
	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerStop", true, true, false, false, instanceDataType, vector<VuoPort *>(), vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerUpdateFunction(Module *module, string moduleKey,
																			 Type *instanceDataType,
																			 const vector<VuoPort *> &modelInputPorts,
																			 map<VuoPort *, size_t> &indexOfParameter,
																			 VuoCompilerConstantsCache *constantsCache)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerUpdate", true, true, false, false, instanceDataType, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeEventFunction(Module *module, string moduleKey, bool isSubcomposition, bool isStateful,
															 Type *instanceDataType,
															 const vector<VuoPort *> &modelInputPorts,
															 const vector<VuoPort *> &modelOutputPorts,
															 const map<VuoPort *, json_object *> &detailsForPorts,
															 const map<VuoPort *, string> &displayNamesForPorts,
															 const map<VuoPort *, string> &defaultValuesForInputPorts,
															 const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
															 map<VuoPort *, size_t> &indexOfParameter,
															 map<VuoPort *, size_t> &indexOfEventParameter,
															 VuoCompilerConstantsCache *constantsCache)
{
	string functionName = (isStateful ? "nodeInstanceEvent" : "nodeEvent");
	return getNodeFunction(module, moduleKey, functionName, isSubcomposition, isStateful, false, true, instanceDataType, modelInputPorts, modelOutputPorts,
						   detailsForPorts, displayNamesForPorts, defaultValuesForInputPorts, eventBlockingForInputPorts,
						   indexOfParameter, indexOfEventParameter, constantsCache);
}

Function * VuoCompilerCodeGenUtilities::getNodeFunction(Module *module, string moduleKey, string functionName,
														bool hasCompositionStateArg, bool hasInstanceDataArg,
														bool hasInstanceDataReturn, bool hasEventArgs,
														Type *instanceDataType,
														const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
														const map<VuoPort *, json_object *> &detailsForPorts,
														const map<VuoPort *, string> &displayNamesForPorts,
														const map<VuoPort *, string> &defaultValuesForInputPorts,
														const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
														map<VuoPort *, size_t> &indexOfParameter,
														map<VuoPort *, size_t> &indexOfEventParameter,
														VuoCompilerConstantsCache *constantsCache)
{
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());

	if (! function)
	{
		vector<Type *> functionParams;
		map<int, AttributeSet> functionAttributes;
		map<VuoPort *, bool> hasSecondParam;
		Type *boolType = IntegerType::get(module->getContext(), 1);
		size_t indexInEventFunction = 0;

		if (hasCompositionStateArg)
		{
			PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
			functionParams.push_back(pointerToCompositionState);
			indexInEventFunction++;
		}

		if (hasInstanceDataArg)
		{
			functionParams.push_back( PointerType::get(instanceDataType, 0) );
			indexInEventFunction++;
		}

		for (vector<VuoPort *>::const_iterator i = modelInputPorts.begin(); i != modelInputPorts.end(); ++i)
		{
			VuoCompilerPort *modelInputPort = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
			VuoType *type = modelInputPort->getDataVuoType();

			if (type)
			{
				Type *paramSecondType = NULL;
				Type *paramType = type->getCompiler()->getFunctionParameterType(&paramSecondType);
				AttributeSet paramAttributes = type->getCompiler()->getFunctionParameterAttributes();

				functionParams.push_back(paramType);

				// If we're generating a node function for a subcomposition, and the parameter is a struct that
				// would normally be passed "byval", don't actually make it "byval" in the node function.
				// Instead, the Vuo compiler will generate code equivalent to the "byval" semantics.
				//
				// This is a workaround for a bug where LLVM would sometimes give the node function body
				// an invalid value for a "byval" struct argument. https://b33p.net/kosada/node/11386
				if (! (hasCompositionStateArg &&
					   type->getCompiler()->getType()->isStructTy() &&
					   paramAttributes.hasAttrSomewhere(Attribute::ByVal)) )
				{
					functionAttributes[ functionParams.size() - 1] = paramAttributes;
				}

				indexOfParameter[modelInputPort->getBase()] = indexInEventFunction++;

				if (paramSecondType)
				{
					functionParams.push_back(paramSecondType);
					functionAttributes[ functionParams.size() - 1] = paramAttributes;
					hasSecondParam[*i] = true;

					indexInEventFunction++;
				}

				if (hasEventArgs)
				{
					functionParams.push_back(boolType);

					indexOfEventParameter[modelInputPort->getBase()] = indexInEventFunction++;
				}
			}
			else if (hasEventArgs)
			{
				functionParams.push_back(boolType);

				indexOfParameter[modelInputPort->getBase()] = indexInEventFunction++;
			}
		}

		for (vector<VuoPort *>::const_iterator i = modelOutputPorts.begin(); i != modelOutputPorts.end(); ++i)
		{
			VuoCompilerPort *modelOutputPort = static_cast<VuoCompilerPort *>( (*i)->getCompiler() );
			VuoType *type = modelOutputPort->getDataVuoType();

			if (modelOutputPort->getBase()->getClass()->getPortType() == VuoPortClass::triggerPort)
			{
				FunctionType *functionType = static_cast<VuoCompilerTriggerPortClass *>( modelOutputPort->getBase()->getClass()->getCompiler() )->getFunctionType();
				PointerType *triggerFunctionPointerType = PointerType::get(functionType, 0);
				functionParams.push_back(triggerFunctionPointerType);

				indexOfParameter[modelOutputPort->getBase()] = indexInEventFunction++;
			}
			else
			{
				if (type)
				{
					PointerType *paramType = PointerType::get( type->getCompiler()->getType(), 0 );
					functionParams.push_back(paramType);

					indexOfParameter[modelOutputPort->getBase()] = indexInEventFunction++;

					if (hasEventArgs)
					{
						PointerType *eventParamType = PointerType::get( boolType, 0 );
						functionParams.push_back(eventParamType);

						indexOfEventParameter[modelOutputPort->getBase()] = indexInEventFunction++;
					}
				}
				else if (hasEventArgs)
				{
					PointerType *paramType = PointerType::get( boolType, 0 );
					functionParams.push_back(paramType);

					indexOfParameter[modelOutputPort->getBase()] = indexInEventFunction++;
				}
			}
		}

		Type *returnType = (hasInstanceDataReturn ? instanceDataType : Type::getVoidTy(module->getContext()));
		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		for (map<int, AttributeSet>::iterator i = functionAttributes.begin(); i != functionAttributes.end(); ++i)
		{
			int attributesIndex = i->first + 1;
			AttributeSet attributes = i->second;
			function->addAttributes(attributesIndex, copyAttributesToIndex(attributes, attributesIndex));
		}

		set<string> argNamesUsed;

		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);
		Function::arg_iterator argIter = function->arg_begin();

		if (hasCompositionStateArg)
		{
			Value *arg = argIter++;
			string argName = "compositionState";
			arg->setName(argName);
			argNamesUsed.insert(argName);
		}

		if (hasInstanceDataArg)
		{
			Value *arg = argIter++;
			string argName = "instanceData";
			arg->setName(argName);
			argNamesUsed.insert(argName);
			generateAnnotation(module, block, arg, "vuoInstanceData", "", 0, constantsCache);
		}

		argNamesUsed.insert("refresh");

		map<VuoPort *, string> primaryArgNames;
		auto recordUniqueArgName = [&primaryArgNames, &argNamesUsed] (VuoPort *modelPort)
		{
			string argName = VuoStringUtilities::formUniqueIdentifier(argNamesUsed, modelPort->getClass()->getName());
			primaryArgNames[modelPort] = argName;
		};
		std::for_each(modelInputPorts.begin(), modelInputPorts.end(), recordUniqueArgName);
		std::for_each(modelOutputPorts.begin(), modelOutputPorts.end(), recordUniqueArgName);

		for (vector<VuoPort *>::const_iterator i = modelInputPorts.begin(); i != modelInputPorts.end(); ++i)
		{
			VuoPort *modelInputPort = *i;
			VuoType *type = static_cast<VuoCompilerPort *>( modelInputPort->getCompiler() )->getDataVuoType();

			if (type || hasEventArgs)
			{
				Value *arg = argIter++;

				string portName = modelInputPort->getClass()->getName();
				string argName = primaryArgNames[modelInputPort];
				arg->setName(argName);

				map<VuoPort *, VuoPortClass::EventBlocking>::const_iterator eventBlockingIter = eventBlockingForInputPorts.find(modelInputPort);
				bool hasNonDefaultEventBlocking = (eventBlockingIter != eventBlockingForInputPorts.end() && eventBlockingIter->second != VuoPortClass::EventBlocking_None);
				string eventBlockingStr = (eventBlockingIter->second == VuoPortClass::EventBlocking_Door ? "door" : "wall");

				json_object *details = json_object_new_object();
				map<VuoPort *, json_object *>::const_iterator detailsIter = detailsForPorts.find(modelInputPort);
				if (detailsIter != detailsForPorts.end())
				{
					json_object_object_foreach(detailsIter->second, key, val)
					{
						json_object_object_add(details, key, val);
						json_object_get(val);
					}
				}
				map<VuoPort *, string>::const_iterator displayNameIter = displayNamesForPorts.find(modelInputPort);
				if (displayNameIter != displayNamesForPorts.end())
					json_object_object_add(details, "name", json_object_new_string(displayNameIter->second.c_str()));

				if (type)
				{
					generateAnnotation(module, block, arg, "vuoInputData", "", 0, constantsCache);
					generateAnnotation(module, block, arg, "vuoType:" + type->getModuleKey(), "", 0, constantsCache);

					map<VuoPort *, string>::const_iterator defaultValueIter = defaultValuesForInputPorts.find(modelInputPort);
					if (defaultValueIter != defaultValuesForInputPorts.end())
						json_object_object_add(details, "default", json_tokener_parse(defaultValueIter->second.c_str()));

					if (hasSecondParam[modelInputPort])
					{
						Value *secondArg = argIter++;
						secondArg->setName(argName + ".1");
					}

					if (hasEventArgs)
					{
						Value *eventArg = argIter++;

						string preferredEventArgName = portName + "Event";
						string eventArgName = VuoStringUtilities::formUniqueIdentifier(argNamesUsed, preferredEventArgName);
						eventArg->setName(eventArgName);

						generateAnnotation(module, block, eventArg, "vuoInputEvent", "", 0, constantsCache);

						json_object *eventDetails = json_object_new_object();
						json_object_object_add(eventDetails, "data", json_object_new_string(argName.c_str()));
						if (hasNonDefaultEventBlocking)
							json_object_object_add(eventDetails, "eventBlocking", json_object_new_string(eventBlockingStr.c_str()));
						string eventDetailsStr = json_object_to_json_string_ext(eventDetails, JSON_C_TO_STRING_PLAIN);
						json_object_put(eventDetails);
						generateAnnotation(module, block, eventArg, "vuoDetails:" + eventDetailsStr, "", 0, constantsCache);
					}
				}
				else if (hasEventArgs)
				{
					generateAnnotation(module, block, arg, "vuoInputEvent", "", 0, constantsCache);

					if (hasNonDefaultEventBlocking)
						json_object_object_add(details, "eventBlocking", json_object_new_string(eventBlockingStr.c_str()));
				}

				string detailsStr = json_object_to_json_string_ext(details, JSON_C_TO_STRING_PLAIN);
				json_object_put(details);
				generateAnnotation(module, block, arg, "vuoDetails:" + detailsStr, "", 0, constantsCache);
			}
		}

		for (vector<VuoPort *>::const_iterator i = modelOutputPorts.begin(); i != modelOutputPorts.end(); ++i)
		{
			VuoPort *modelOutputPort = *i;

			Value *arg = argIter++;

			string portName = modelOutputPort->getClass()->getName();
			string argName = primaryArgNames[modelOutputPort];
			arg->setName(argName);

			json_object *details = json_object_new_object();
			map<VuoPort *, json_object *>::const_iterator detailsIter = detailsForPorts.find(modelOutputPort);
			if (detailsIter != detailsForPorts.end())
			{
				json_object_object_foreach(detailsIter->second, key, val)
				{
					json_object_object_add(details, key, val);
					json_object_get(val);
				}
			}
			map<VuoPort *, string>::const_iterator displayNameIter = displayNamesForPorts.find(modelOutputPort);
			if (displayNameIter != displayNamesForPorts.end())
				json_object_object_add(details, "name", json_object_new_string(displayNameIter->second.c_str()));
			string detailsStr = json_object_to_json_string_ext(details, JSON_C_TO_STRING_PLAIN);
			json_object_put(details);
			generateAnnotation(module, block, arg, "vuoDetails:" + detailsStr, "", 0, constantsCache);

			VuoType *type = static_cast<VuoCompilerPort *>( modelOutputPort->getCompiler() )->getDataVuoType();
			if (modelOutputPort->getClass()->getPortType() == VuoPortClass::triggerPort)
			{
				generateAnnotation(module, block, arg, "vuoOutputTrigger:" + argName, "", 0, constantsCache);
				generateAnnotation(module, block, arg, "vuoType:" + (type ? type->getModuleKey() : "void"), "", 0, constantsCache);
			}
			else
			{
				if (type)
				{
					generateAnnotation(module, block, arg, "vuoOutputData", "", 0, constantsCache);
					generateAnnotation(module, block, arg, "vuoType:" + type->getModuleKey(), "", 0, constantsCache);

					if (hasEventArgs)
					{
						Value *eventArg = argIter++;

						string preferredEventArgName = portName + "Event";
						string eventArgName = VuoStringUtilities::formUniqueIdentifier(argNamesUsed, preferredEventArgName);
						eventArg->setName(eventArgName);

						generateAnnotation(module, block, eventArg, "vuoOutputEvent", "", 0, constantsCache);

						json_object *eventDetails = json_object_new_object();
						json_object_object_add(eventDetails, "data", json_object_new_string(argName.c_str()));
						string eventDetailsStr = json_object_to_json_string_ext(eventDetails, JSON_C_TO_STRING_PLAIN);
						json_object_put(eventDetails);
						generateAnnotation(module, block, eventArg, "vuoDetails:" + eventDetailsStr, "", 0, constantsCache);
					}
				}
				else if (hasEventArgs)
				{
					generateAnnotation(module, block, arg, "vuoOutputEvent", "", 0, constantsCache);
				}
			}
		}
	}

	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoRegisterFunction(Module *module)
{
	const char *functionName = "VuoRegisterF";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		FunctionType *deallocateFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), voidPointerType, false);
		PointerType *deallocateFunctionPointerType = PointerType::get(deallocateFunctionType, 0);

		vector<Type *> functionParams;
		functionParams.push_back(voidPointerType);
		functionParams.push_back(deallocateFunctionPointerType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(intType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(intType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoRetainFunction(Module *module)
{
	const char *functionName = "VuoRetain";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(voidPointerType);	// pointer
		FunctionType *functionType = FunctionType::get(intType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getVuoReleaseFunction(Module *module)
{
	const char *functionName = "VuoRelease";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(voidPointerType);	// pointer
		FunctionType *functionType = FunctionType::get(intType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionGetPortValueFunction(Module *module)
{
	string functionName = "compositionGetPortValue";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(intType);
		functionParams.push_back(intType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(Module *module)
{
	string functionName = "compositionSetPortValue";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *boolType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionFireTriggerPortEventFunction(Module *module)
{
	string functionName = "compositionFireTriggerPortEvent";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(Module *module)
{
	const char *functionName = "vuoSetInputPortValue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getGetPublishedInputPortValueFunction(Module *module)
{
	const char *functionName = "getPublishedInputPortValue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(intType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getGetPublishedOutputPortValueFunction(Module *module)
{
	const char *functionName = "getPublishedOutputPortValue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(intType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionSetPublishedInputPortValueFunction(Module *module)
{
	string functionName = "compositionSetPublishedInputPortValue";
	string moduleKey = module->getModuleIdentifier();
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCompositionState = PointerType::get(getCompositionStateType(module), 0);
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *boolType = IntegerType::get(module->getContext(), 64);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCompositionState);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSetPublishedInputPortValueFunction(Module *module)
{
	const char *functionName = "setPublishedInputPortValue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}
///@}

/**
 * Guesses the type that the parameter of the function had in the source code, before it was possibly
 * converted ("lowered") to some other type by Clang/LLVM when the module was compiled.
 * (For example, a struct whose members are all floats may be lowered to a vector of floats.)
 */
Type * VuoCompilerCodeGenUtilities::getParameterTypeBeforeLowering(Function *function, Module *module, string typeName)
{
	if (typeName == "VuoPoint2d")
		return VectorType::get(Type::getFloatTy(module->getContext()), 2);
	if (typeName == "VuoPoint3d")
		return VectorType::get(Type::getFloatTy(module->getContext()), 3);
	if (typeName == "VuoPoint4d")
		return VectorType::get(Type::getFloatTy(module->getContext()), 4);

	Type *type = function->getFunctionType()->getParamType(0);
	bool hasSecondType = (function->getFunctionType()->getNumParams() == 2);

	// Parameter was originally a struct, but was lowered to a "byval" pointer-to-struct, a vector, or two parameters.
	if (isParameterPassedByValue(function, 0) || type->isVectorTy() || hasSecondType)
		type = module->getTypeByName("struct." + typeName);

	if (! type)
	{
		VUserLog("Couldn't guess the original type for %s", typeName.c_str());
		function->getFunctionType()->dump();  fprintf(stderr, "\n");
	}

	return type;
}

/**
 * If needed, generates code to convert a function argument from the "lowered" type of the function parameter to an un-lowered type.
 *
 * @param unloweredVuoType The type that the argument should be converted to.
 * @param function The function to which the argument has been passed.
 * @param parameterIndex The index of the argument in the function's argument list (starting at 0).
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @return The argument converted to its un-lowered type. May be the same as the original argument.
 */
Value * VuoCompilerCodeGenUtilities::unlowerArgument(VuoCompilerType *unloweredVuoType, Function *function, int parameterIndex,
													 Module *module, BasicBlock *block)
{
	Type *unloweredType = unloweredVuoType->getType();

	Type *secondLoweredType = NULL;
	unloweredVuoType->getFunctionParameterType(&secondLoweredType);
	bool hasSecondArgument = (secondLoweredType != NULL);

	Function::arg_iterator argIter = function->arg_begin();
	for (int i = 0; i < parameterIndex; ++i)
		++argIter;
	Value *argument = argIter;
	Value *secondArgument = (hasSecondArgument ? ++argIter : NULL);

	if (unloweredType->isVectorTy())
	{
		if (static_cast<VectorType *>(unloweredType)->getElementType()->isFloatTy())
		{
			uint64_t elementCount = static_cast<VectorType *>(unloweredType)->getNumElements();
			if (elementCount == 2 && argument->getType()->isDoubleTy())
			{
				// VuoPoint2d — Argument is a vector of 2 floats lowered to a double.
				PointerType *pointerToVectorType = PointerType::get(unloweredType, 0);

				Value *vectorVariableAsDouble = generatePointerToValue(block, argument);
				Value *vectorVariable = new BitCastInst(vectorVariableAsDouble, pointerToVectorType, "", block);
				return new LoadInst(vectorVariable, "", false, block);
			}
			else if (elementCount == 3 && argument->getType()->isVectorTy() &&
					 static_cast<VectorType *>(argument->getType())->getNumElements() == 2 &&
					 static_cast<VectorType *>(argument->getType())->getElementType()->isDoubleTy())
			{
				// VuoPoint3d — Argument is a vector of 3 floats lowered to a vector of 2 doubles.
				PointerType *pointerToFloatVectorType = PointerType::get(unloweredType, 0);

				Value *floatVectorValueAsDoubleVector = generatePointerToValue(block, argument);
				Value *floatVectorVariable = new BitCastInst(floatVectorValueAsDoubleVector, pointerToFloatVectorType, "", block);
				return new LoadInst(floatVectorVariable, "", false, block);
			}
		}
	}
	else if (unloweredType->isStructTy())
	{
		if (! hasSecondArgument)
		{
			if (argument->getType()->isVectorTy())
			{
				// Argument is a struct lowered to a vector.
				PointerType *pointerToStructType = PointerType::get(unloweredType, 0);

				Value *structVariableAsVector = generatePointerToValue(block, argument);
				Value *structVariable = new BitCastInst(structVariableAsVector, pointerToStructType, "", block);
				return new LoadInst(structVariable, "", false, block);
			}
		}
		else
		{
			// Argument is a struct lowered to two parameters (e.g. two vectors).
			PointerType *pointerToStructType = PointerType::get(unloweredType, 0);

			vector<Type *> bothLoweredMembers;
			bothLoweredMembers.push_back(argument->getType());
			bothLoweredMembers.push_back(secondLoweredType);
			StructType *bothLoweredTypes = StructType::create(bothLoweredMembers);

			Value *structVariableAsBothLoweredTypes = new AllocaInst(bothLoweredTypes, "", block);

			ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
			ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(32, 1));

			vector<Value *> firstMemberIndices;
			firstMemberIndices.push_back(zeroValue);
			firstMemberIndices.push_back(zeroValue);
			Value *firstMemberPointer = GetElementPtrInst::Create(structVariableAsBothLoweredTypes, firstMemberIndices, "", block);
			new StoreInst(argument, firstMemberPointer, block);

			vector<Value *> secondMemberIndices;
			secondMemberIndices.push_back(zeroValue);
			secondMemberIndices.push_back(oneValue);
			Value *secondMemberPointer = GetElementPtrInst::Create(structVariableAsBothLoweredTypes, secondMemberIndices, "", block);
			new StoreInst(secondArgument, secondMemberPointer, block);

			Value *structVariable = new BitCastInst(structVariableAsBothLoweredTypes, pointerToStructType, "", block);
			return new LoadInst(structVariable, "", false, block);
		}

		if (isParameterPassedByValue(function, parameterIndex))
		{
			// Argument is a struct passed by value.
			argument = new LoadInst(argument, "", false, block);
		}
	}

	if (argument->getType() != unloweredType)
	{
		// Argument type doesn't match parameter type because they're structs loaded from different modules.
		if (argument->getType()->isStructTy() && unloweredType->isStructTy())
		{
			Value *argumentVariable = generatePointerToValue(block, argument);
			Type *pointerToUnloweredType = PointerType::get(unloweredType, 0);
			Value *argumentVariableAsUnloweredType = new BitCastInst(argumentVariable, pointerToUnloweredType, "", block);
			return new LoadInst(argumentVariableAsUnloweredType, "", false, block);
		}
		else
		{
			Type *sourceType = argument->getType();
			Type *destinationType = unloweredType;
			while (sourceType->isPointerTy() && destinationType->isPointerTy())
			{
				Type *sourceElementType = static_cast<PointerType *>(sourceType)->getElementType();
				Type *destinationElementType = static_cast<PointerType *>(destinationType)->getElementType();
				if (sourceElementType->isStructTy() && destinationElementType->isStructTy())
					return new BitCastInst(argument, unloweredType, "", block);
				sourceType = sourceElementType;
				destinationType = destinationElementType;
			}
		}
	}

	if (argument->getType() != unloweredType)
	{
		VUserLog("Couldn't convert argument %d of %s to the type of %s", parameterIndex, function->getName().str().c_str(), unloweredVuoType->getBase()->getModuleKey().c_str());
		function->arg_begin()->getType()->dump();  fprintf(stderr, "\n");
		argument->getType()->dump();  fprintf(stderr, "\n");
		unloweredType->dump();  fprintf(stderr, "\n");
		return NULL;
	}

	return argument;
}

/**
 * If needed, generates code to convert the argument to the (possibly "lowered") type(s) of the function parameter(s).
 *
 * @param argument The argument to be converted.
 * @param function The function the argument is being passed to.
 * @param parameterIndex The index of the first (and possibly only) function parameter corresponding to @a argument.
 * @param[out] secondArgument Pointer to the second converted argument, to be passed to the second function parameter corresponding to @a argument. If none, pass NULL.
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @return The first converted argument, to be passed to the first (and possibly only) function parameter corresponding to @a argument. May be the same as @a argument.
 * @throw VuoCompilerException `argument` couldn't be converted to the function's parameter type.
 */
Value * VuoCompilerCodeGenUtilities::convertArgumentToParameterType(Value *argument, Function *function, int parameterIndex,
																	Value **secondArgument, Module *module, BasicBlock *block)
{
	return convertArgumentToParameterType(argument, function->getFunctionType(), parameterIndex, isParameterPassedByValue(function, parameterIndex),
										  secondArgument, module, block);
}

/**
 * If needed, generates code to convert the argument to the (possibly "lowered") type(s) of the function parameter(s).
 *
 * @param argument The argument to be converted.
 * @param functionType The type of the function that the argument is being passed to.
 * @param parameterIndex The index of the first (and possibly only) function parameter corresponding to @a argument.
 * @param isPassedByValue Whether the parameter corresponding to @a argument has the "byval" attribute.
 * @param[out] secondArgument Pointer to the second converted argument, to be passed to the second function parameter corresponding to @a argument. If none, pass NULL.
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @return The first converted argument, to be passed to the first (and possibly only) function parameter corresponding to @a argument. May be the same as @a argument.
 * @throw VuoCompilerException `argument` couldn't be converted to the function's parameter type.
 */
Value * VuoCompilerCodeGenUtilities::convertArgumentToParameterType(Value *argument, FunctionType *functionType, int parameterIndex, bool isPassedByValue,
																	Value **secondArgument, Module *module, BasicBlock *block)
{
	if (secondArgument)
		*secondArgument = NULL;

	Type *parameterType = functionType->getParamType(parameterIndex);

	if (argument->getType()->isVectorTy())
	{
		if (static_cast<VectorType *>(argument->getType())->getElementType()->isFloatTy())
		{
			uint64_t elementCount = static_cast<VectorType *>(argument->getType())->getNumElements();
			if (elementCount == 2 && parameterType->isDoubleTy())
			{
				// VuoPoint2d — Argument is a vector of 2 floats lowered to a double.
				PointerType *pointerToDoubleType = PointerType::get(parameterType, 0);

				Value *vectorVariable = generatePointerToValue(block, argument);
				CastInst *vectorVariableAsDouble = new BitCastInst(vectorVariable, pointerToDoubleType, "", block);
				return new LoadInst(vectorVariableAsDouble, "", false, block);
			}
			else if (elementCount == 3 && parameterType->isVectorTy() &&
					 static_cast<VectorType *>(parameterType)->getNumElements() == 2 &&
					 static_cast<VectorType *>(parameterType)->getElementType()->isDoubleTy())
			{
				// VuoPoint3d — Argument is a vector of 3 floats lowered to a vector of 2 doubles.
				PointerType *pointerToDoubleVectorType = PointerType::get(parameterType, 0);

				Value *floatVectorVariable = generatePointerToValue(block, argument);
				CastInst *floatVectorVariableAsDoubleVector = new BitCastInst(floatVectorVariable, pointerToDoubleVectorType, "", block);
				return new LoadInst(floatVectorVariableAsDoubleVector, "", false, block);
			}
			else if (elementCount == 2 && parameterType->isPointerTy() && static_cast<PointerType *>(parameterType)->getElementType() == argument->getType())
			{
				// Workaround for vuo.image.make.checkerboard2 center (https://b33p.net/kosada/node/15936)
				// VuoPoint2d — Argument is a vector of 2 floats lowered to a pointer to a vector of 2 floats.
				return generatePointerToValue(block, argument);
			}
		}
	}
	else if (argument->getType()->isStructTy())
	{
		if (! secondArgument)
		{
			if (parameterType->isVectorTy())
			{
				// Argument is a struct lowered to a vector.
				PointerType *pointerToVectorType = PointerType::get(parameterType, 0);

				Value *structVariable = generatePointerToValue(block, argument);
				CastInst *structVariableAsVector = new BitCastInst(structVariable, pointerToVectorType, "", block);
				return new LoadInst(structVariableAsVector, "", false, block);
			}
		}
		else
		{
			// Argument is a struct lowered to two parameters (e.g. two vectors).
			Type *secondParameterType = functionType->getParamType(parameterIndex + 1);
			vector<Type *> bothParameterMembers;
			bothParameterMembers.push_back(parameterType);
			bothParameterMembers.push_back(secondParameterType);
			StructType *bothParameterTypes = StructType::create(bothParameterMembers);
			PointerType *pointerToBothParameterTypes = PointerType::get(bothParameterTypes, 0);

			Value *structVariable = generatePointerToValue(block, argument);
			CastInst *structVariableAsBothParameterTypes = new BitCastInst(structVariable, pointerToBothParameterTypes, "", block);

			ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
			ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(32, 1));

			vector<Value *> firstMemberIndices;
			firstMemberIndices.push_back(zeroValue);
			firstMemberIndices.push_back(zeroValue);
			Value *firstMemberPointer = GetElementPtrInst::Create(structVariableAsBothParameterTypes, firstMemberIndices, "", block);
			Value *firstMember = new LoadInst(firstMemberPointer, "", false, block);

			vector<Value *> secondMemberIndices;
			secondMemberIndices.push_back(zeroValue);
			secondMemberIndices.push_back(oneValue);
			Value *secondMemberPointer = GetElementPtrInst::Create(structVariableAsBothParameterTypes, secondMemberIndices, "", block);
			Value *secondMember = new LoadInst(secondMemberPointer, "", false, block);

			*secondArgument = secondMember;
			return firstMember;
		}

		if (isPassedByValue)
		{
			// Argument is a struct passed by value.
			argument = generatePointerToValue(block, argument);
		}
	}

	if (argument->getType() != parameterType)
	{
		// Argument type doesn't match parameter type because they're structs loaded from different modules.
		if (argument->getType()->isStructTy() && parameterType->isStructTy())
		{
			Value *argumentVariable = generatePointerToValue(block, argument);
			Type *pointerToParameterType = PointerType::get(parameterType, 0);
			Value *argumentVariableAsParameterType = new BitCastInst(argumentVariable, pointerToParameterType, "", block);
			return new LoadInst(argumentVariableAsParameterType, "", false, block);
		}
		else
		{
			Type *sourceType = argument->getType();
			Type *destinationType = parameterType;
			while (sourceType->isPointerTy() && destinationType->isPointerTy())
			{
				Type *sourceElementType = static_cast<PointerType *>(sourceType)->getElementType();
				Type *destinationElementType = static_cast<PointerType *>(destinationType)->getElementType();
				if (! sourceElementType->isPointerTy() && ! destinationElementType->isPointerTy())
					return new BitCastInst(argument, parameterType, "", block);
				sourceType = sourceElementType;
				destinationType = destinationElementType;
			}
		}
	}

	if (argument->getType() != parameterType)
	{
		string s;
		raw_string_ostream oss(s);
		oss << "Couldn't convert argument type `";
		argument->getType()->print(oss);
		oss << "` to parameter type `";
		parameterType->print(oss);
		oss << "` for function:  \n\n    ";
		functionType->print(oss);
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling composition", "", "Unsupported composition layout", oss.str());
		throw VuoCompilerException(issue);
	}

	return argument;
}

/**
 * Returns true if the function parameter (indexed from 0) has LLVM's 'byval' attribute.
 *
 * @param parameterIndex The index of the function parameter (starting at 0).
 */
bool VuoCompilerCodeGenUtilities::isParameterPassedByValue(Function *function, int parameterIndex)
{
	AttributeSet functionAttrs = function->getAttributes();
	AttributeSet paramAttrs = functionAttrs.getParamAttributes(parameterIndex + 1);
	return paramAttrs.hasAttrSomewhere(Attribute::ByVal);
}

/**
 * Generates a call to the function, whose first parameter is assumed to have LLVM's @c sret attribute.
 */
Value * VuoCompilerCodeGenUtilities::callFunctionWithStructReturn(Function *function, vector<Value *> args, BasicBlock *block)
{
	PointerType *pointerToReturnType = static_cast<PointerType *>(function->getFunctionType()->getParamType(0));
	Type *returnType = pointerToReturnType->getElementType();
	Value *returnVariable = new AllocaInst(returnType, "", block);
	args.insert(args.begin(), returnVariable);
	CallInst::Create(function, args, "", block);
	return returnVariable;
}

/**
 * Returns true if the function's first parameter has LLVM's @c sret attribute, or in other words,
 * LLVM has transformed `struct MyType foo(...)` to `void foo(struct MyType *, ...)`.
 */
bool VuoCompilerCodeGenUtilities::isFunctionReturningStructViaParameter(Function *function)
{
	AttributeSet functionAttrs = function->getAttributes();
	AttributeSet paramAttrs = functionAttrs.getParamAttributes(1);
	return paramAttrs.hasAttrSomewhere(Attribute::StructRet);
}

/**
 * Returns a function type that accepts a @a vuoType (or its lowered form) as its parameter
 * and has a void return type.
 */
FunctionType * VuoCompilerCodeGenUtilities::getFunctionType(Module *module, VuoType *vuoType)
{
	vector<Type *> params;

	if (vuoType)
	{
		Type *secondParamType = NULL;
		Type *firstParamType = vuoType->getCompiler()->getFunctionParameterType(&secondParamType);
		params.push_back(firstParamType);
		if (secondParamType)
			params.push_back(secondParamType);
	}

	return FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
}

/**
 * Returns the argument at @a index (numbered from 0) in the function's argument list.
 */
Value * VuoCompilerCodeGenUtilities::getArgumentAtIndex(Function *function, size_t index)
{
	Value *argValue = NULL;

	Function::arg_iterator args = function->arg_begin();
	for (size_t i = 0; i <= index && i < function->arg_size(); ++i)
		argValue = args++;

	return argValue;
}

/**
 * Creates a new AttributeSet
 * with the attributes from `attributesToCopy`'s index 1 (the AttributeSet's first function parameter)
 * placed in the output AttributeSet's index `destinationIndex` (0 for the return value, 1 for the first parameter, …).
 */
AttributeSet VuoCompilerCodeGenUtilities::copyAttributesToIndex(AttributeSet attributesToCopy, int destinationIndex)
{
	if (attributesToCopy.getNumSlots() > 1)
		VUserLog("Warning: I was expecting an AttributeSet with 0 or 1 slots, but got %d.", attributesToCopy.getNumSlots());

	int inputIndex = AttributeSet::ReturnIndex + 1;

	string attributeString = attributesToCopy.getAsString(inputIndex);
	if (!attributeString.empty()
		&& attributeString != "byval align 8"
		&& attributeString != "byval align 16")
		VUserLog("Warning: I don't know how to handle all the attributes in '%s'.", attributeString.c_str());

	AttrBuilder builder;

	if (attributesToCopy.hasAttribute(inputIndex, Attribute::ByVal))
		builder.addAttribute(Attribute::ByVal);

	if (attributesToCopy.hasAttribute(inputIndex, Attribute::Alignment))
		builder.addAlignmentAttr(attributesToCopy.getParamAlignment(inputIndex));

	return AttributeSet::get(getGlobalContext(), destinationIndex, builder);
}
