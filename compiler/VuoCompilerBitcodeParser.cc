/**
 * @file
 * VuoCompilerBitcodeParser implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerBitcodeParser.hh"


/**
 * Creates a parser to parse @c module.
 */
VuoCompilerBitcodeParser::VuoCompilerBitcodeParser(Module *module)
{
	this->module = module;
}

/**
 * Returns the value of the global constant of type unsigned int (or bool) with variable name 'name',
 * or 0 if the global constant is not found.
 */
uint64_t VuoCompilerBitcodeParser::getGlobalUInt(string name)
{
	GlobalValue *gv = module->getNamedValue(name);
	if((gv == NULL) || (gv->getValueID() != Value::GlobalVariableVal))
		return 0;

	Value *v = gv->getOperand(0);
	if(v->getValueID() != Value::ConstantIntVal)
		return 0;

	ConstantInt *ci = (ConstantInt *)v;
	return ci->getValue().getLimitedValue();
}

/**
 * Get the value of the global constant of type char* with variable name 'name'.
 */
string VuoCompilerBitcodeParser::getGlobalString(string name)
{
	string s = resolveGlobalToConst(name);
	if(s=="")
		return "";

	return getGlobalValueConstString(s);
}

/**
 * Get the values of the global constants of type char* stored in
 * global array with name 'name'.
 */
vector<string> VuoCompilerBitcodeParser::getStringsFromGlobalArray(string name)
{
	vector<string> globalConstStrings;
	GlobalValue *gv = module->getNamedValue(name);

	if((gv == NULL) || (gv->getValueID() != Value::GlobalVariableVal))
	{
		return globalConstStrings;
	}
	Value *v = gv->getOperand(0);

	if(v->getValueID() != Value::ConstantArrayVal)
	{
		return globalConstStrings;
	}
	ConstantArray *constantArray = (ConstantArray *)v;
	int numArrayElmts = constantArray->getNumOperands();

	for(int i = 0; i < numArrayElmts; i++)
	{
		Constant *arrayConstElmt = (Constant *)(constantArray->getOperand(i));

		if(arrayConstElmt->getValueID() == Value::ConstantExprVal)
		{
			ConstantExpr *ce = (ConstantExpr *)arrayConstElmt;

			if(ce->getOpcode() == Instruction::GetElementPtr)
			{
				// `ConstantExpr` operands: http://llvm.org/docs/LangRef.html#constantexprs
				Value *gv2 = ce->getOperand(0);

				if(gv2->getValueID() == Value::GlobalVariableVal)
				{
					string constantName = gv2->getName().str();
					string constantVal = getGlobalValueConstString(constantName);
					globalConstStrings.push_back(constantVal);
				}
			}
		}
	}
	return globalConstStrings;
}

string VuoCompilerBitcodeParser::resolveGlobalToConst(string name)
{
	GlobalValue *gv = module->getNamedValue(name);
	if((gv == NULL) || (gv->getValueID() != Value::GlobalVariableVal))
		return "";

	Value *v = gv->getOperand(0);
	if(v->getValueID() != Value::ConstantExprVal)
		return "";
	ConstantExpr *ce = (ConstantExpr *)v;
	if(ce->getOpcode() != Instruction::GetElementPtr)
		return "";

	// `ConstantExpr` operands: http://llvm.org/docs/LangRef.html#constantexprs
	Value *gv2 = ce->getOperand(0);

	if(gv2->getValueID() != Value::GlobalVariableVal)
		return "";

	return gv2->getName().str();
}

string VuoCompilerBitcodeParser::getGlobalValueConstString(string name)
{
	GlobalValue *gv = module->getNamedValue(name);

	// assumption: the zeroth operand of a Value::GlobalVariableVal is the actual Value
	Value *v = gv->getOperand(0);
	if(v->getValueID() != Value::ConstantDataArrayVal)
		return "";  // if the string's value is "", v->getValueID() is Value::ConstantAggregateZeroVal

	ConstantDataArray *ca = (ConstantDataArray *)v;
	string caStr = ca->getAsCString().str();
	return caStr;
}

/**
 * Get the function with the given name.
 */
Function * VuoCompilerBitcodeParser::getFunction(string name)
{
	return module->getFunction(name);
}

/**
 * Returns the name of an argument as it would have appeared in the source code of the function,
 * by stripping off the suffix (e.g. ".coerce"), if any.
 */
string VuoCompilerBitcodeParser::getArgumentNameInSourceCode(string argumentNameInBitcode)
{
	return argumentNameInBitcode.substr(0, argumentNameInBitcode.find('.'));
}

/**
 * Returns true if this argument and the argument after it correspond to a single argument
 * in the source code, and were lowered to two arguments when Clang compiled them to bitcode.
 * This happens for some struct types.
 */
bool VuoCompilerBitcodeParser::isFirstOfTwoLoweredArguments(Argument *firstArgument)
{
	Function *function = firstArgument->getParent();
	for (Function::arg_iterator i = function->arg_begin(); i != function->arg_end(); ++i)
	{
		Argument *currArgument = i;
		if (currArgument == firstArgument)
		{
			if (++i != function->arg_end())
			{
				Argument *secondArgument = i;

				string firstArgumentName = getArgumentNameInSourceCode(firstArgument->getName());
				string secondArgumentName = getArgumentNameInSourceCode(secondArgument->getName());
				return (firstArgumentName == secondArgumentName);
			}
			break;
		}
	}
	return false;
}

/**
 * Gets all the function's arguments that have annotations, and associates each argument with its annotation(s).
 */
vector<pair<Argument *, string> > VuoCompilerBitcodeParser::getAnnotatedArguments(Function *function)
{
	vector<pair<Argument *, string> > annotatedArguments;

	// assumption: @llvm.var.annotation calls are always in the function's entry block.
	BasicBlock *b = &function->getEntryBlock();

	// Run through the entry block to associate annotations with function parameters.
	BitCastInst *precedingBitCastInst = NULL;
	for(BasicBlock::iterator it = b->begin();it!=b->end();++it)
	{
		Instruction *inst = it;
		if(! CallInst::classof(inst))
		{
			if (BitCastInst::classof(inst))
				precedingBitCastInst = static_cast<BitCastInst *>(inst);
			continue;
		}

		// assumption: Instruction::Call's operands are the function arguments, followed by the function name
		Value *calledFunction = inst->getOperand(inst->getNumOperands()-1);
		if(calledFunction->getName().str() != "llvm.var.annotation")
			continue;

		// Find an operand whose name has the same prefix as the corresponding function parameter's name.
		// The correct place to look for this operand depends on the data type of the parameter.
		// For a struct, it's the bitcast. For a bool, it's the llvm.var.annotation call.
		// For other data types, either works.
		Value *annotatedValue;
		if (precedingBitCastInst)
		{
			// `bitcast` operands: http://llvm.org/docs/LangRef.html#bitcast-to-instruction
			annotatedValue = precedingBitCastInst->getOperand(0);
			precedingBitCastInst = NULL;
		}
		else
		{
			// `llvm.var.annotation` operands: http://llvm.org/docs/LangRef.html#llvm-var-annotation-intrinsic
			annotatedValue = inst->getOperand(0);
		}

		// Find the first function parameter whose name matches the operand's name.
		// The function parameter and/or the operand may have a suffix that begins with ".", e.g. ".coerce" or ".addr1".
		Argument *argument = NULL;
		string annotationNamePrefix = getArgumentNameInSourceCode(annotatedValue->getName());
		for (Function::arg_iterator i = function->arg_begin(); i != function->arg_end(); ++i)
		{
			Argument *currArgument = i;
			string argNamePrefix = getArgumentNameInSourceCode(currArgument->getName());
			if (argNamePrefix == annotationNamePrefix)
			{
				argument = currArgument;
				break;
			}
		}
		if(!argument)
			continue;

		Value *annotation = inst->getOperand(1);
		if(annotation->getValueID() != Value::ConstantExprVal)
			continue;
		ConstantExpr *ce = (ConstantExpr *)annotation;
		if(ce->getOpcode() != Instruction::GetElementPtr)
			continue;

		// `ConstantExpr` operands: http://llvm.org/docs/LangRef.html#constant-expressions
		Value *gv = ce->getOperand(0);

		if(gv->getValueID() != Value::GlobalVariableVal)
			continue;

		string annotationName = getGlobalValueConstString(gv->getName().str());

		annotatedArguments.push_back(pair<Argument *, string>(argument, annotationName));
	}

	return annotatedArguments;
}
