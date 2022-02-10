/**
 * @file
 * VuoOscInputDevice implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoOscInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "OSC Input Device",
					  "description" : "Information about an OSC input device.",
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
VuoOscInputDevice VuoOscInputDevice_makeFromJson(json_object * js)
{
	return (VuoOscInputDevice){
		VuoJson_getObjectValue(VuoText,    js, "name",      NULL),
		VuoJson_getObjectValue(VuoText,    js, "ipAddress", NULL),
		VuoJson_getObjectValue(VuoInteger, js, "port",      0)
	};
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoOscInputDevice_getJson(const VuoOscInputDevice value)
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
char *VuoOscInputDevice_getSummary(const VuoOscInputDevice value)
{
	const char *nameOrDefault = value.name ? value.name : "Vuo OSC Server";

	if (value.ipAddress && value.port)
		return VuoText_format("<code>%s</code><br>receiving from %s port %lld", nameOrDefault, value.ipAddress, value.port);
	else if (value.ipAddress)
		return VuoText_format("<code>%s</code><br>receiving from %s on an automatically-selected port", nameOrDefault, value.ipAddress);
	else if (value.port)
		return VuoText_format("<code>%s</code><br>receiving from anywhere on port %lld", nameOrDefault, value.port);
	else
		return VuoText_format("<code>%s</code><br>receiving from anywhere on an automatically-selected port", nameOrDefault);
}

/**
 * Creates a new OSC device description.
 */
VuoOscInputDevice VuoOscInputDevice_make(const VuoText name, const VuoText ipAddress, const VuoInteger port)
{
	return (VuoOscInputDevice){name, ipAddress, VuoInteger_clamp(port, 0, 65535)};
}

/**
 * Returns true if the two input devices are equivalent:
 * the name, IP address, and port must match.
 */
bool VuoOscInputDevice_areEqual(const VuoOscInputDevice a, const VuoOscInputDevice b)
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
bool VuoOscInputDevice_isLessThan(const VuoOscInputDevice a, const VuoOscInputDevice b)
{
	VuoType_returnInequality(VuoText,    a.name,      b.name);
	VuoType_returnInequality(VuoText,    a.ipAddress, b.ipAddress);
	VuoType_returnInequality(VuoInteger, a.port,      b.port);
	return false;
}
