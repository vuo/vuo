/**
 * @file
 * VuoIntegerRange implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoIntegerRange.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Integer Range",
					 "description" : "Defines an integer range with an optionally bound/unbound min/max.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoReal"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoIntegerRange
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     	"minimum" : 0.0,
 *		"maximum" : 42.0
 *   }
 * }
 */
VuoIntegerRange VuoIntegerRange_makeFromJson(json_object * js)
{
	return (VuoIntegerRange){
		VuoJson_getObjectValue(VuoInteger, js, "minimum", VuoIntegerRange_NoMinimum),
		VuoJson_getObjectValue(VuoInteger, js, "maximum", VuoIntegerRange_NoMaximum),
	};
}

/**
 * @ingroup VuoIntegerRange
 * Encodes @c value as a JSON object.
 */
json_object * VuoIntegerRange_getJson(const VuoIntegerRange value)
{
	json_object *js = json_object_new_object();

	if (value.minimum != VuoIntegerRange_NoMinimum)
	{
		json_object *minObject = VuoInteger_getJson(value.minimum);
		json_object_object_add(js, "minimum", minObject);
	}

	if (value.maximum != VuoIntegerRange_NoMaximum)
	{
		json_object *maxObject = VuoInteger_getJson(value.maximum);
		json_object_object_add(js, "maximum", maxObject);
	}

	return js;
}


/**
 * @ingroup VuoIntegerRange
 * Returns a compact string representation of @c value.
 */
char * VuoIntegerRange_getSummary(const VuoIntegerRange value)
{
	if (value.minimum != VuoIntegerRange_NoMinimum && value.maximum != VuoIntegerRange_NoMaximum)
		return VuoText_format("%lld to %lld", value.minimum, value.maximum);
	else if (value.minimum != VuoIntegerRange_NoMinimum)
		return VuoText_format("%lld to ∞", value.minimum);
	else if (value.maximum != VuoIntegerRange_NoMaximum)
		return VuoText_format("-∞ to %lld", value.maximum);
	else
		return VuoText_format("-∞ to ∞");
}
