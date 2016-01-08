/**
 * @file
 * VuoCompilerNodeArgument interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODEARGUMENT_H
#define VUOCOMPILERNODEARGUMENT_H

#include "VuoCompilerNodeArgumentClass.hh"

class VuoPort;

/**
 * An argument to a node's event and/or init function. For each @c VuoCompilerNodeArgumentClass,
 * there are 0 or more instances of @c VuoCompilerNodeArgument.
 *
 * @see VuoPort
 */
class VuoCompilerNodeArgument : public VuoBaseDetail<VuoPort>
{
private:
	string getVariableName(string nodeInstanceIdentifier);

protected:
	GlobalVariable *variable; ///< The global variable in the generated code in which this argument is stored.

	VuoCompilerNodeArgument(VuoPort * basePort);
	virtual string getVariableBaseName(void);

public:
	virtual void generateAllocation(Module *module, string nodeInstanceIdentifier);
	virtual LoadInst * generateLoad(BasicBlock *block);
	virtual StoreInst * generateStore(Value *value, BasicBlock *block);
	GlobalVariable * getVariable(void);
};


#endif
