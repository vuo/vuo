/**
 * @file
 * VuoCompilerTriggerPort implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTriggerPort.hh"

#include "VuoPort.hh"

/**
 * Creates a trigger port based on @c portClass.
 */
VuoCompilerTriggerPort::VuoCompilerTriggerPort(VuoPort * basePort)
	: VuoCompilerPort(basePort)
{
	function = NULL;
}

/**
 * Does nothing.
 */
void VuoCompilerTriggerPort::generateAllocation(Module *module, string nodeInstanceIdentifier)
{
	this->nodeInstanceIdentifier = nodeInstanceIdentifier;

	// do nothing -- overrides the superclass implementation
}

/**
 * Does nothing.
 */
LoadInst * VuoCompilerTriggerPort::generateLoad(BasicBlock *block)
{
	// do nothing -- overrides the superclass implementation
	return NULL;
}

/**
 * Does nothing.
 */
StoreInst * VuoCompilerTriggerPort::generateStore(Value *value, BasicBlock *block)
{
	// do nothing -- overrides the superclass implementation
	return NULL;
}

/**
 * Sets the function that's called each time the trigger port generates a push.
 */
void VuoCompilerTriggerPort::setFunction(Function *function)
{
	this->function = function;
}

/**
 * Returns the function that's called each time the trigger port generates a push.
 */
Function * VuoCompilerTriggerPort::getFunction(void)
{
	return function;
}

/**
 * Returns the trigger port class of this trigger port.
 */
VuoCompilerTriggerPortClass * VuoCompilerTriggerPort::getClass(void)
{
	return (VuoCompilerTriggerPortClass *)(getBase()->getClass()->getCompiler());
}

/**
 * Returns a unique, consistent identifier for this port.
 *
 * Assumes @c generateAllocation has been called.
 */
string VuoCompilerTriggerPort::getIdentifier(void)
{
	return nodeInstanceIdentifier + "__" + getClass()->getBase()->getName();
}
