/**
 * @file
 * VuoBlackmagicInputDevice implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoBlackmagicInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Blackmagic Input Device",
	"description" : "Information about a Blackmagic video capture device.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoBlackmagicConnection",
		"VuoBlackmagicVideoMode",
		"VuoDeinterlacing",
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
 *     "subDevice": 0,
 *     "connection": "hdmi",
 *     "videoMode": "ntsc",
 *     "deinterlacing": "alternate",
 *   }
 * }
 */
VuoBlackmagicInputDevice VuoBlackmagicInputDevice_makeFromJson(json_object * js)
{
	VuoBlackmagicInputDevice value = {NULL, 0, VuoBlackmagicConnection_Composite, VuoBlackmagicVideoMode_NTSC, VuoDeinterlacing_None};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "subDevice", &o))
		value.subDevice = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "connection", &o))
		value.connection = VuoBlackmagicConnection_makeFromJson(o);

	if (json_object_object_get_ex(js, "videoMode", &o))
		value.videoMode = VuoBlackmagicVideoMode_makeFromJson(o);

	if (json_object_object_get_ex(js, "deinterlacing", &o))
		value.deinterlacing = VuoDeinterlacing_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoBlackmagicInputDevice_getJson(const VuoBlackmagicInputDevice value)
{
	json_object *js = json_object_new_object();

	if (value.name)
		json_object_object_add(js, "name", VuoText_getJson(value.name));

	if (value.subDevice)
		json_object_object_add(js, "subDevice", VuoInteger_getJson(value.subDevice));

	json_object_object_add(js, "connection", VuoBlackmagicConnection_getJson(value.connection));

	json_object_object_add(js, "videoMode", VuoBlackmagicVideoMode_getJson(value.videoMode));

	json_object_object_add(js, "deinterlacing", VuoDeinterlacing_getJson(value.deinterlacing));

	return js;
}

/**
 * Returns a string representation of @c value.
 */
char *VuoBlackmagicInputDevice_getSummary(const VuoBlackmagicInputDevice value)
{
	char *connection = VuoBlackmagicConnection_getSummary(value.connection);
	char *mode = VuoBlackmagicVideoMode_getSummary(value.videoMode);
	char *deinterlacing = VuoDeinterlacing_getSummary(value.deinterlacing);

	char *summary;
	if (value.name)
		summary = VuoText_format("%s<br>Sub-device %lld<br>Connection: %s<br>Mode: %s<br>Deinterlacing: %s",
			value.name, value.subDevice, connection, mode, deinterlacing);
	else
		summary = VuoText_format("First available Blackmagic input device<br>Connection: %s<br>Mode: %s<br>Deinterlacing: %s",
			connection, mode, deinterlacing);

	free(connection);
	free(mode);
	free(deinterlacing);
	return summary;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoBlackmagicInputDevice_getShortSummary(const VuoBlackmagicInputDevice value)
{
	if (value.name)
		return VuoText_format("%s, sub-device %lld", value.name, value.subDevice);
	else
		return strdup("First available Blackmagic input device");
}

/**
 * Creates a new Blackmagic input device description.
 */
VuoBlackmagicInputDevice VuoBlackmagicInputDevice_make(const VuoText name, const VuoInteger subDevice, const VuoBlackmagicConnection connection, const VuoBlackmagicVideoMode videoMode, const VuoDeinterlacing deinterlacing)
{
	return (VuoBlackmagicInputDevice){name, subDevice, connection, videoMode, deinterlacing};
}

/**
 * Returns true if the two input devices are equivalent.
 */
bool VuoBlackmagicInputDevice_areEqual(const VuoBlackmagicInputDevice value1, const VuoBlackmagicInputDevice value2)
{
	if (!VuoText_areEqual(value1.name, value2.name))
		return false;

	if (value1.subDevice != value2.subDevice)
		return false;

	if (!VuoBlackmagicConnection_areEqual(value1.connection, value2.connection))
		return false;

	if (!VuoBlackmagicVideoMode_areEqual(value1.videoMode, value2.videoMode))
		return false;

	if (!VuoDeinterlacing_areEqual(value1.deinterlacing, value2.deinterlacing))
		return false;

	return true;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoBlackmagicInputDevice_isLessThan(const VuoBlackmagicInputDevice a, const VuoBlackmagicInputDevice b)
{
	if (VuoText_isLessThan(a.name, b.name)) return true;
	if (VuoText_isLessThan(b.name, a.name)) return false;

	if (a.subDevice < b.subDevice) return true;
	if (b.subDevice < a.subDevice) return false;

	if (VuoBlackmagicConnection_isLessThan(a.connection, b.connection)) return true;
	if (VuoBlackmagicConnection_isLessThan(b.connection, a.connection)) return false;

	if (VuoBlackmagicVideoMode_isLessThan(a.videoMode, b.videoMode)) return true;
	if (VuoBlackmagicVideoMode_isLessThan(b.videoMode, a.videoMode)) return false;

	if (VuoDeinterlacing_isLessThan(a.deinterlacing, b.deinterlacing)) return true;
	/*if (VuoDeinterlacing_isLessThan(b.deinterlacing, a.deinterlacing))*/ return false;
}
