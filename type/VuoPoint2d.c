/**
 * @file
 * VuoPoint2d implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoPoint2d.h"
#include "VuoReal.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "2D Point",
					 "description" : "A floating-point 2-dimensional Cartesian spatial location.",
					 "keywords" : [ "coordinate" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoPoint2d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1
 *   }
 * }
 */
VuoPoint2d VuoPoint2d_valueFromJson(json_object * js)
{
	VuoPoint2d point = {0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = VuoReal_valueFromJson(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint2d_jsonFromValue(const VuoPoint2d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = VuoReal_jsonFromValue(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = VuoReal_jsonFromValue(value.y);
	json_object_object_add(js, "y", yObject);

	return js;
}

/**
 * @ingroup VuoPoint2d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint2d_summaryFromValue(const VuoPoint2d value)
{
	const char *format = "%g, %g";
	int size = snprintf(NULL,0,format,value.x,value.y);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value.x,value.y);
	return valueAsString;
}
