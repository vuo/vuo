/**
 * @file
 * VuoArtNetInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoArtNetInputDevice.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Art-Net Input Device",
					  "description" : "Information about an Art-Net input device.",
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
 *     "ethernetAddress": "",
 *     "net": 0,
 *     "subNet": 0,
 *     "universe": 0,
 *   }
 * }
 */
VuoArtNetInputDevice VuoArtNetInputDevice_makeFromJson(json_object * js)
{
	VuoArtNetInputDevice value = {NULL, NULL, NULL, 0, 0, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "ipAddress", &o))
		value.ipAddress = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "ethernetAddress", &o))
		value.ethernetAddress = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "net", &o))
		value.address.net = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "subNet", &o))
		value.address.subNet = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "universe", &o))
		value.address.universe = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoArtNetInputDevice_getJson(const VuoArtNetInputDevice value)
{
	json_object *js = json_object_new_object();

	if (value.name)
		json_object_object_add(js, "name", VuoText_getJson(value.name));

	if (value.ipAddress)
		json_object_object_add(js, "ipAddress", VuoText_getJson(value.ipAddress));

	if (value.ethernetAddress)
		json_object_object_add(js, "ethernetAddress", VuoText_getJson(value.ethernetAddress));

	json_object_object_add(js, "net", VuoInteger_getJson(value.address.net));

	json_object_object_add(js, "subNet", VuoInteger_getJson(value.address.subNet));

	json_object_object_add(js, "universe", VuoInteger_getJson(value.address.universe));

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoArtNetInputDevice_getSummary(const VuoArtNetInputDevice value)
{
	if (value.name && value.ipAddress && value.ethernetAddress)
		return VuoText_format("%s<br>%s (%s)<br>Net %lld, Sub-Net %lld, Universe %lld",
							  value.name,
							  value.ipAddress, value.ethernetAddress,
							  value.address.net, value.address.subNet, value.address.universe);
	else
		return VuoText_format("Net %lld, Sub-Net %lld, Universe %lld", value.address.net, value.address.subNet, value.address.universe);
}

/**
 * Creates a new Art-Net device description from an address.
 */
VuoArtNetInputDevice VuoArtNetInputDevice_make(const VuoInteger net, const VuoInteger subNet, const VuoInteger universe)
{
	VuoArtNetInputDevice d;
	bzero(&d, sizeof(VuoArtNetInputDevice));
	d.address.net      = MIN(MAX(net,      0), 127);
	d.address.subNet   = MIN(MAX(subNet,   0),  15);
	d.address.universe = MIN(MAX(universe, 0),  15);
	return d;
}

/**
 * Returns true if the two input devices are equivalent:
 *
 *    - The addresses (Net/Sub-Net/Universe) must match
 *    - The sending mode (broadcast vs. unicast) must match, and if unicast is used, the IP address must match
 *    - The name and ethernetAddress must match
 */
bool VuoArtNetInputDevice_areEqual(const VuoArtNetInputDevice value1, const VuoArtNetInputDevice value2)
{
	if (!VuoArtNetAddress_areEqual(value1.address, value2.address))
		return false;

	if (!VuoText_areEqual(value1.ipAddress, value2.ipAddress))
		return false;

	if (!VuoText_areEqual(value1.name, value2.name))
		return false;

	if (!VuoText_areEqual(value1.ethernetAddress, value2.ethernetAddress))
		return false;

	return true;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoArtNetInputDevice_isLessThan(const VuoArtNetInputDevice a, const VuoArtNetInputDevice b)
{
	if (VuoText_isLessThan(a.name, b.name)) return true;
	if (VuoText_isLessThan(b.name, a.name)) return false;

	if (VuoText_isLessThan(a.ipAddress, b.ipAddress)) return true;
	if (VuoText_isLessThan(b.ipAddress, a.ipAddress)) return false;

	if (VuoText_isLessThan(a.ethernetAddress, b.ethernetAddress)) return true;
	if (VuoText_isLessThan(b.ethernetAddress, a.ethernetAddress)) return false;

	return VuoArtNetAddress_isLessThan(a.address, b.address);
}

/**
 * Returns true if the two addresses are identical.
 */
bool VuoArtNetAddress_areEqual(const VuoArtNetAddress value1, const VuoArtNetAddress value2)
{
	return value1.net      == value2.net
		&& value1.subNet   == value2.subNet
		&& value1.universe == value2.universe;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoArtNetAddress_isLessThan(const VuoArtNetAddress value1, const VuoArtNetAddress value2)
{
	VuoInteger a1 = (value1.net << 8) + (value1.subNet << 4) + value1.universe;
	VuoInteger a2 = (value2.net << 8) + (value2.subNet << 4) + value2.universe;

	return a1 < a2;
}
