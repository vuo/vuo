/**
 * @file
 * VuoCompilerPublishedInputPort interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDINPUTPORT_HH
#define VUOCOMPILERPUBLISHEDINPUTPORT_HH

#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerTriggerPort.hh"

/**
 * A published input port.
 */
class VuoCompilerPublishedInputPort : public VuoCompilerPublishedPort
{
private:
	VuoCompilerTriggerPort *triggerPort;

public:
	VuoCompilerPublishedInputPort(string name, VuoType *type, const set<VuoCompilerPort *> &connectedPorts, VuoCompilerTriggerPort *triggerPort);
	VuoPort * getVuoPseudoPort(void);
	VuoCompilerTriggerPort * getTriggerPort(void);
	void setTriggerPort(VuoCompilerTriggerPort *port);
	string getInitialValue(void);
	void setInitialValue(string initialValueAsString);
};

#endif // VUOCOMPILERPUBLISHEDINPUTPORT_HH
