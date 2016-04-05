/**
 * @file
 * VuoCompilerNodeArgument implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerNodeArgument.hh"

#include "VuoPort.hh"

/**
 * Creates an argument instance based on the specified @c argumentClass.
 */
VuoCompilerNodeArgument::VuoCompilerNodeArgument(VuoPort *basePort)
	: VuoBaseDetail<VuoPort>("VuoCompilerNodeArgument", basePort)
{
	getBase()->setCompiler(this);
	variable = NULL;
}

/**
 * Generate the allocation of the argument's runtime representation: a global variable to store the argument's value.
 *
 * Assumes @c nodeInstanceIdentifier is not empty.
 */
void VuoCompilerNodeArgument::generateAllocation(Module *module, string nodeInstanceIdentifier)
{
	string variableName = getVariableName(nodeInstanceIdentifier);
	Type *type = getBase()->getClass()->getCompiler()->getType();

	variable = new GlobalVariable(*module,
								  type,
								  false,
								  GlobalValue::PrivateLinkage,
								  0,
								  variableName);

	Constant *initializer = Constant::getNullValue(type);
	variable->setInitializer(initializer);
}

/**
 * Generate code to get the port's value.
 */
LoadInst * VuoCompilerNodeArgument::generateLoad(BasicBlock *block)
{
	return new LoadInst(variable, getVariableBaseName(), block);
}

/**
 * Generate code to set the port's value.
 */
StoreInst * VuoCompilerNodeArgument::generateStore(Value *value, BasicBlock *block)
{
	return new StoreInst(value, variable, block);
}


#pragma mark Getters

/**
 * Returns a name for this argument, which will be a substring of variable names in the generated code.
 */
string VuoCompilerNodeArgument::getVariableBaseName(void)
{
	return getBase()->getClass()->getName();
}

/**
 * Returns the variable name of this argument's global variable.
 *
 * @param nodeInstanceIdentifier An identifier for the node to which this argument belongs. It will be a substring of the variable name.
 * @return The variable name.
 */
string VuoCompilerNodeArgument::getVariableName(string nodeInstanceIdentifier)
{
	return nodeInstanceIdentifier + "__" + getVariableBaseName();
}

/**
 * Returns the global variable to store the argument's value, if @c generateAllocation has been called.
 * Otherwise returns NULL.
 */
GlobalVariable * VuoCompilerNodeArgument::getVariable(void)
{
	return variable;
}
