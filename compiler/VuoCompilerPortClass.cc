/**
 * @file
 * VuoCompilerPortClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPortClass.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a port type and creates its corresponding base @c VuoPortClass.
 */
VuoCompilerPortClass::VuoCompilerPortClass(string name, VuoPortClass::PortType portType, Type *type) :
	VuoCompilerNodeArgumentClass(name, portType, type)
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
	if (name == "x")
		return "X";
	else if (name == "y")
		return "Y";
	else if (name == "z")
		return "Z";
	else if (name == "w")
		return "W";
	else if (name == "xy")
		return "XY";
	else if (name == "xyz")
		return "XYZ";
	else if (name == "xyzw")
		return "XYZW";

	string formattedString = VuoStringUtilities::expandCamelCase(name);

	VuoStringUtilities::replaceAll(formattedString, "2d", "2D");
	VuoStringUtilities::replaceAll(formattedString, "3d", "3D");
	VuoStringUtilities::replaceAll(formattedString, "4d", "4D");

	return formattedString;
}
