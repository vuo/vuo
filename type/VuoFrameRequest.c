/**
 * @file
 * VuoFrameRequest implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoFrameRequest.h"
#include "VuoReal.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Frame Request",
					 "description" : "Information about a request for rendering a new graphical frame.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoFrameRequest
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "timestamp" : 0.5,
 *     "frameCount" : 1
 *   }
 * }
 */
VuoFrameRequest VuoFrameRequest_valueFromJson(json_object * js)
{
	VuoFrameRequest fr = {-1,-1};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "timestamp", &o))
		fr.timestamp = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "frameCount", &o))
		fr.frameCount = VuoReal_valueFromJson(o);

	return fr;
}

/**
 * @ingroup VuoFrameRequest
 * Encodes @c value as a JSON object.
 */
json_object * VuoFrameRequest_jsonFromValue(const VuoFrameRequest value)
{
	json_object *js = json_object_new_object();

	json_object *timestampObject = VuoReal_jsonFromValue(value.timestamp);
	json_object_object_add(js, "timestamp", timestampObject);

	json_object *frameCountObject = VuoReal_jsonFromValue(value.frameCount);
	json_object_object_add(js, "frameCount", frameCountObject);

	return js;
}

/**
 * @ingroup VuoFrameRequest
 * Returns a compact string representation of @c value.
 */
char * VuoFrameRequest_summaryFromValue(const VuoFrameRequest value)
{
	const char *format = "Frame #%d @ %g";
	int size = snprintf(NULL,0,format,value.frameCount,value.timestamp);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value.frameCount,value.timestamp);
	return valueAsString;
}
