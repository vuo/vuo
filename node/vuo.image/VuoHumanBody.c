/**
 * @file
 * VuoHumanBody implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHumanBody.h"
#include "json-c/json_object.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "Human Body",
	"description": "Coordinates of landmarks on a human body.",
	"keywords": [],
	"version": "1.0.0",
});
#endif
/// @}

/**
 * @ingroup VuoHumanBody
 * Decodes the JSON object `js` to create a new value.
 */
VuoHumanBody VuoHumanBody_makeFromJson(json_object *js)
{
	// Use the json_object itself as the VuoType value.
	VuoRegister(js, (void (*)(void *))json_object_put);
	return (VuoHumanBody)json_object_get(js);
}

/**
 * @ingroup VuoHumanBody
 * Encodes `hb` as a JSON object.
 */
json_object *VuoHumanBody_getJson(const VuoHumanBody hb)
{
	return json_object_get((struct json_object *)hb);
}

/**
 * @ingroup VuoHumanBody
 * Returns a compact string representation of `hb`.
 */
char *VuoHumanBody_getSummary(const VuoHumanBody hb)
{
	struct json_object *b = (struct json_object *)hb;
	if (!b || !json_object_is_type(b, json_type_object))
		return strdup("(no data)");

	return strdup(json_object_to_json_string(b));
}
