/**
 * @file
 * VuoCompilerTriggerPort interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGERPORT_H
#define VUOCOMPILERTRIGGERPORT_H

#include "VuoCompilerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"

/**
 * A trigger output port.
 *
 * \see{VuoCompilerTriggerPortClass}
 */
class VuoCompilerTriggerPort : public VuoCompilerPort
{
private:
	Function *function;
	string nodeInstanceIdentifier;

public:
	VuoCompilerTriggerPort(VuoPort * basePort);
	void generateAllocation(Module *module, string nodeInstanceIdentifier);
	LoadInst * generateLoad(BasicBlock *block);
	StoreInst * generateStore(Value *value, BasicBlock *block);
	void setFunction(Function *function);
	Function * getFunction(void);
	VuoCompilerTriggerPortClass * getClass(void);
	string getIdentifier(void);
};

#endif
