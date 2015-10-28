/**
 * @file
 * VuoCompilerPublishedInputNodeClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerCodeGenUtilities.hh"

/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base VuoNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(Module *module)
	: VuoCompilerNodeClass(VuoNodeClass::publishedInputNodeClassName, module)
{
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @a nodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *nodeClass)
	: VuoCompilerNodeClass(nodeClass)
{
}

/**
 * Generates a node class for the published input pseudo-node.
 *
 * @param dummyNodeClass A base node class whose output ports are used as the model for the generated node class.
 * @return The generated node class.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClass(VuoNodeClass *dummyNodeClass)
{
	vector<VuoPortClass *> outputPortClasses = dummyNodeClass->getOutputPortClasses();

	// Add a trigger port that can fire an event for all published input ports simultaneously.
	VuoPortClass *simultaneousTriggerClass = new VuoPortClass(VuoNodeClass::publishedInputNodeSimultaneousTriggerName, VuoPortClass::eventOnlyPort);
	outputPortClasses.push_back(simultaneousTriggerClass);

	Module *module = new Module("", getGlobalContext());
	Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	// const char *moduleDetails = ...;
	string moduleDetails = "{ \"title\" : \"Published Input Ports\" }";
	Constant *moduleDetailsValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, moduleDetails, ".str");  // VuoCompilerBitcodeParser::resolveGlobalToConst requires that the variable have a name
	GlobalVariable *moduleDetailsVariable = new GlobalVariable(*module, pointerToCharType, false, GlobalValue::ExternalLinkage, 0, "moduleDetails");
	moduleDetailsVariable->setInitializer(moduleDetailsValue);

	// void publishedInput...(void);
	vector<Type *> triggerFunctionParams;
	FunctionType *triggerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), triggerFunctionParams, false);
	PointerType *pointerToTriggerFunctionType = PointerType::get(triggerFunctionType, 0);

	// void nodeEvent
	// (
	//  VuoOutputTrigger(publishedInput0,void),
	//  VuoOutputTrigger(publishedInput1,void),
	//  ...
	// ) { }

	vector<Type *> eventFunctionParams(outputPortClasses.size(), pointerToTriggerFunctionType);
	FunctionType *eventFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), eventFunctionParams, false);
	Function *eventFunction = Function::Create(eventFunctionType, GlobalValue::ExternalLinkage, "nodeEvent", module);

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", eventFunction, 0);
	Function::arg_iterator argIter = eventFunction->arg_begin();
	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
	{
		VuoPortClass *outputPortClass = *i;

		Value *arg = argIter++;
		string argName = outputPortClass->getName();
		arg->setName(argName);

		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoType:void", "", 0);
		VuoCompilerCodeGenUtilities::generateAnnotation(module, block, arg, "vuoOutputTrigger:" + outputPortClass->getName(), "", 0);
	}
	ReturnInst::Create(module->getContext(), block);

	VuoCompilerPublishedInputNodeClass *tmpNodeClass = new VuoCompilerPublishedInputNodeClass(module);
	VuoCompilerPublishedInputNodeClass *nodeClass = new VuoCompilerPublishedInputNodeClass(tmpNodeClass);
	delete tmpNodeClass;
	return nodeClass->getBase();
}
