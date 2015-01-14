/**
 * @file
 * VuoReal implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoReal.h"

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
	const char *format = "%g";
	int size = snprintf(NULL,0,format,value);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value);
	return valueAsString;
}
