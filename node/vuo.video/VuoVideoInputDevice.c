/**
 * @file
 * VuoVideoInputDevice implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "VuoVideoInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Video Input Device",
					  "description" : "Information about a video input device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Returns a device matching type constant for the specified identifier string.
 */
static VuoVideoInputDevice_MatchType VuoVideoInputDevice_getMatchTypeForString(const char *string)
{
	if (strcmp(string, "id") == 0)
		return VuoVideoInputDevice_MatchId;

	return VuoVideoInputDevice_MatchIdThenName;
}

/**
 * Returns an identifier string for the specified device matching type.
 */
static const char *VuoVideoInputDevice_getStringForMatchType(VuoVideoInputDevice_MatchType type)
{
	if (type == VuoVideoInputDevice_MatchId)
		return "id";

	return "id-name";
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : "",
 *     "name" : ""
 *   }
 * }
 */
VuoVideoInputDevice VuoVideoInputDevice_makeFromJson(json_object *js)
{
	VuoVideoInputDevice value = {VuoVideoInputDevice_MatchIdThenName, "", ""};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "matchType", &o))
		value.matchType = VuoVideoInputDevice_getMatchTypeForString(json_object_get_string(o));

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoText_makeFromJson(o);
	else
		value.id = VuoText_make("");

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	else
		value.name = VuoText_make("");

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoVideoInputDevice_getJson(const VuoVideoInputDevice value)
{
	json_object *js = json_object_new_object();

	json_object *matchTypeObject = json_object_new_string(VuoVideoInputDevice_getStringForMatchType(value.matchType));
	json_object_object_add(js, "matchType", matchTypeObject);

	json_object *idObject = VuoText_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_getJson(value.name);
	json_object_object_add(js, "name", nameObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoVideoInputDevice_getSummary(const VuoVideoInputDevice value)
{
	if (VuoText_isEmpty(value.name) && VuoText_isEmpty(value.id))
		return strdup("Default device");
	else
	{
		if (value.matchType == VuoVideoInputDevice_MatchId)
			return VuoText_format("Device with ID \"%s\" (\"%s\")", value.id, value.name);
		else
			return VuoText_format("Device with name \"%s\"", value.name);
	}
}

/**
 * Returns true if the two video input device specifications are identical.
 */
bool VuoVideoInputDevice_areEqual(VuoVideoInputDevice value1, VuoVideoInputDevice value2)
{
	return value1.matchType == value2.matchType
		&& VuoText_areEqual(value1.id, value2.id)
		&& VuoText_areEqual(value1.name, value2.name);
}

/**
 * Returns true if `a < b`.
 * @version200New
 */
bool VuoVideoInputDevice_isLessThan(const VuoVideoInputDevice a, const VuoVideoInputDevice b)
{
	VuoType_returnInequality(VuoInteger, a.matchType, b.matchType);
	VuoType_returnInequality(VuoText,    a.id,        b.id);
	VuoType_returnInequality(VuoText,    a.name,      b.name);
	return false;
}
