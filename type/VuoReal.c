/**
 * @file
 * VuoReal implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoReal.h"
#include "VuoText.h"
#include <float.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Real",
					 "description" : "A floating-point number.",
					 "keywords" : [ "double", "float", "number" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoSceneObject
 * Decodes the JSON object @c js, expected to contain a double, to create a new value.
 */
VuoReal VuoReal_valueFromJson(json_object * js)
{
	return json_object_get_double(js);
}

/**
 * @ingroup VuoReal
 * Encodes @c value as a JSON object.
 */
json_object * VuoReal_jsonFromValue(const VuoReal value)
{
	return json_object_new_double(value);
}

/**
 * @ingroup VuoReal
 * Returns a string representation of @c value (either decimal or scientific notation, whichever is shorter).
 */
char * VuoReal_summaryFromValue(const VuoReal value)
{
	return VuoText_format("%g", value);
}

/**
 * Returns the minimum of an array of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_min(VuoReal *terms, unsigned long termsCount)
{
	if (termsCount == 0)
		return 0;

	VuoReal min = DBL_MAX;
	for (unsigned long i = 0; i < termsCount; ++i)
		min = MIN(min, terms[i]);

	return min;
}

/**
 * Returns the maximum of an array of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_max(VuoReal *terms, unsigned long termsCount)
{
	if (termsCount == 0)
		return 0;

	VuoReal max = -DBL_MAX;
	for (unsigned long i = 0; i < termsCount; ++i)
		max = MAX(max, terms[i]);

	return max;
}