/**
 * @file
 * VuoRange implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoRange.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Range",
					 "description" : "Defines a range with an optionally bound/unbound min/max.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoReal"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoRange
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     	"minimum" : 0.0,
 *		"maximum" : 42.0
 *   }
 * }
 */
VuoRange VuoRange_makeFromJson(json_object * js)
{
	VuoRange range = VuoRange_make(VuoRange_NoMinimum, VuoRange_NoMaximum);

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "minimum", &o))
		range.minimum = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "maximum", &o))
		range.maximum = VuoReal_makeFromJson(o);

	return range;
}

/**
 * @ingroup VuoRange
 * Encodes @c value as a JSON object.
 */
json_object * VuoRange_getJson(const VuoRange value)
{
	json_object *js = json_object_new_object();

	if (value.minimum != VuoRange_NoMinimum)
	{
		json_object *cutoffObject = VuoReal_getJson(value.minimum);
		json_object_object_add(js, "minimum", cutoffObject);
	}

	if (value.maximum != VuoRange_NoMaximum)
	{
		json_object *cropObject = VuoReal_getJson(value.maximum);
		json_object_object_add(js, "maximum", cropObject);
	}

	return js;
}


/**
 * @ingroup VuoRange
 * Returns a compact string representation of @c value.
 */
char * VuoRange_getSummary(const VuoRange value)
{
	if (value.minimum != VuoRange_NoMinimum && value.maximum != VuoRange_NoMaximum)
		return VuoText_format("%g to %g", value.minimum, value.maximum);
	else if (value.minimum != VuoRange_NoMinimum)
		return VuoText_format("%g to ∞", value.minimum);
	else if (value.maximum != VuoRange_NoMaximum)
		return VuoText_format("-∞ to %g", value.maximum);
	else
		return VuoText_format("-∞ to ∞");
}
