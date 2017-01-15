/**
 * @file
 * VuoCompilerInstanceData interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINSTANCEDATA_H
#define VUOCOMPILERINSTANCEDATA_H

#include "VuoCompilerNodeArgument.hh"

class VuoCompilerInstanceDataClass;

/**
 * The instance data for a node.
 *
 * \see{VuoCompilerInstanceDataClass}
 */
class VuoCompilerInstanceData : public VuoCompilerNodeArgument
{
public:
	VuoCompilerInstanceData(VuoCompilerInstanceDataClass *instanceDataClass);
	Value * generateLoad(Module *module, BasicBlock *block, Value *nodeContextValue);
	void generateStore(Module *module, BasicBlock *block, Value *nodeContextValue, Value *instanceDataValue);
	Value * getVariable(Module *module, BasicBlock *block, Value *nodeContextValue);
};


#endif
