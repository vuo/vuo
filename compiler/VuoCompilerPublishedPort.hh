/**
 * @file
 * VuoCompilerPublishedPort interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDPORT_HH
#define VUOCOMPILERPUBLISHEDPORT_HH

#include "VuoCompilerData.hh"
#include "VuoCompilerPort.hh"
#include "VuoPublishedPort.hh"

/**
 * The compiler detail class for @c VuoPublishedPort.
 */
class VuoCompilerPublishedPort : public VuoBaseDetail<VuoPublishedPort>
{
public:
	set<string> getConnectedPortIdentifiers(void);
	virtual VuoPort * getVuoPseudoPort(void) = 0;  ///< Returns the associated port belonging to the composition's published ("vuo.in" or "vuo.out") pseudo-node.

protected:
	VuoCompilerPublishedPort(string name, bool isOutput, const set<VuoCompilerPort *> &connectedPorts=set<VuoCompilerPort *>());

private:
	static VuoType * getTypeForPort(VuoCompilerPort *connectedPort);
	static set<VuoPort *> getBasePorts(set<VuoCompilerPort *>list);
};

#endif // VUOCOMPILERPUBLISHEDPORT_HH
