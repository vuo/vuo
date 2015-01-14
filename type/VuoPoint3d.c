/**
 * @file
 * VuoPoint3d implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoPoint3d.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "3D Point",
					 "description" : "A floating-point 3-dimensional Cartesian spatial location.",
					 "keywords" : [ "coordinate" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoPoint3d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1,
 *     "z" : 0
 *   }
 * }
 */
VuoPoint3d VuoPoint3d_valueFromJson(json_object * js)
{
	VuoPoint3d point = {0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = json_object_get_double(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = json_object_get_double(o);

	if (json_object_object_get_ex(js, "z", &o))
		point.z = json_object_get_double(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint3d_jsonFromValue(const VuoPoint3d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = json_object_new_double(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = json_object_new_double(value.y);
	json_object_object_add(js, "y", yObject);

	json_object *zObject = json_object_new_double(value.z);
	json_object_object_add(js, "z", zObject);

	return js;
}

/**
 * @ingroup VuoPoint3d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint3d_summaryFromValue(const VuoPoint3d value)
{
	const char *format = "%g, %g, %g";
	int size = snprintf(NULL,0,format,value.x,value.y,value.z);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value.x,value.y,value.z);
	return valueAsString;
}
