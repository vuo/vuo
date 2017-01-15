/**
 * @file
 * VuoCompilerPublishedPort interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDPORT_HH
#define VUOCOMPILERPUBLISHEDPORT_HH

#include "VuoCompilerPort.hh"

class VuoCompilerPublishedPortClass;
class VuoPort;
class VuoPublishedPort;
class VuoType;

/**
 * The compiler detail class for @c VuoPublishedPort.
 */
class VuoCompilerPublishedPort : public VuoCompilerPort
{
public:
	VuoCompilerPublishedPort(VuoPort *basePort);
	string getIdentifier(void);
	void setInitialValue(string initialValueAsString);
	string getInitialValue(void);
	json_object * getDetails(bool isInput);
	string getGraphvizAttributes(void);
	Value * generateCreatePortContext(Module *module, BasicBlock *block);
};

#endif // VUOCOMPILERPUBLISHEDPORT_HH
