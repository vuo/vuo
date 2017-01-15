/**
 * @file
 * VuoCompilerPortClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPORTCLASS_H
#define VUOCOMPILERPORTCLASS_H

#include "VuoCompilerNodeArgumentClass.hh"
#include "VuoPortClass.hh"

class VuoCompilerPort;
class VuoPort;
class VuoType;

/**
 * A port type.
 */
class VuoCompilerPortClass : public VuoCompilerNodeArgumentClass
{
protected:
	struct json_object *details;  ///< Metadata specified in the node class implementation, such as the port's display name.

	VuoCompilerPortClass(string name, VuoPortClass::PortType portType, Type *type);

public:
	~VuoCompilerPortClass(void);

	/**
	 * Factory method to construct a @c VuoCompilerPort instance from this port class, and create its corresponding base @c VuoPort.
	 */
	virtual VuoCompilerPort * newPort(void) = 0;

	/**
	 * Factory method to construct a @c VuoCompilerPort instance from this port class, using the pre-existing @c port as its base.
	 */
	virtual VuoCompilerPort * newPort(VuoPort *port) = 0;

	/**
	 * Returns the type of data carried by this port, or null if this port is event-only.
	 */
	virtual VuoType * getDataVuoType(void) = 0;

	/**
	 * Sets the type of data carried by this port. The type must be null if this port is event-only.
	 */
	virtual void setDataVuoType(VuoType *type) = 0;

	/**
	 * Returns the port class's display name, camel-case expanded, and optionally overridden by the port class `details`.
	 */
	virtual string getDisplayName(void);

	void setDetails(struct json_object *details);
	json_object * getDetails(void);
};

#endif
