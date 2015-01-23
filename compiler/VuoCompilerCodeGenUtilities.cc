/**
 * @file
 * VuoCompilerCodeGenUtilities implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"

/**
 * Generates code that allocates a dispatch_semaphore_t.
 *
 * @param module The module in which to generate code.
 * @param identifier The variable name to use for the dispatch_semaphore_t variable.
 * @return The dispatch_semaphore_t variable.
 */
GlobalVariable * VuoCompilerCodeGenUtilities::generateAllocationForSemaphore(Module *module, string identifier)
{
	PointerType *dispatch_semaphore_t_type = getDispatchSemaphoreType(module);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(dispatch_semaphore_t_type);
	GlobalVariable *semaphoreVariable = new GlobalVariable(*module,
														   dispatch_semaphore_t_type,
														   false,
														   GlobalValue::InternalLinkage,
														   nullValue,
														   identifier);
	return semaphoreVariable;
}

/**
 * Generates code that initializes a dispatch_semaphore_t.
 */
void VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, int initialValue)
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
	CallInst *call = CallInst::Create(dispatch_semaphore_create_function, initialValueConst, "", block);
	new StoreInst(call, semaphoreVariable, block);
}

/**
 * Generates code that waits for and claims a dispatch_semaphore_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param semaphoreVariable The semaphore.
 * @param timeoutInNanoseconds The timeout to be turned into a dispatch_time_t and passed to dispatch_semaphore_wait().
 * @return The return value of dispatch_semaphore_wait().
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, int64_t timeoutInNanoseconds)
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
	ConstantInt *deltaValue = ConstantInt::get(module->getContext(), APInt(64, timeoutInNanoseconds, true));

	vector<Value *> args;
	args.push_back(whenValue);
	args.push_back(deltaValue);
	CallInst *timeoutValue = CallInst::Create(dispatch_time_function, args, "", block);

	return generateWaitForSemaphore(module, block, semaphoreVariable, timeoutValue);
}

/**
 * Generates code that waits for and claims a dispatch_semaphore_t, with a timeout of @c DISPATCH_TIME_FOREVER.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param semaphoreVariable The semaphore.
 * @return The return value of dispatch_semaphore_wait().
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable)
{
	ConstantInt *timeoutValue = ConstantInt::get(module->getContext(), APInt(64, DISPATCH_TIME_FOREVER));
	return generateWaitForSemaphore(module, block, semaphoreVariable, timeoutValue);
}

/**
 * Generates code that waits for and claims a dispatch_semaphore_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param semaphoreVariable The semaphore.
 * @param timeoutValue A value of type dispatch_time_t to pass to dispatch_semaphore_wait().
 * @return The return value of dispatch_semaphore_wait().
 */
Value * VuoCompilerCodeGenUtilities::generateWaitForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable, Value *timeoutValue)
{
	PointerType *dispatch_semaphore_t_type = getDispatchSemaphoreType(module);

	vector<Type *> dispatch_semaphore_wait_functionParams;
	dispatch_semaphore_wait_functionParams.push_back(dispatch_semaphore_t_type);
	dispatch_semaphore_wait_functionParams.push_back(IntegerType::get(module->getContext(), 64));
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

	LoadInst *semaphoreValue = new LoadInst(semaphoreVariable, "", false, block);
	vector<Value*> args;
	args.push_back(semaphoreValue);
	args.push_back(timeoutValue);
	return CallInst::Create(dispatch_semaphore_wait_function, args, "", block);
}

/**
 * Generates code that signals a dispatch_semaphore_t.
 */
void VuoCompilerCodeGenUtilities::generateSignalForSemaphore(Module *module, BasicBlock *block, GlobalVariable *semaphoreVariable)
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

	LoadInst *semaphoreValue = new LoadInst(semaphoreVariable, "", false, block);
	vector<Value*> args;
	args.push_back(semaphoreValue);
	CallInst::Create(dispatch_semaphore_signal_function, args, "", block);
}

/**
 * Generates code that allocates a dispatch_group_t.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param identifier The variable name to use for the dispatch_group_t variable.
 * @return The dispatch_group_t variable.
 */
AllocaInst * VuoCompilerCodeGenUtilities::generateAllocationForDispatchGroup(Module *module, BasicBlock *block, string identifier)
{
	PointerType *dispatch_group_t_type = getDispatchGroupType(module);
	AllocaInst *dispatchGroupVariable = new AllocaInst(dispatch_group_t_type, identifier, block);
	return dispatchGroupVariable;
}

/**
 * Generates code that initializes a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateInitializationForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable)
{
	PointerType *dispatch_group_t_type = getDispatchGroupType(module);

	Function *dispatch_group_create_function = module->getFunction("dispatch_group_create");
	if (! dispatch_group_create_function)
	{
		vector<Type *> dispatch_group_create_functionParams;
		FunctionType *dispatch_group_create_functionType = FunctionType::get(dispatch_group_t_type,
																			 dispatch_group_create_functionParams,
																			 false);
		dispatch_group_create_function = Function::Create(dispatch_group_create_functionType,
														  GlobalValue::ExternalLinkage,
														  "dispatch_group_create", module);
	}

	CallInst *call = CallInst::Create(dispatch_group_create_function, "", block);
	new StoreInst(call, dispatchGroupVariable, block);
}

/**
 * Generates code that submits a function for asynchronous execution on the global dispatch queue
 * and associates the function with a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable,
																	 Function *workerFunction, Value *contextValue)
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

	PointerType *dispatch_group_t_type = getDispatchGroupType(module);
	PointerType *pointerToi8 = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> dispatch_group_async_f_functionParams;
	dispatch_group_async_f_functionParams.push_back(dispatch_group_t_type);
	dispatch_group_async_f_functionParams.push_back(dispatch_queue_t_type);
	dispatch_group_async_f_functionParams.push_back(pointerToi8);
	dispatch_group_async_f_functionParams.push_back(workerFunction->getType());
	FunctionType *dispatch_group_async_f_functionType = FunctionType::get(Type::getVoidTy(module->getContext()),
																		  dispatch_group_async_f_functionParams,
																		  false);

	Function *dispatch_group_async_f_function = module->getFunction("dispatch_group_async_f");
	if (! dispatch_group_async_f_function) {
		dispatch_group_async_f_function = Function::Create(dispatch_group_async_f_functionType,
														   GlobalValue::ExternalLinkage,
														   "dispatch_group_async_f",
														   module);
	}

	ConstantInt *zeroi64Value = ConstantInt::get(module->getContext(), APInt(64, 0));

	// Get the global queue.
	AllocaInst *globalQueueVariable = new AllocaInst(dispatch_queue_t_type, "globalQueue", block);
	{
		vector<Value *> args;
		args.push_back(zeroi64Value);
		args.push_back(zeroi64Value);
		CallInst *call = CallInst::Create(dispatch_get_global_queue_function, args, "", block);
		new StoreInst(call, globalQueueVariable, false, block);
	}

	// Call dispatch_group_async_f, adding the worker function to the global queue.
	{
		LoadInst *dispatchGroupValue = new LoadInst(dispatchGroupVariable, "", false, block);
		LoadInst *globalQueueValue = new LoadInst(globalQueueVariable, "", false, block);
		vector<Value *> args;
		args.push_back(dispatchGroupValue);
		args.push_back(globalQueueValue);
		args.push_back(contextValue);
		args.push_back(workerFunction);
		CallInst::Create(dispatch_group_async_f_function, args, "", block);
	}
}

/**
 * Generates code that waits on a dispatch_group_t.
 */
void VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(Module *module, BasicBlock *block, AllocaInst *dispatchGroupVariable, dispatch_time_t timeout)
{
	PointerType *dispatch_group_t_type = getDispatchGroupType(module);

	vector<Type *> dispatch_group_wait_functionParams;
	dispatch_group_wait_functionParams.push_back(dispatch_group_t_type);
	dispatch_group_wait_functionParams.push_back(IntegerType::get(module->getContext(), 64));
	FunctionType *dispatch_group_wait_functionType = FunctionType::get(IntegerType::get(module->getContext(), 64),
																	   dispatch_group_wait_functionParams,
																	   false);

	Function *dispatch_group_wait_function = module->getFunction("dispatch_group_wait");
	if (! dispatch_group_wait_function) {
		dispatch_group_wait_function = Function::Create(dispatch_group_wait_functionType,
														GlobalValue::ExternalLinkage,
														"dispatch_group_wait",
														module);
	}

	ConstantInt *timeout_value = ConstantInt::get(module->getContext(), APInt(64, timeout, true));

	LoadInst *dispatchGroupValue = new LoadInst(dispatchGroupVariable, "", false, block);
	vector<Value *> args;
	args.push_back(dispatchGroupValue);
	args.push_back(timeout_value);
	CallInst::Create(dispatch_group_wait_function, args, "", block);
}

/**
 * Generates code that allocates a dispatch_queue_t.
 *
 * @param module The module in which to generate code.
 * @param identifier The variable name to use for the dispatch_queue_t variable.
 * @return The dispatch_queue_t variable.
 */
GlobalVariable * VuoCompilerCodeGenUtilities::generateAllocationForDispatchQueue(Module *module, string identifier)
{
	PointerType *dispatch_queue_t_type = getDispatchQueueType(module);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(dispatch_queue_t_type);
	GlobalVariable *dispatchQueueVariable = new GlobalVariable(*module,
															   dispatch_queue_t_type,
															   false,
															   GlobalValue::InternalLinkage,
															   nullValue,
															   identifier);
	return dispatchQueueVariable;
}

/**
 * Generates code that initializes a dispatch_queue_t.
 */
void VuoCompilerCodeGenUtilities::generateInitializationForDispatchQueue(Module *module, BasicBlock *block,
																		 GlobalVariable *dispatchQueueVariable, string dispatchQueueName)
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
																   GlobalValue::PrivateLinkage,
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
	CallInst *call = CallInst::Create(dispatch_queue_create_function, args, "", block);
	new StoreInst(call, dispatchQueueVariable, block);
}

/**
 * Generates code that submits a function for asynchronous execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable,
																				Function *workerFunction, Value *contextValue)
{
	generateSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, contextValue, false);
}

/**
 * Generates code that submits a function for synchronous execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable,
																			   Function *workerFunction, Value *contextValue)
{
	generateSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, contextValue, true);
}

/**
 * Generates code that submits a function for execution on a dispatch queue.
 */
void VuoCompilerCodeGenUtilities::generateSubmissionToDispatchQueue(Module *module, BasicBlock *block, GlobalVariable *dispatchQueueVariable,
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

	Value *dispatchQueueValue = new LoadInst(dispatchQueueVariable, "", false, block);

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
	GlobalVariable *global = new GlobalVariable(*module, arrayType, true, GlobalValue::PrivateLinkage, 0, "");
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
 * @param blocksForString For each key string, the first block and last block to execute if the input string matches that string. The caller is responsible for branching (directly or indirectly) from the first to the last block.
 */
void VuoCompilerCodeGenUtilities::generateStringMatchingCode(Module *module, Function *function,
															 BasicBlock *initialBlock, BasicBlock *finalBlock, Value *inputStringValue,
															 map<string, pair<BasicBlock *, BasicBlock *> > blocksForString)
{
	Function *strcmpFunction = VuoCompilerCodeGenUtilities::getStrcmpFunction(module);
	BasicBlock *currentBlock = initialBlock;

	for (map<string, pair<BasicBlock *, BasicBlock *> >::iterator i = blocksForString.begin(); i != blocksForString.end(); ++i)
	{
		string currentString = i->first;
		BasicBlock *firstTrueBlock = i->second.first;
		BasicBlock *lastTrueBlock = i->second.second;

		Constant *currentStringValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, currentString);

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
 * Generates code that allocates a buffer to hold the composite string, then calls @c snprintf to combine the format string
 * and replacement values into the composite string.
 *
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @param formatString The format string to be passed to @c snprintf.
 * @param replacementValues The replacement values to be passed to @c snprintf.
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateFormattedString(Module *module, BasicBlock *block, string formatString, vector<Value *> replacementValues)
{
	Function *snprintfFunction = getSnprintfFunction(module);

	Type *charType = IntegerType::get(module->getContext(), 8);
	PointerType *pointerToCharType = PointerType::get(charType, 0);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToCharType);
	ConstantInt *zeroValue64 = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *oneValue64 = ConstantInt::get(module->getContext(), APInt(64, 1));

	// int bufferLength = snprintf(NULL, 0, format, ...) + 1;
	Constant *formatStringValue = generatePointerToConstantString(module, formatString);
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
 * @return A value containing the address of the composite string.
 */
Value * VuoCompilerCodeGenUtilities::generateStringConcatenation(Module *module, BasicBlock *block, vector<Value *> stringsToConcatenate)
{
	if (stringsToConcatenate.empty())
		return generatePointerToConstantString(module, "");

	Type *charType = IntegerType::get(module->getContext(), 8);
	PointerType *pointerToCharType = PointerType::get(charType, 0);
	ConstantInt *zeroValue64 = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *zeroValue8 = ConstantInt::get(module->getContext(), APInt(8, 0));
	ConstantInt *oneValue64 = ConstantInt::get(module->getContext(), APInt(64, 1));

	// int bufferLength = strlen(string0) + strlen(string1) + ... + 1;
	Value *bufferLengthValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	Function *strlenFunction = getStrlenFunction(module);
	for (vector<Value *>::iterator i = stringsToConcatenate.begin(); i != stringsToConcatenate.end(); ++i)
	{
		Value *stringToConcatenate = *i;
		CallInst *stringLength = CallInst::Create(strlenFunction, stringToConcatenate, "", block);
		bufferLengthValue = BinaryOperator::Create(Instruction::Add, bufferLengthValue, stringLength, "", block);
	}
	bufferLengthValue = BinaryOperator::Create(Instruction::Add, bufferLengthValue, oneValue64, "", block);

	// buffer = malloc(bufferLength);
	AllocaInst *bufferVariable = new AllocaInst(pointerToCharType, "buffer", block);
	Value *bufferValue = generateMemoryAllocation(module, block, charType, bufferLengthValue);
	new StoreInst(bufferValue, bufferVariable, false, block);

	// buffer[0] = 0;
	GetElementPtrInst *elementPointer = GetElementPtrInst::Create(bufferValue, zeroValue64, "", block);
	new StoreInst(zeroValue8, elementPointer, false, block);

	// strcat(buffer, string0);
	// strcat(buffer, string1);
	// ...
	Function *strcatFunction = getStrcatFunction(module);
	for (vector<Value *>::iterator i = stringsToConcatenate.begin(); i != stringsToConcatenate.end(); ++i)
	{
		Value *stringToConcatenate = *i;
		vector<Value *> strcatArgs;
		strcatArgs.push_back(bufferValue);
		strcatArgs.push_back(stringToConcatenate);
		CallInst::Create(strcatFunction, strcatArgs, "", block);
	}

	return bufferValue;
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
 */
void VuoCompilerCodeGenUtilities::generateAnnotation(Module *module, BasicBlock *block, Value *value,
													 string annotation, string fileName, unsigned int lineNumber)
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

	Constant *fileNamePointer = generatePointerToConstantString(module, fileName);
	annotateFunctionArgs.push_back(fileNamePointer);

	ConstantInt *lineNumberValue = ConstantInt::get(module->getContext(), APInt(32, lineNumber));
	annotateFunctionArgs.push_back(lineNumberValue);

	CallInst::Create(annotateFunction, annotateFunctionArgs, "", block);
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

	fprintf(stderr, "Couldn't cast from lowered type to struct:\n");
	originalValueToCast->getType()->dump();  fprintf(stderr, "\n");
	typeToCastTo->dump();  fprintf(stderr, "\n");
	return originalValueToCast;
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

		Value *valueCasted = VuoCompilerCodeGenUtilities::generateTypeCast(module, block, argument, voidPointerType);
		CallInst::Create(function, valueCasted, "", block);
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
 * Generates code that creates a string representation of the given value.
 */
Value * VuoCompilerCodeGenUtilities::generateSerialization(Module *module, BasicBlock *block, Value *valueToSerialize)
{
	if (valueToSerialize->getType()->isPointerTy())
	{
		vector<Value *> replacementValues;
		replacementValues.push_back(valueToSerialize);
		return generateFormattedString(module, block, "%lx", replacementValues);
	}
	else
	{
		/// @todo Handle other primitive types and structs (https://b33p.net/kosada/node/3942)
		fprintf(stderr, "Couldn't serialize non-pointer value\n");
		return NULL;
	}
}

/**
 * Generates code that creates a value of the given type from the given string representation,
 * and stores it in the destination variable.
 */
void VuoCompilerCodeGenUtilities::generateUnserialization(Module *module, BasicBlock *block, Value *stringToUnserialize, GlobalVariable *destination)
{
	if (destination->getType()->isPointerTy())
	{
		// sscanf(stringToUnserialize, "%lx", destinationPointer);
		Value *formatString = generatePointerToConstantString(module, "%lx");
		Function *sscanfFunction = getSscanfFunction(module);
		vector<Value *> sscanfArgs;
		sscanfArgs.push_back(stringToUnserialize);
		sscanfArgs.push_back(formatString);
		sscanfArgs.push_back(destination);
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
 * Generates code that prints a string. (Useful for debugging.)
 */
void VuoCompilerCodeGenUtilities::generatePrint(Module *module, BasicBlock *block, string stringToPrint)
{
	Constant *stringValue = generatePointerToConstantString(module, stringToPrint, "stringToPrint");

	Function *putsFunction = getPutsFunction(module);
	CallInst::Create(putsFunction, stringValue, "", block);
}

/**
 * Generates code that prints a value. (Useful for debugging.)
 */
void VuoCompilerCodeGenUtilities::generatePrint(Module *module, BasicBlock *block, string formatString, Value *value)
{
	Value *formatStringValue = generatePointerToConstantString(module, formatString);

	Function *printfFunction = getPrintfFunction(module);
	vector<Value *> printfArgs;
	printfArgs.push_back(formatStringValue);
	printfArgs.push_back(value);
	CallInst::Create(printfFunction, printfArgs, "", block);
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

Function * VuoCompilerCodeGenUtilities::getPrintfFunction(Module *module)
{
	const char *functionName = "printf";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
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

Function * VuoCompilerCodeGenUtilities::getCallbackStartFunction(Module *module)
{
	const char *functionName = "nodeInstanceTriggerStart";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getCallbackStopFunction(Module *module)
{
	const char *functionName = "nodeInstanceTriggerStop";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getInitializeReferenceCountsFunction(Module *module)
{
	const char *functionName = "VuoHeap_init";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getFinalizeReferenceCountsFunction(Module *module)
{
	const char *functionName = "VuoHeap_fini";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
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
		functionParams.push_back(voidPointerType);
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
		functionParams.push_back(voidPointerType);
		FunctionType *functionType = FunctionType::get(intType, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}
	return function;
}

Function * VuoCompilerCodeGenUtilities::getGetInputPortValueFunction(Module *module)
{
	const char *functionName = "getInputPortValue";
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

Function * VuoCompilerCodeGenUtilities::getGetInputPortValueThreadUnsafeFunction(Module *module)
{
	const char *functionName = "getInputPortValueThreadUnsafe";
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

Function * VuoCompilerCodeGenUtilities::getGetOutputPortValueFunction(Module *module)
{
	const char *functionName = "getOutputPortValue";
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

Function * VuoCompilerCodeGenUtilities::getGetOutputPortValueThreadUnsafeFunction(Module *module)
{
	const char *functionName = "getOutputPortValueThreadUnsafe";
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

Function * VuoCompilerCodeGenUtilities::getGetInputPortSummaryFunction(Module *module)
{
	const char *functionName = "getInputPortSummary";
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

Function * VuoCompilerCodeGenUtilities::getGetOutputPortSummaryFunction(Module *module)
{
	const char *functionName = "getOutputPortSummary";
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

Function * VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(Module *module)
{
	const char *functionName = "setInputPortValue";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(pointerToCharType);
		functionParams.push_back(IntegerType::get(module->getContext(), 32));
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
//@}

/**
 * Guesses the type that the function parameter had in the source code, before it was possibly
 * converted ("lowered") to some other type by Clang/LLVM when the module was compiled.
 * (For example, a struct whose members are all floats may be lowered to a vector of floats.)
 */
Type * VuoCompilerCodeGenUtilities::getParameterTypeBeforeLowering(Function *function, int parameterIndex, Module *module, string typeName)
{
	Type *type = function->getFunctionType()->getParamType(parameterIndex);

	// Parameter was originally a struct, but was lowered to a "byval" pointer-to-struct or a vector.
	if (isParameterPassedByValue(function, parameterIndex) || type->isVectorTy())
		type = module->getTypeByName("struct." + typeName);

	return type;
}

/**
 * If needed, generates code to convert the argument to the (possibly "lowered") type(s) of the function parameter(s).
 *
 * @param argument The argument to be converted.
 * @param function The function the argument is being passed to.
 * @param parameterIndex The index of the first (and possibly only) function parameter corresponding to @c argument.
 * @param[out] secondArgument Pointer to the second converted argument, to be passed to the second function parameter corresponding to @c argument. If none, pass NULL.
 * @param module The module in which to generate code.
 * @param block The block in which to generate code.
 * @return The first converted argument, to be passed to the first (and possibly only) function parameter corresponding to @c argument. May be the same as @c argument.
 */
Value * VuoCompilerCodeGenUtilities::convertArgumentToParameterType(Value *argument, Function *function, int parameterIndex,
																	Value **secondArgument, Module *module, BasicBlock *block)
{
	if (secondArgument)
		*secondArgument = NULL;

	Type *parameterType = function->getFunctionType()->getParamType(parameterIndex);

	// Argument is a struct...
	if (argument->getType()->isStructTy())
	{
		if (! secondArgument)
		{
			// ... lowered to a vector.
			if (parameterType->isVectorTy())
			{
				PointerType *pointerToVectorType = PointerType::get(parameterType, 0);

				Value *structVariable = generatePointerToValue(block, argument);
				CastInst *structVariableAsVector = new BitCastInst(structVariable, pointerToVectorType, "", block);
				return new LoadInst(structVariableAsVector, "", false, block);
			}
		}
		// ... lowered to two parameters (e.g. two vectors).
		else
		{
			Type *secondParameterType = function->getFunctionType()->getParamType(parameterIndex + 1);
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
	}

	// Argument is passed by value.
	if (isParameterPassedByValue(function, parameterIndex))
		argument = generatePointerToValue(block, argument);

	// Argument type doesn't match parameter type because they're structs loaded from different modules.
	if (argument->getType() != parameterType)
	{
		Type *possiblePointerType = argument->getType();
		while (possiblePointerType->isPointerTy())
		{
			PointerType *pointerType = static_cast<PointerType *>(possiblePointerType);
			Type *elementType = pointerType->getElementType();
			if (elementType->isStructTy())
				argument = new BitCastInst(argument, parameterType, "", block);
			possiblePointerType = elementType;
		}
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
