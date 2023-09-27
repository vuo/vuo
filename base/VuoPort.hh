/**
 * @file
 * VuoPort interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBase.hh"
#include "VuoPortClass.hh"

class VuoCompilerNodeArgument;
class VuoRendererPort;
class VuoCable;

/**
 * A port instance on a @c VuoNode instance.
 *
 * @see VuoCompilerPort
 * @see VuoRendererPort
 */
class VuoPort : public VuoBase<VuoCompilerNodeArgument,VuoRendererPort>
{
public:
	VuoPort(VuoPortClass *portClass);
	virtual ~VuoPort(void);  ///< to make this class dynamic_cast-able

	VuoPortClass * getClass(void);

	vector<VuoCable *> getConnectedCables(bool includePublishedCables=true);
	void addConnectedCable(VuoCable *cable);
	void removeConnectedCable(VuoCable *cable);
	VuoCable * getCableConnecting(VuoPort *otherPort);
	VuoPortClass::EventThrottling getEventThrottling(void);
	void setEventThrottling(VuoPortClass::EventThrottling eventThrottling);

	void setRawInitialValue(const string &rawInitialValue);
	string getRawInitialValue(void);

	void print(void);

private:
	VuoPortClass *portClass; ///< The port class this port is an instance of.
	vector<VuoCable *> connectedCables;
	VuoPortClass::EventThrottling eventThrottling; ///< The port's default event-throttling behavior. Only applies to trigger ports.

	string rawInitialValue;
};
