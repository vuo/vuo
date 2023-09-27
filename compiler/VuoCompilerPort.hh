/**
 * @file
 * VuoCompilerPort interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerNodeArgument.hh"

class VuoCompilerConstantsCache;
class VuoPort;
class VuoType;

/**
 * A port.
 */
class VuoCompilerPort : public VuoCompilerNodeArgument
{
public:
	bool hasConnectedCable(void) const;
	bool hasConnectedDataCable(void) const;
	VuoType * getDataVuoType(void);
	void setDataVuoType(VuoType *dataType);
	void setNodeIdentifier(string nodeIdentifier);
	virtual string getIdentifier(void);
	void setIndexInPortContexts(int indexInPortContexts);
	int getIndexInPortContexts(void);
	void setConstantsCache(VuoCompilerConstantsCache *constantsCache);
	Value * getDataVariable(Module *module, BasicBlock *block, Value *nodeContextValue);
	Value * generateGetPortContext(Module *module, BasicBlock *block, Value *nodeContextValue);

	/**
	 * Generates code that creates a `PortContext *` and initializes it for this port.
	 */
	virtual Value * generateCreatePortContext(Module *module, BasicBlock *block) = 0;

protected:
	VuoCompilerPort(VuoPort * basePort);

	VuoCompilerConstantsCache *constantsCache;  ///< Cache used to generate constant string values.

private:
	VuoType *dataType;
	string nodeIdentifier;
	int indexInPortContexts;
};
