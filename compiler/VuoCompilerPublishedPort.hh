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
	~VuoCompilerPublishedPort(void);
	set<string> getConnectedPortIdentifiers(void);
	virtual VuoPort * getVuoPseudoPort(void) = 0;  ///< Returns the associated port belonging to the composition's published ("vuo.in" or "vuo.out") pseudo-node.
	json_object * getDetails(void);
	void setDetail(string key, string value);
	void unsetDetail(string key);
	string getGraphvizAttributes(void);

protected:
	VuoCompilerPublishedPort(string name, VuoType *type, bool isOutput, const set<VuoCompilerPort *> &connectedPorts=set<VuoCompilerPort *>());
	struct json_object *details;  ///< Metadata specified in the serialized composition, such as the default value.

private:
	static set<VuoPort *> getBasePorts(set<VuoCompilerPort *>list);
};

#endif // VUOCOMPILERPUBLISHEDPORT_HH
