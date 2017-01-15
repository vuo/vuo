/**
 * @file
 * VuoCompilerEventPort interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEREVENTPORT_H
#define VUOCOMPILEREVENTPORT_H

#include "VuoCompilerPort.hh"

class VuoCompilerData;
class VuoCompilerType;

/**
 * A passive (non-trigger) port, optionally with data.
 */
class VuoCompilerEventPort : public VuoCompilerPort
{
public:
	Value * generateCreatePortContext(Module *module, BasicBlock *block);
	Value * generateLoadEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *portContextValue=NULL);
	void generateStoreEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventValue, Value *portContextValue=NULL);
	void generateStoreEvent(Module *module, BasicBlock *block, Value *nodeContextValue, bool event, Value *portContextValue=NULL);
	Value * generateLoadData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *portContextValue=NULL);
	void generateStoreData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *dataValue);
	void generateReplaceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *dataValue, Value *portContextValue=NULL);

	/**
	 * Returns this port's data, or NULL if none.
	 */
	virtual VuoCompilerData * getData(void) = 0;

	VuoCompilerType * getDataType(void);

protected:
	VuoCompilerData *data; ///< Optional data stored in this port.

	VuoCompilerEventPort(VuoPort *basePort);
};

#endif
