/**
 * @file
 * VuoPublishedPort interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPUBLISHEDPORT_HH
#define VUOPUBLISHEDPORT_HH

#include <set>
#include "VuoBase.hh"
#include "VuoType.hh"

class VuoCompilerPublishedPort;
class VuoRendererPublishedPort;
class VuoPort;

/**
 * This class represents a published port in a composition. Published ports are visible to clients
 * of the composition, for example to a @c VuoRunner running the composition or to another composition in which
 * this composition is a subcomposition.
 *
 * @see VuoCompilerPublishedPort
 * @see VuoRendererPublishedPort
 */
class VuoPublishedPort : public VuoBase<VuoCompilerPublishedPort,VuoRendererPublishedPort>
{
public:
	VuoPublishedPort(string name, VuoType *type, bool isOutput, set<VuoPort *> connectedPorts=set<VuoPort *>());
	string getName(void);
	VuoType * getType(void);
	bool getInput(void) const;
	bool getOutput(void) const;
	set<VuoPort *> getConnectedPorts(void);
	bool hasConnectedPort(VuoPort *port);
	void addConnectedPort(VuoPort *port);
	void removeConnectedPort(VuoPort *port);
	void setName(string name);
	void setType(VuoType *type);

private:
	string name;
	VuoType *type;
	bool isOutput;
	set<VuoPort *> connectedPorts;
	string initialValueAsString;
};

#endif // VUOPUBLISHEDPORT_HH
