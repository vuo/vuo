/**
 * @file
 * VuoCompilerCable interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCABLE_H
#define VUOCOMPILERCABLE_H

#include "VuoBaseDetail.hh"
#include "VuoCable.hh"

class VuoCompilerNode;
class VuoCompilerPort;

/**
 * Represents a connection from a node's output port to a node's input port.
 */
class VuoCompilerCable : public VuoBaseDetail<VuoCable>
{
public:
	VuoCompilerCable(VuoCompilerNode * fromNode, VuoCompilerPort * fromPort, VuoCompilerNode * toNode, VuoCompilerPort * toPort);
	void setAlwaysEventOnly(bool isAlwaysEventOnly);
	bool getAlwaysEventOnly(void);
	void setHidden(bool hidden);
	bool getHidden(void);
	string getGraphvizDeclaration(void);
	bool carriesData(void);
	void generateTransmission(Module *module, BasicBlock *block, Value *toNodeContextValue, Value *toPortContextValue, Value *outputDataValue, bool shouldTransmitEvent=true);

private:
	bool isAlwaysEventOnly;
	bool isHidden;

	static bool portHasData(VuoPort *port);
};

#endif
