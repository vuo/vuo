/**
 * @file
 * VuoVideoFrame implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVideoFrame.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Video Frame",
					  "description" : "An image and timestamp for a single frame of video.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoImage",
						"VuoReal"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "image" : NULL,
 *     "timestamp" : 0.0
 *   }
 * }
 */
VuoVideoFrame VuoVideoFrame_makeFromJson(json_object *js)
{
	VuoVideoFrame value = {NULL,0.0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "image", &o))
		value.image = VuoImage_makeFromJson(o);
	else
		value.image = NULL;

	if (json_object_object_get_ex(js, "timestamp", &o))
		value.timestamp = VuoReal_makeFromJson(o);
	else
		value.timestamp = 0.0;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoVideoFrame_getJson(const VuoVideoFrame value)
{
	json_object *js = json_object_new_object();

	json_object *imageObject = VuoImage_getJson(value.image);
	json_object_object_add(js, "image", imageObject);

	json_object *timestampObject = VuoReal_getJson(value.timestamp);
	json_object_object_add(js, "timestamp", timestampObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoVideoFrame_getSummary(const VuoVideoFrame value)
{
	return VuoText_format("%s<br />Timestamp: %f", VuoImage_getSummary(value.image), value.timestamp );
}

/**
 * Returns true if the frames are identical.
 */
bool VuoVideoFrame_areEqual(VuoVideoFrame value1, VuoVideoFrame value2)
{
	// Maybe this should just check image?
	return ( abs(value1.timestamp - value2.timestamp) < .001 && VuoImage_areEqual(value1.image, value2.image) );
}
