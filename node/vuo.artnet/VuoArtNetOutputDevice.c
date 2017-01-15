/**
 * @file
 * VuoArtNetOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoArtNetOutputDevice.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Art-Net Output Device",
					  "description" : "Information about an Art-Net output device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoArtNetInputDevice",
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
VuoArtNetOutputDevice VuoArtNetOutputDevice_makeFromJson(json_object *js)
{
	VuoArtNetOutputDevice value = {NULL, NULL, NULL, 0, 0, 0};
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
json_object *VuoArtNetOutputDevice_getJson(const VuoArtNetOutputDevice value)
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
char *VuoArtNetOutputDevice_getSummary(const VuoArtNetOutputDevice value)
{
	if (value.name && value.ipAddress && value.ethernetAddress)
		return VuoText_format("%s<br>%s (%s)<br>Net %lld, Sub-Net %lld, Universe %lld",
							  value.name,
							  value.ipAddress, value.ethernetAddress,
							  value.address.net, value.address.subNet, value.address.universe);
	else if (value.ipAddress)
		return VuoText_format("%s<br>Net %lld, Sub-Net %lld, Universe %lld",
							  value.ipAddress,
							  value.address.net, value.address.subNet, value.address.universe);
	else
		return VuoText_format("Net %lld, Sub-Net %lld, Universe %lld", value.address.net, value.address.subNet, value.address.universe);
}

/**
 * Creates a new Art-Net device description, for broadcasting.
 */
VuoArtNetOutputDevice VuoArtNetOutputDevice_makeBroadcast(const VuoInteger net, const VuoInteger subNet, const VuoInteger universe)
{
	VuoArtNetOutputDevice d;
	bzero(&d, sizeof(VuoArtNetOutputDevice));
	d.address.net = net;
	d.address.subNet = subNet;
	d.address.universe = universe;
	return d;
}

/**
 * Creates a new Art-Net device description, for sending to a single IP.
 */
VuoArtNetOutputDevice VuoArtNetOutputDevice_makeUnicast(const VuoText ipAddress, const VuoInteger net, const VuoInteger subNet, const VuoInteger universe)
{
	VuoArtNetOutputDevice d;
	bzero(&d, sizeof(VuoArtNetOutputDevice));
	d.ipAddress = ipAddress;
	d.address.net = net;
	d.address.subNet = subNet;
	d.address.universe = universe;
	return d;
}

/**
 * Returns true if the two output devices are equivalent:
 *
 *    - The addresses (Net/Sub-Net/Universe) must match
 *    - The sending mode (broadcast vs. unicast) must match, and if unicast is used, the IP address must match
 *    - The name and ethernetAddress must match
 */
bool VuoArtNetOutputDevice_areEqual(const VuoArtNetOutputDevice value1, const VuoArtNetOutputDevice value2)
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
bool VuoArtNetOutputDevice_isLessThan(const VuoArtNetOutputDevice a, const VuoArtNetOutputDevice b)
{
	if (VuoText_isLessThan(a.name, b.name)) return true;
	if (VuoText_isLessThan(b.name, a.name)) return false;

	if (VuoText_isLessThan(a.ipAddress, b.ipAddress)) return true;
	if (VuoText_isLessThan(b.ipAddress, a.ipAddress)) return false;

	if (VuoText_isLessThan(a.ethernetAddress, b.ethernetAddress)) return true;
	if (VuoText_isLessThan(b.ethernetAddress, a.ethernetAddress)) return false;

	return VuoArtNetAddress_isLessThan(a.address, b.address);
}
