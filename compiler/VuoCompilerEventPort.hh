/**
 * @file
 * VuoCompilerEventPort interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEREVENTPORT_H
#define VUOCOMPILEREVENTPORT_H

#include "VuoCompilerPort.hh"
#include "VuoCompilerEventPortClass.hh"
#include "VuoCompilerData.hh"

/**
 * A passive (non-trigger) port, optionally with data.
 */
class VuoCompilerEventPort : public VuoCompilerPort
{
protected:
	VuoCompilerData *data; ///< Optional data stored in this port.

	VuoCompilerEventPort(VuoPort *basePort);
	Constant * getBoolConstant(bool value);
	string getVariableBaseName(void);

public:
	void generateAllocation(Module *module, string nodeInstanceIdentifier);
	StoreInst * generateStore(Value *value, BasicBlock *block);
	StoreInst * generateStore(bool pushed, BasicBlock *block);
	GlobalVariable * getDataVariable(void);
	string getIdentifier(void);

	/**
	 * Returns this port's data, or NULL if none.
	 */
	virtual VuoCompilerData * getData(void) = 0;

	VuoCompilerType * getDataType(void);
};

#endif
