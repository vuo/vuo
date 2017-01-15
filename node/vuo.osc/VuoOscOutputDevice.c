/**
 * @file
 * VuoOscOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoOscOutputDevice.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "OSC Output Device",
					  "description" : "Information about an OSC output device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoInteger",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "name": "",
 *     "ipAddress": "",
 *     "port": 0
 *   }
 * }
 */
VuoOscOutputDevice VuoOscOutputDevice_makeFromJson(json_object * js)
{
	VuoOscOutputDevice value = {NULL, NULL, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "ipAddress", &o))
		value.ipAddress = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "port", &o))
		value.port = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoOscOutputDevice_getJson(const VuoOscOutputDevice value)
{
	json_object *js = json_object_new_object();

	if (value.name)
		json_object_object_add(js, "name", VuoText_getJson(value.name));

	if (value.ipAddress)
		json_object_object_add(js, "ipAddress", VuoText_getJson(value.ipAddress));

	json_object_object_add(js, "port", VuoInteger_getJson(value.port));

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoOscOutputDevice_getSummary(const VuoOscOutputDevice value)
{
	const char *nameOrDefault = value.name ? value.name : "Vuo OSC Client";

	if (value.ipAddress && value.port)
		return VuoText_format("<code>%s</code><br>sending to %s on port %lld", nameOrDefault, value.ipAddress, value.port);
	else if (value.ipAddress)
		return VuoText_format("<code>%s</code><br>sending to %s on an automatically-selected port", nameOrDefault, value.ipAddress);
	else if (value.port)
		return VuoText_format("<code>%s</code><br>broadcasting on port %lld", nameOrDefault, value.port);
	else
		return VuoText_format("<code>%s</code><br>broadcasting on an automatically-selected port", nameOrDefault);
}

/**
 * Creates a new OSC device description.
 */
VuoOscOutputDevice VuoOscOutputDevice_makeBroadcast(const VuoText name, const VuoInteger port)
{
	return (VuoOscOutputDevice){name, NULL, VuoInteger_clamp(port, 0, 65535)};
}

/**
 * Creates a new OSC device description.
 */
VuoOscOutputDevice VuoOscOutputDevice_makeUnicast(const VuoText name, const VuoText ipAddress, const VuoInteger port)
{
	return (VuoOscOutputDevice){name, ipAddress, VuoInteger_clamp(port, 0, 65535)};
}

/**
 * Returns true if the two output devices are equivalent:
 * the name, IP address, and port must match.
 */
bool VuoOscOutputDevice_areEqual(const VuoOscOutputDevice a, const VuoOscOutputDevice b)
{
	if (!VuoText_areEqual(a.name, b.name))
		return false;

	if (!VuoText_areEqual(a.ipAddress, b.ipAddress))
		return false;

	if (a.port != b.port)
		return false;

	return true;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoOscOutputDevice_isLessThan(const VuoOscOutputDevice a, const VuoOscOutputDevice b)
{
	if (VuoText_isLessThan(a.name, b.name)) return true;
	if (VuoText_isLessThan(b.name, a.name)) return false;

	if (VuoText_isLessThan(a.ipAddress, b.ipAddress)) return true;
	if (VuoText_isLessThan(b.ipAddress, a.ipAddress)) return false;

	if (a.port < b.port) return true;
//	if (b.port < a.port) return false;

	return false;
}
