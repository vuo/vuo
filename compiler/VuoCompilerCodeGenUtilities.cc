/**
 * @file
 * VuoCompilerCodeGenUtilities implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerEventPortClass.hh"
#include "VuoCompilerNodeArgumentClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoStringUtilities.hh"

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
		Type *triggerFunctionType = portContextType->getElementType(4);

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
		Type *dispatchQueueType = portContextType->getElementType(2);

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
		Type *dispatchSemaphoreType = portContextType->getElementType(3);

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
		Type *triggerFunctionType = portContextType->getElementType(4);

		FunctionType *functionType = FunctionType::get(triggerFunctionType, pointerToPortContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *triggerFunctionAsVoidPointer = CallInst::Create(function, portContextValue, "", block);
	PointerType *pointerToTriggerFunctionType = PointerType::get(functionType, 0);
	return new BitCastInst(triggerFunctionAsVoidPointer, pointerToTriggerFunctionType, "", block);
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
 * Generates code that sets the `portContexts` field of a NodeContext.
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
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(portContextsArrayValue);
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
	Type *voidPointerType = nodeContextType->getElementType(1);

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
		Type *eventIdType = nodeContextType->getElementType(3);

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
 * Generates code that sets the `executingEventId` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param executingEventIdValue The value to set the field to.
 */
void VuoCompilerCodeGenUtilities::generateSetNodeContextExecutingEventId(Module *module, BasicBlock *block, Value *nodeContextValue, Value *executingEventIdValue)
{
	const char *functionName = "vuoSetNodeContextExecutingEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *eventIdType = nodeContextType->getElementType(5);

		vector<Type *> params;
		params.push_back(pointerToNodeContext);
		params.push_back(eventIdType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(executingEventIdValue);
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
		Type *voidPointerType = nodeContextType->getElementType(1);

		FunctionType *functionType = FunctionType::get(voidPointerType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Value *pointerToInstanceDataAsVoidPointer = CallInst::Create(function, nodeContextValue, "", block);
	return new BitCastInst(pointerToInstanceDataAsVoidPointer, PointerType::get(instanceDataType, 0), "", block);
}

/**
 * Generates code that gets the `semaphore` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoGetNodeContextSemaphore";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *dispatchSemaphoreType = nodeContextType->getElementType(2);

		FunctionType *functionType = FunctionType::get(dispatchSemaphoreType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, nodeContextValue, "", block);
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
		Type *eventIdType = nodeContextType->getElementType(3);

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
		Type *dispatchGroupType = nodeContextType->getElementType(4);

		FunctionType *functionType = FunctionType::get(dispatchGroupType, pointerToNodeContext, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	return CallInst::Create(function, nodeContextValue, "", block);
}

/**
 * Generates code that gets the `executingEventId` field of a NodeContext.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @return The value of the field.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingEventId(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	const char *functionName = "vuoGetNodeContextExecutingEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		StructType *nodeContextType = getNodeContextType(module);
		PointerType *pointerToNodeContext = PointerType::get(nodeContextType, 0);
		Type *eventIdType = nodeContextType->getElementType(5);

		FunctionType *functionType = FunctionType::get(eventIdType, pointerToNodeContext, false);
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
 * @param portCount The number of elements in the node context's `portContexts` field.
 */
void VuoCompilerCodeGenUtilities::generateResetNodeContextEvents(Module *module, BasicBlock *block, Value *nodeContextValue, size_t portCount)
{
	IntegerType *sizeType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoResetNodeContextEvents";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);
		params.push_back(sizeType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(ConstantInt::get(sizeType, portCount));
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that frees a `NodeContext *` and its fields, including the port contexts within its `portContexts` field.
 *
 * The generated code does not release the data pointed to by the `instanceData` field, if any.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param nodeContextValue A value of type `NodeContext *`.
 * @param portContextCount The number of elements in the node context's `portContexts` field.
 */
void VuoCompilerCodeGenUtilities::generateFreeNodeContext(Module *module, BasicBlock *block, Value *nodeContextValue, size_t portContextCount)
{
	IntegerType *sizeType = IntegerType::get(module->getContext(), 64);

	const char *functionName = "vuoFreeNodeContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> params;
		params.push_back(pointerToNodeContextType);
		params.push_back(sizeType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeContextValue);
	args.push_back(ConstantInt::get(sizeType, portContextCount));
	CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrives the `data` field in a port's context, given the port's identifier.
 */
Value * VuoCompilerCodeGenUtilities::generateGetDataForPort(Module *module, BasicBlock *block,
															Value *compositionIdentifierValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetDataForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> params;
		params.push_back(pointerToCharType);
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(voidPointerType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionIdentifierValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrives the `semaphore` field in a node's context, given the identifier of a port on the node.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeSemaphoreForPort(Module *module, BasicBlock *block,
																	 Value *compositionIdentifierValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetNodeSemaphoreForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *dispatchSemaphoreType = getDispatchSemaphoreType(module);

		vector<Type *> params;
		params.push_back(pointerToCharType);
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(dispatchSemaphoreType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionIdentifierValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrieves the index (in VuoCompilerBitcodeGenerator::orderedNodes) of a node, given the identifier of a port on the node.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeIndexForPort(Module *module, BasicBlock *block,
																 Value *compositionIdentifierValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetNodeIndexForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToCharType);
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(unsignedLongType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionIdentifierValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that retrieves the index (in VuoCompilerBitcodeGenerator::orderedTypes) of a port's type, given the port's identifier.
 */
Value * VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(Module *module, BasicBlock *block,
																 Value *compositionIdentifierValue, Value *portIdentifierValue)
{
	const char *functionName = "vuoGetTypeIndexForPort";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToCharType);
		params.push_back(pointerToCharType);

		FunctionType *functionType = FunctionType::get(unsignedLongType, params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionIdentifierValue);
	args.push_back(portIdentifierValue);
	return CallInst::Create(function, args, "", block);
}

/**
 * Generates code that stores information about the port, indexed on the port's identifier, so it can be efficiently retrieved later.
 */
void VuoCompilerCodeGenUtilities::generateAddPortIdentifier(Module *module, BasicBlock *block,
															Value *compositionIdentifierValue, Value *portIdentifierValue,
															Value *portDataVariable, Value *nodeSemaphoreValue,
															Value *nodeIndexValue, Value *typeIndexValue)
{
	const char *functionName = "vuoAddPortIdentifier";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *dispatchSemaphoreType = getDispatchSemaphoreType(module);
		Type *unsignedLongType = IntegerType::get(module->getContext(), 64);

		vector<Type *> params;
		params.push_back(pointerToCharType);
		params.push_back(pointerToCharType);
		params.push_back(voidPointerType);
		params.push_back(dispatchSemaphoreType);
		params.push_back(unsignedLongType);
		params.push_back(unsignedLongType);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(compositionIdentifierValue);
	args.push_back(portIdentifierValue);
	args.push_back(portDataVariable);
	args.push_back(nodeSemaphoreValue);
	args.push_back(nodeIndexValue);
	args.push_back(typeIndexValue);
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
 * Generates code that gets the address of the array element at the given index.
 */
Value * VuoCompilerCodeGenUtilities::generateGetArrayElementVariable(Module *module, BasicBlock *block, Value *arrayValue, size_t elementIndex)
{
	ConstantInt *indexValue = ConstantInt::get(module->getContext(), APInt(32, elementIndex));
	return GetElementPtrInst::Create(arrayValue, indexValue, "", block);
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
 * @param constantStrings The cache of LLVM constants used to generate string values.
 */
void VuoCompilerCodeGenUtilities::generateStringMatchingCode(Module *module, Function *function,
															 BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue,
															 map<string, pair<BasicBlock *, BasicBlock *> > blocksForString,
															 VuoCompilerConstantStringCache &constantStrings)
{
	Function *strcmpFunction = getStrcmpFunction(module);
	BasicBlock *currentBlock = initialBlock;

	for (map<string, pair<BasicBlock *, BasicBlock *> >::iterator i = blocksForString.begin(); i != blocksForString.end(); ++i)
	{
		string currentString = i->first;
		BasicBlock *firstTrueBlock = i->second.first;
		BasicBlock *lastTrueBlock = i->second.second;

		Constant *currentStringValue = constantStrings.get(module, currentString);

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
 * @param constantStrings The cache of LLVM constants used to generate string values.
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues,
															 VuoCompilerConstantStringCache &constantStrings)
{
	Function *snprintfFunction = getSnprintfFunction(module);

	Type *charType = IntegerType::get(module->getContext(), 8);
	PointerType *pointerToCharType = PointerType::get(charType, 0);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToCharType);
	ConstantInt *zeroValue64 = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *oneValue64 = ConstantInt::get(module->getContext(), APInt(64, 1));

	// int bufferLength = snprintf(NULL, 0, format, ...) + 1;
	Constant *formatStringValue = constantStrings.get(module, formatString);
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
 * @param constantStrings The cache of LLVM constants used to generate string values.
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate,
																 VuoCompilerConstantStringCache &constantStrings)
{
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	if (stringsToConcatenate.empty())
	{
		return constantStrings.get(module, "");
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

	if (valueToCast->getType()->isIntegerTy() && typeToCastTo->isPointerTy())
		return generateTypeCastFromIntegerToPointer(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isFloatingPointTy() && typeToCastTo->isPointerTy())
		return generateTypeCastFromFloatingPointToPointer(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isPointerTy() && typeToCastTo->isIntegerTy())
		return generateTypeCastFromPointerToInteger(module, block, valueToCast, typeToCastTo);
	else if (valueToCast->getType()->isPointerTy() && typeToCastTo->isFloatingPointTy())
		return generateTypeCastFromPointerToFloatingPoint(module, block, valueToCast, typeToCastTo);
	else if (typeToCastTo->isStructTy())
		return generateTypeCastFromLoweredTypeToStruct(block, valueToCast, typeToCastTo);
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
 * @param constantStrings The cache of LLVM constants used to generate string values.
 */
void VuoCompilerCodeGenUtilities::generateAnnotation(Module *module, BasicBlock *block, Value *value,
													 string annotation, string fileName, unsigned int lineNumber,
													 VuoCompilerConstantStringCache &constantStrings)
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

	Constant *fileNamePointer = constantStrings.get(module, fileName);
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

	if (valueToCast->getType()->isVectorTy())
	{
		// Struct was lowered to a vector.
		PointerType *pointerToVectorType = PointerType::get(valueToCast->getType(), 0);

		AllocaInst *structVariable = new AllocaInst(typeToCastTo, "", block);
		CastInst *structVariableAsVector = new BitCastInst(structVariable, pointerToVectorType, "", block);
		new StoreInst(valueToCast, structVariableAsVector, false, block);
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

	fprintf(stderr, "Error: Couldn't cast from lowered type to struct:\n");
	originalValueToCast->getType()->dump();  fprintf(stderr, "\n");
	typeToCastTo->dump();  fprintf(stderr, "\n");
	return originalValueToCast;
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

		ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));

		int numElements = static_cast<StructType *>(argument->getType())->getNumElements();
		for (unsigned i = 0; i < numElements; ++i)
		{
			ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(32, i));

			vector<Value *> memberIndices;
			memberIndices.push_back(zeroValue);
			memberIndices.push_back(iValue);
			Value *memberPointer = GetElementPtrInst::Create(structPointer, memberIndices, "", block);
			Value *member = new LoadInst(memberPointer, "", false, block);

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
 * Generates code that creates a string representation of the given value.
 */
Value * VuoCompilerCodeGenUtilities::generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize,
														   VuoCompilerConstantStringCache &constantStrings)
{
	if (valueToSerialize->getType()->isPointerTy())
	{
		vector<Value *> replacementValues;
		replacementValues.push_back(valueToSerialize);
		return generateFormattedString(module, block, "%lx", replacementValues, constantStrings);
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
														  VuoCompilerConstantStringCache &constantStrings)
{
	if (destinationVariable->getType()->isPointerTy())
	{
		// sscanf(stringToUnserialize, "%lx", destinationPointer);
		Value *formatString = constantStrings.get(module, "%lx");
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
 * Generates code that gets the value of the @c isPaused global variable as a comparison value.
 */
ICmpInst * VuoCompilerCodeGenUtilities::generateIsPausedComparison(Module *module, BasicBlock *block)
{
	GlobalVariable *isPausedVariable = getIsPausedVariable(module);
	LoadInst *isPausedValue = new LoadInst(isPausedVariable, "", false, block);
	Constant *zeroValue = ConstantInt::get(static_cast<PointerType *>(isPausedVariable->getType())->getElementType(), 0);
	return new ICmpInst(*block, ICmpInst::ICMP_NE, isPausedValue, zeroValue, "");
}

/**
 * Generates code that gets the return value of the `vuoShouldSendPortDataTelemetry()` function as a comparison value.
 */
ICmpInst * VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(Module *module, BasicBlock *block, string portIdentifier,
																			  VuoCompilerConstantStringCache &constantStrings)
{
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	const char *functionName = "vuoShouldSendPortDataTelemetry";
	Function *shouldSendTelemetryFunction = module->getFunction(functionName);
	if (! shouldSendTelemetryFunction)
	{
		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		shouldSendTelemetryFunction = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Constant *portIdentifierValue = constantStrings.get(module, portIdentifier);

	CallInst *retValue = CallInst::Create(shouldSendTelemetryFunction, portIdentifierValue, "", block);
	Constant *zeroValue = ConstantInt::get(retValue->getType(), 0);
	return new ICmpInst(*block, ICmpInst::ICMP_NE, retValue, zeroValue, "");
}

/**
 * Generates code that checks if `isNodeInBothCompositions()` returns true for @a nodeIdentifier. This function initializes
 * @a trueBlock and @ falseBlock and sets them up to be the blocks executed depending on the return value of `isNodeInBothCompositions()`.
 */
void VuoCompilerCodeGenUtilities::generateIsNodeInBothCompositionsCheck(Module *module, Function *function, string nodeIdentifier,
																		BasicBlock *initialBlock, BasicBlock *&trueBlock, BasicBlock *&falseBlock,
																		VuoCompilerConstantStringCache &constantStrings)
{
	Value *nodeIdentifierValue = constantStrings.get(module, nodeIdentifier);
	Function *isInBothFunction = getIsNodeInBothCompositionsFunction(module);
	CallInst *isInBothValue = CallInst::Create(isInBothFunction, nodeIdentifierValue, "", initialBlock);

	trueBlock = BasicBlock::Create(module->getContext(), "inBothCompositions", function, NULL);
	falseBlock = BasicBlock::Create(module->getContext(), "notInBothCompositions", function, NULL);
	ConstantInt *falseValue = ConstantInt::get(module->getContext(), APInt(32, 0));
	ICmpInst *isInBothIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, isInBothValue, falseValue, "");
	BranchInst::Create(trueBlock, falseBlock, isInBothIsTrue, initialBlock);
}

/**
 * Generates a dummy event ID to represent that no event is claiming a node.
 */
ConstantInt * VuoCompilerCodeGenUtilities::generateNoEventIdConstant(Module *module)
{
	return ConstantInt::get(module->getContext(), APInt(64, 0));
}

/**
 * Generates a call to `vuoAddNodeContext()`.
 */
void VuoCompilerCodeGenUtilities::generateAddNodeContext(Module *module, BasicBlock *block, Value *nodeIdentifierValue, Value *nodeContextValue)
{
	const char *functionName = "vuoAddNodeContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToNodeContextType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeIdentifierValue);
	args.push_back(nodeContextValue);
	CallInst::Create(function, args, "", block);
}

/**
 * Generates a call to `vuoGetNodeContext()`.
 */
Value * VuoCompilerCodeGenUtilities::generateGetNodeContext(Module *module, BasicBlock *block, Value *nodeIdentifierValue)
{
	const char *functionName = "vuoGetNodeContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToNodeContextType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	vector<Value *> args;
	args.push_back(nodeIdentifierValue);
	return CallInst::Create(function, args, "", block);
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

//@{
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
		Type *eventIdType = generateNoEventIdConstant(module)->getType();

		vector<Type *> fields;
		fields.push_back(pointerToPointerToPortContextType);
		fields.push_back(voidPointerType);
		fields.push_back(getDispatchSemaphoreType(module));
		fields.push_back(eventIdType);
		fields.push_back(getDispatchGroupType(module));
		fields.push_back(eventIdType);
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
		fields.push_back(getDispatchQueueType(module));
		fields.push_back(getDispatchSemaphoreType(module));
		fields.push_back(voidPointerType);
		portContextType->setBody(fields, false);
	}

	return portContextType;
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

PointerType * VuoCompilerCodeGenUtilities::getInstanceDataType(Module *module)
{
	return PointerType::get(IntegerType::get(module->getContext(), 8), 0);
}
//@}


//@{
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

Function * VuoCompilerCodeGenUtilities::getCompositionContextInitFunction(Module *module, string moduleKey)
{
	string functionName = "compositionContextInit";
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToNodeContextType = PointerType::get(getNodeContextType(module), 0);

		FunctionType *functionType = FunctionType::get(pointerToNodeContextType, pointerToCharType, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionContextFiniFunction(Module *module, string moduleKey)
{
	string functionName = "compositionContextFini";
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), pointerToCharType, false);
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
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), PointerType::get(getInstanceDataType(module), 0), false);
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
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), PointerType::get(getInstanceDataType(module), 0), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(Module *module, string moduleKey,
																	const vector<VuoPort *> &modelInputPorts,
																	map<VuoPort *, size_t> &indexOfParameter,
																	VuoCompilerConstantStringCache &constantStrings)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceInit", true, false, true, false, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(Module *module, string moduleKey,
																	VuoCompilerConstantStringCache &constantStrings)
{
	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceFini", true, true, false, false, vector<VuoPort *>(), vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStartFunction(Module *module, string moduleKey,
																			const vector<VuoPort *> &modelInputPorts,
																			map<VuoPort *, size_t> &indexOfParameter,
																			VuoCompilerConstantStringCache &constantStrings)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerStart", true, true, false, false, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(),  map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStopFunction(Module *module, string moduleKey,
																		   VuoCompilerConstantStringCache &constantStrings)
{
	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerStop", true, true, false, false, vector<VuoPort *>(), vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeInstanceTriggerUpdateFunction(Module *module, string moduleKey,
																			 const vector<VuoPort *> &modelInputPorts,
																			 map<VuoPort *, size_t> &indexOfParameter,
																			 VuoCompilerConstantStringCache &constantStrings)
{
	map<VuoPort *, size_t> indexOfEventParameter;
	return getNodeFunction(module, moduleKey, "nodeInstanceTriggerUpdate", true, true, false, false, modelInputPorts, vector<VuoPort *>(),
						   map<VuoPort *, json_object *>(), map<VuoPort *, string>(), map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeEventFunction(Module *module, string moduleKey, bool isSubcomposition, bool isStateful,
															 const vector<VuoPort *> &modelInputPorts,
															 const vector<VuoPort *> &modelOutputPorts,
															 const map<VuoPort *, json_object *> &detailsForPorts,
															 const map<VuoPort *, string> &displayNamesForPorts,
															 const map<VuoPort *, string> &defaultValuesForInputPorts,
															 const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
															 map<VuoPort *, size_t> &indexOfParameter,
															 map<VuoPort *, size_t> &indexOfEventParameter,
															 VuoCompilerConstantStringCache &constantStrings)
{
	string functionName = (isStateful ? "nodeInstanceEvent" : "nodeEvent");
	return getNodeFunction(module, moduleKey, functionName, isSubcomposition, isStateful, false, true, modelInputPorts, modelOutputPorts,
						   detailsForPorts, displayNamesForPorts, defaultValuesForInputPorts, eventBlockingForInputPorts,
						   indexOfParameter, indexOfEventParameter, constantStrings);
}

Function * VuoCompilerCodeGenUtilities::getNodeFunction(Module *module, string moduleKey, string functionName,
														bool hasCompositionIdentifierArg, bool hasInstanceDataArg,
														bool hasInstanceDataReturn, bool hasEventOnlyInputArgs,
														const vector<VuoPort *> &modelInputPorts, const vector<VuoPort *> &modelOutputPorts,
														const map<VuoPort *, json_object *> &detailsForPorts,
														const map<VuoPort *, string> &displayNamesForPorts,
														const map<VuoPort *, string> &defaultValuesForInputPorts,
														const map<VuoPort *, VuoPortClass::EventBlocking> &eventBlockingForInputPorts,
														map<VuoPort *, size_t> &indexOfParameter,
														map<VuoPort *, size_t> &indexOfEventParameter,
														VuoCompilerConstantStringCache &constantStrings)
{
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());

	if (! function)
	{
		bool hasDoorEventBlocking = false;
		for (map<VuoPort *, VuoPortClass::EventBlocking>::const_iterator i = eventBlockingForInputPorts.begin(); i != eventBlockingForInputPorts.end(); ++i)
		{
			if (i->second == VuoPortClass::EventBlocking_Door)
			{
				hasDoorEventBlocking = true;
				break;
			}
		}

		set<string> argumentNamesUsed;
		for (vector<VuoPort *>::const_iterator i = modelInputPorts.begin(); i != modelInputPorts.end(); ++i)
			argumentNamesUsed.insert( (*i)->getClass()->getName() );
		for (vector<VuoPort *>::const_iterator i = modelOutputPorts.begin(); i != modelOutputPorts.end(); ++i)
			argumentNamesUsed.insert( (*i)->getClass()->getName() );

		vector<Type *> functionParams;
		map<int, Attributes> functionAttributes;
		map<VuoPort *, bool> hasSecondParam;
		Type *boolType = IntegerType::get(module->getContext(), 1);
		size_t indexInEventFunction = 0;

		if (hasCompositionIdentifierArg)
		{
			PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
			functionParams.push_back(pointerToCharType);
			indexInEventFunction++;
		}

		if (hasInstanceDataArg)
		{
			functionParams.push_back( PointerType::get(getInstanceDataType(module), 0) );
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
				Attributes paramAttributes = type->getCompiler()->getFunctionParameterAttributes();

				functionParams.push_back(paramType);

				// If we're generating a node function for a subcomposition, and the parameter is a struct that
				// would normally be passed "byval", don't actually make it "byval" in the node function.
				// Instead, the Vuo compiler will generate code equivalent to the "byval" semantics.
				//
				// This is a workaround for a bug where LLVM would sometimes give the node function body
				// an invalid value for a "byval" struct argument. https://b33p.net/kosada/node/11386
				if (! (hasCompositionIdentifierArg &&
					   type->getCompiler()->getType()->isStructTy() &&
					   paramAttributes.hasAttribute(Attributes::ByVal)) )
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

				map<VuoPort *, VuoPortClass::EventBlocking>::const_iterator eventBlockingIter = eventBlockingForInputPorts.find(modelInputPort->getBase());
				if (eventBlockingIter != eventBlockingForInputPorts.end() && eventBlockingIter->second != VuoPortClass::EventBlocking_None)
				{
					functionParams.push_back(boolType);

					indexOfEventParameter[modelInputPort->getBase()] = indexInEventFunction++;
				}
			}
			else if (hasEventOnlyInputArgs)
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

					if (hasDoorEventBlocking)
					{
						PointerType *eventParamType = PointerType::get( boolType, 0 );
						functionParams.push_back(eventParamType);

						indexOfEventParameter[modelOutputPort->getBase()] = indexInEventFunction++;
					}
				}
				else
				{
					PointerType *paramType = PointerType::get( boolType, 0 );
					functionParams.push_back(paramType);

					indexOfParameter[modelOutputPort->getBase()] = indexInEventFunction++;
				}
			}
		}

		Type *returnType = (hasInstanceDataReturn ? getInstanceDataType(module) : Type::getVoidTy(module->getContext()));
		FunctionType *functionType = FunctionType::get(returnType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

		for (map<int, Attributes>::iterator i = functionAttributes.begin(); i != functionAttributes.end(); ++i)
		{
			int attributesIndex = i->first + 1;
			Attributes attributes = i->second;
			function->addAttribute(attributesIndex, attributes);
		}


		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);
		Function::arg_iterator argIter = function->arg_begin();

		if (hasCompositionIdentifierArg)
		{
			Value *arg = argIter++;
			arg->setName("compositionIdentifier");
		}

		if (hasInstanceDataArg)
		{
			Value *arg = argIter++;
			arg->setName("instanceData");
			generateAnnotation(module, block, arg, "vuoInstanceData", "", 0, constantStrings);
		}

		for (vector<VuoPort *>::const_iterator i = modelInputPorts.begin(); i != modelInputPorts.end(); ++i)
		{
			VuoPort *modelInputPort = *i;
			VuoType *type = static_cast<VuoCompilerPort *>( modelInputPort->getCompiler() )->getDataVuoType();

			if (type || hasEventOnlyInputArgs)
			{
				Value *arg = argIter++;
				string argName = modelInputPort->getClass()->getName();

				if (argName == "refresh")
				{
					argName = "refresh_";
					generateAnnotation(module, block, arg, "vuoDetails:{\"name\":\"refresh\"}", "", 0, constantStrings);
				}

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
				Value *argWithDetails = arg;

				if (type)
				{
					generateAnnotation(module, block, arg, "vuoInputData", "", 0, constantStrings);
					generateAnnotation(module, block, arg, "vuoType:" + type->getModuleKey(), "", 0, constantStrings);

					map<VuoPort *, string>::const_iterator defaultValueIter = defaultValuesForInputPorts.find(modelInputPort);
					if (defaultValueIter != defaultValuesForInputPorts.end())
						json_object_object_add(details, "default", json_tokener_parse(defaultValueIter->second.c_str()));

					if (hasSecondParam[modelInputPort])
					{
						arg = argIter++;
						arg->setName(argName + ".1");
					}

					if (hasNonDefaultEventBlocking)
					{
						arg = argIter++;
						string uniqueArgName = argName + "Event";
						while (argumentNamesUsed.find(uniqueArgName) != argumentNamesUsed.end())
							uniqueArgName += "_";
						argumentNamesUsed.insert(uniqueArgName);
						arg->setName(uniqueArgName);

						generateAnnotation(module, block, arg, "vuoInputEvent", "", 0, constantStrings);
						generateAnnotation(module, block, arg, "vuoDetails:{\"eventBlocking\":\"" + eventBlockingStr + "\",\"data\":\"" + argName + "\"}",
										   "", 0, constantStrings);
					}
				}
				else if (hasEventOnlyInputArgs)
				{
					generateAnnotation(module, block, arg, "vuoInputEvent", "", 0, constantStrings);

					if (hasNonDefaultEventBlocking)
						json_object_object_add(details, "eventBlocking", json_object_new_string(eventBlockingStr.c_str()));
				}

				string detailsStr = json_object_to_json_string_ext(details, JSON_C_TO_STRING_PLAIN);
				json_object_put(details);
				generateAnnotation(module, block, argWithDetails, "vuoDetails:" + detailsStr, "", 0, constantStrings);
			}
		}

		for (vector<VuoPort *>::const_iterator i = modelOutputPorts.begin(); i != modelOutputPorts.end(); ++i)
		{
			VuoPort *modelOutputPort = *i;

			Value *arg = argIter++;
			string argName = modelOutputPort->getClass()->getName();
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
			generateAnnotation(module, block, arg, "vuoDetails:" + detailsStr, "", 0, constantStrings);

			VuoType *type = static_cast<VuoCompilerPort *>( modelOutputPort->getCompiler() )->getDataVuoType();
			if (modelOutputPort->getClass()->getPortType() == VuoPortClass::triggerPort)
			{
				generateAnnotation(module, block, arg, "vuoOutputTrigger:" + argName, "", 0, constantStrings);
				generateAnnotation(module, block, arg, "vuoType:" + (type ? type->getModuleKey() : "void"), "", 0, constantStrings);
			}
			else
			{
				if (type)
				{
					generateAnnotation(module, block, arg, "vuoOutputData", "", 0, constantStrings);
					generateAnnotation(module, block, arg, "vuoType:" + type->getModuleKey(), "", 0, constantStrings);

					if (hasDoorEventBlocking)
					{
						arg = argIter++;
						string uniqueArgName = argName + "Event";
						while (argumentNamesUsed.find(uniqueArgName) != argumentNamesUsed.end())
							uniqueArgName += "_";
						argumentNamesUsed.insert(uniqueArgName);
						arg->setName(uniqueArgName);

						generateAnnotation(module, block, arg, "vuoOutputEvent", "", 0, constantStrings);
						generateAnnotation(module, block, arg, "vuoDetails:{\"data\":\"" + argName + "\"}", "", 0, constantStrings);
					}
				}
				else
				{
					generateAnnotation(module, block, arg, "vuoOutputEvent", "", 0, constantStrings);
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

Function * VuoCompilerCodeGenUtilities::getWaitForNodeFunction(Module *module, string moduleKey)
{
	string functionName = "compositionWaitForNode";
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *unsignedLongType = IntegerType::get(module->getContext(), 64);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(unsignedLongType);
		functionParams.push_back(unsignedLongType);
		functionParams.push_back(boolType);
		FunctionType *functionType = FunctionType::get(boolType, functionParams, false);
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
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *intType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
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
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Type *boolType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
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

Function * VuoCompilerCodeGenUtilities::getGetPortValueFunction(Module *module)
{
	const char *functionName = "vuoGetPortValue";
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

Function * VuoCompilerCodeGenUtilities::getGetInputPortStringFunction(Module *module)
{
	const char *functionName = "getInputPortString";
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

Function * VuoCompilerCodeGenUtilities::getGetOutputPortStringFunction(Module *module)
{
	const char *functionName = "getOutputPortString";
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

Function * VuoCompilerCodeGenUtilities::getSendNodeExecutionStartedFunction(Module *module)
{
	const char *functionName = "sendNodeExecutionStarted";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSendNodeExecutionFinishedFunction(Module *module)
{
	const char *functionName = "sendNodeExecutionFinished";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(Module *module)
{
	const char *functionName = "sendInputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(Module *module)
{
	const char *functionName = "sendOutputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSendPublishedOutputPortsUpdatedFunction(Module *module)
{
	const char *functionName = "sendPublishedOutputPortsUpdated";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		IntegerType *boolType = IntegerType::get(module->getContext(), 1);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(boolType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSendEventDroppedFunction(Module *module)
{
	const char *functionName = "sendEventDropped";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getGetNextEventIdFunction(Module *module)
{
	const char *functionName = "vuoGetNextEventId";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 64), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getTranscodeToGraphvizIdentifierFunction(Module *module)
{
	const char *functionName = "vuoTranscodeToGraphvizIdentifier";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		FunctionType *functionType = FunctionType::get(pointerToCharType, pointerToCharType, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCompositionSerializeFunction(Module *module, string moduleKey)
{
	string functionName = "compositionSerialize";
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
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

Function * VuoCompilerCodeGenUtilities::getCompositionUnserializeFunction(Module *module, string moduleKey)
{
	string functionName = "compositionUnserialize";
	if (! moduleKey.empty())
		functionName = VuoStringUtilities::prefixSymbolName(functionName, moduleKey);
	Function *function = module->getFunction(functionName.c_str());
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToGraphType = PointerType::get(getGraphvizGraphType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToGraphType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getSerializeFunction(Module *module)
{
	const char *functionName = "vuoSerialize";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getUnserializeFunction(Module *module)
{
	const char *functionName = "vuoUnserialize";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getOpenGraphvizGraphFunction(Module *module)
{
	const char *functionName = "openGraphvizGraph";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToGraphType = PointerType::get(getGraphvizGraphType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToGraphType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCloseGraphvizGraphFunction(Module *module)
{
	const char *functionName = "closeGraphvizGraph";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToGraphType = PointerType::get(getGraphvizGraphType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToGraphType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getGetConstantValueFromGraphvizFunction(Module *module)
{
	const char *functionName = "getConstantValueFromGraphviz";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToGraphType = PointerType::get(getGraphvizGraphType(module), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToGraphType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(pointerToCharType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getIsNodeInBothCompositionsFunction(Module *module)
{
	const char *functionName = "isNodeInBothCompositions";
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
//@}

//@{
/**
 * Returns a GlobalVariable reference, generating code for the declaration if needed.
 */
GlobalVariable * VuoCompilerCodeGenUtilities::getIsPausedVariable(Module *module)
{
	const char *variableName = "isPaused";
	GlobalVariable *isPausedVariable = module->getNamedGlobal(variableName);
	if (! isPausedVariable)
	{
		IntegerType *boolType = IntegerType::get(module->getContext(), 8);
		isPausedVariable = new GlobalVariable(*module, boolType, false, GlobalValue::ExternalLinkage, 0, variableName);
	}
	return isPausedVariable;
}

GlobalVariable * VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(Module *module)
{
	const char *variableName = "vuoTopLevelCompositionIdentifier";
	GlobalVariable *variable = module->getNamedGlobal(variableName);
	if (! variable)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		variable = new GlobalVariable(*module, pointerToCharType, false, GlobalValue::ExternalLinkage, 0, variableName);
	}
	return variable;
}
//@}

/**
 * Guesses the type that the parameter of the function had in the source code, before it was possibly
 * converted ("lowered") to some other type by Clang/LLVM when the module was compiled.
 * (For example, a struct whose members are all floats may be lowered to a vector of floats.)
 */
Type * VuoCompilerCodeGenUtilities::getParameterTypeBeforeLowering(Function *function, Module *module, string typeName)
{
	Type *type = function->getFunctionType()->getParamType(0);
	bool hasSecondType = (function->getFunctionType()->getNumParams() == 2);

	// Parameter was originally a struct, but was lowered to a "byval" pointer-to-struct, a vector, or two parameters.
	if (isParameterPassedByValue(function, 0) || type->isVectorTy() || hasSecondType)
		type = module->getTypeByName("struct." + typeName);

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

	if (unloweredType->isStructTy())
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
 */
Value * VuoCompilerCodeGenUtilities::convertArgumentToParameterType(Value *argument, FunctionType *functionType, int parameterIndex, bool isPassedByValue,
																	Value **secondArgument, Module *module, BasicBlock *block)
{
	if (secondArgument)
		*secondArgument = NULL;

	Type *parameterType = functionType->getParamType(parameterIndex);

	if (argument->getType()->isStructTy())
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
				if (sourceElementType->isStructTy() && destinationElementType->isStructTy())
					return new BitCastInst(argument, parameterType, "", block);
				sourceType = sourceElementType;
				destinationType = destinationElementType;
			}
		}
	}

	if (argument->getType() != parameterType)
	{
		VUserLog("Couldn't convert an argument to the type of parameter %d", parameterIndex);
		functionType->dump();  fprintf(stderr, "\n");
		argument->getType()->dump();  fprintf(stderr, "\n");
		parameterType->dump();  fprintf(stderr, "\n");
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
	AttrListPtr functionAttrs = function->getAttributes();
	Attributes paramAttrs = functionAttrs.getParamAttributes(parameterIndex + 1);
	return paramAttrs.hasAttribute(Attributes::ByVal);
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
	AttrListPtr functionAttrs = function->getAttributes();
	Attributes paramAttrs = functionAttrs.getParamAttributes(1);
	return paramAttrs.hasAttribute(Attributes::StructRet);
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
