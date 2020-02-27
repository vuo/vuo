/**
 * @file
 * VuoCompilerPublishedPort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerPort.hh"

class VuoPort;

/**
 * The compiler detail class for @c VuoPublishedPort.
 */
class VuoCompilerPublishedPort : public VuoCompilerPort
{
public:
	static VuoCompilerPublishedPort * newPort(string name, VuoType *type);
	VuoCompilerPublishedPort(VuoPort *basePort);
	string getIdentifier(void);
	void setInitialValue(string initialValueAsString);
	string getInitialValue(void);
	json_object * getDetails(bool isInput);
	string getGraphvizAttributes(void);
	Value * generateCreatePortContext(Module *module, BasicBlock *block);
};
