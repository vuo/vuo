/**
 * @file
 * VuoCompilerPortClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerPortClass.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a port type and creates its corresponding base @c VuoPortClass.
 */
VuoCompilerPortClass::VuoCompilerPortClass(string name, VuoPortClass::PortType portType) :
	VuoCompilerNodeArgumentClass(name, portType)
{
	details = NULL;
}

/**
 * Destructor.
 */
VuoCompilerPortClass::~VuoCompilerPortClass(void)
{
	json_object_put(details);
}

/**
 * Sets details for this port.
 *
 * @eg{
 * {
 *   "name":"Set URL",
 *   "hasPortAction":true
 * }
 * }
 */
void VuoCompilerPortClass::setDetails(struct json_object *details)
{
	this->details = details;
}

/**
 * Returns details for this port, set in @c setDetails().
 */
json_object * VuoCompilerPortClass::getDetails(void)
{
	return details;
}

/**
 * Returns the port class's display name, camel-case expanded, and optionally overridden by the port class `details`.
 */
string VuoCompilerPortClass::getDisplayName(void)
{
	VuoPortClass *portClass = getBase();

	// Look for a name stored within the details of the port class.
	if (details)
	{
		json_object *nameValue = NULL;
		if (json_object_object_get_ex(details, "name", &nameValue))
			return json_object_get_string(nameValue);
	}

	// Failing that, attempt to format the port's C identifier.
	string name = portClass->getName();

	return VuoStringUtilities::expandCamelCase(name);
}
