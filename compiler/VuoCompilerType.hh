/**
 * @file
 * VuoCompilerType interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTYPE_HH
#define VUOCOMPILERTYPE_HH

#include "VuoBaseDetail.hh"
#include "VuoType.hh"
#include "VuoCompilerModule.hh"

/**
 * The compiler detail class for @c VuoType.
 */
class VuoCompilerType : public VuoBaseDetail<VuoType>, public VuoCompilerModule  // Order matters: VuoCompilerModule after VuoBaseDetail - http://stackoverflow.com/a/2254306/238387
{
private:
	Function *makeFromJsonFunction;
	Function *getJsonFunction;
	Function *getInterprocessJsonFunction;
	Function *makeFromStringFunction;
	Function *getStringFunction;
	Function *getInterprocessStringFunction;
	Function *getSummaryFunction;
	Function *retainFunction;
	Function *releaseFunction;
	Type *llvmType;

	static bool isType(string typeName, Module *module);
	void parse(void);
	set<string> globalsToRename(void);
	void parseOrGenerateValueFromStringFunction(void);
	void parseOrGenerateStringFromValueFunction(bool isInterprocess);
	void parseOrGenerateRetainOrReleaseFunction(bool isRetain);
	Value * generateFunctionCallWithTypeParameter(Module *module, BasicBlock *block, Value *arg, Function *sourceFunction);

protected:
	VuoCompilerType(string typeName, Module *module);

public:
	static VuoCompilerType * newType(string typeName, Module *module);
	static bool isListType(VuoCompilerType *type);

	Value * generateValueFromStringFunctionCall(Module *module, BasicBlock *block, Value *arg);
	Value * generateStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	Value * generateInterprocessStringFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	Value * generateSummaryFromValueFunctionCall(Module *module, BasicBlock *block, Value *arg);
	void generateRetainCall(Module *module, BasicBlock *block, Value *arg);
	void generateReleaseCall(Module *module, BasicBlock *block, Value *arg);
	Type * getType(void);
	Type * getFunctionParameterType(Type **secondType);
	Attributes getFunctionParameterAttributes(void);
	bool hasInterprocessStringFunction(void);
};

#endif // VUOCOMPILERTYPE_HH
