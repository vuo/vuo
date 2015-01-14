/**
 * @file
 * VuoPort interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPORT_HH
#define VUOPORT_HH

#include "VuoBase.hh"

#include <set>
using namespace std;

class VuoCompilerNodeArgument;
class VuoRendererPort;
class VuoCable;
class VuoPortClass;

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

	VuoPortClass * getClass(void);

	vector<VuoCable *> getConnectedCables(bool includePublishedCables);
	void addConnectedCable(VuoCable *cable);
	void removeConnectedCable(VuoCable *cable);
	VuoCable * getCableConnecting(VuoPort *otherPort);

	void print(void);

private:
	VuoPortClass *portClass; ///< The port class this port is an instance of.
	vector<VuoCable *> connectedCables;
};

#endif // VUOPORT_HH
