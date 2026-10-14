/**
 * @file
 * VuoHumanHand implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHumanHand.h"
#include "json-c/json_object.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "Human Hand",
	"description": "Coordinates of landmarks on a human hand.",
	"keywords": [],
	"version": "1.0.0",
});
#endif
/// @}

/**
 * @ingroup VuoHumanHand
 * Decodes the JSON object `js` to create a new value.
 */
VuoHumanHand VuoHumanHand_makeFromJson(json_object *js)
{
	// Use the json_object itself as the VuoType value.
	VuoRegister(js, (void (*)(void *))json_object_put);
	return (VuoHumanHand)json_object_get(js);
}

/**
 * @ingroup VuoHumanHand
 * Encodes `h` as a JSON object.
 */
json_object *VuoHumanHand_getJson(const VuoHumanHand h)
{
	return json_object_get((struct json_object *)h);
}

/**
 * @ingroup VuoHumanHand
 * Returns a compact string representation of `h`.
 */
char *VuoHumanHand_getSummary(const VuoHumanHand h)
{
	struct json_object *jh = (struct json_object *)h;
	if (!jh || !json_object_is_type(jh, json_type_object))
		return strdup("(no data)");

	return strdup(json_object_to_json_string(jh));
}
