/**
 * @file
 * VuoCompilerCable interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"

class VuoCable;
class VuoCompilerNode;
class VuoCompilerPort;
class VuoPort;

/**
 * Represents a connection from a node's output port to a node's input port.
 */
class VuoCompilerCable : public VuoBaseDetail<VuoCable>
{
public:
	VuoCompilerCable(VuoCompilerNode * fromNode, VuoCompilerPort * fromPort, VuoCompilerNode * toNode, VuoCompilerPort * toPort, bool addCableToPorts = true);
	void setAlwaysEventOnly(bool isAlwaysEventOnly);
	bool getAlwaysEventOnly(void);
	void setHidden(bool hidden);
	bool getHidden(void);
	string getGraphvizDeclaration(void);
	bool carriesData(void);
	void generateTransmission(Module *module, BasicBlock *block, Value *toNodeContextValue, Value *toPortContextValue, Value *outputDataPointer, bool shouldTransmitEvent=true);

private:
	bool isAlwaysEventOnly;
	bool isHidden;

	static bool portHasData(VuoPort *port);
};
