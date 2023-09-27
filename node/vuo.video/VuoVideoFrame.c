/**
 * @file
 * VuoVideoFrame implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
 *     "duration" : 0.0
 *   }
 * }
 */
VuoVideoFrame VuoVideoFrame_makeFromJson(json_object *js)
{
	return (VuoVideoFrame){
		VuoJson_getObjectValue(VuoImage, js, "image",     NULL),
		VuoJson_getObjectValue(VuoReal,  js, "timestamp", 0),
		VuoJson_getObjectValue(VuoReal,  js, "duration",  0),
	};
}

/**
 * Helper for VuoVideoFrame_getJson and VuoVideoFrame_getInterprocessJson.
 */
json_object *VuoVideoFrame_getJsonInternal(const VuoVideoFrame value, bool isInterprocess)
{
	json_object *js = json_object_new_object();

	json_object *imageObject = isInterprocess
							   ? VuoImage_getInterprocessJson(value.image)
							   : VuoImage_getJson(value.image);
	json_object_object_add(js, "image", imageObject);

	json_object *timestampObject = VuoReal_getJson(value.timestamp);
	json_object_object_add(js, "timestamp", timestampObject);

	json_object *durationObject = VuoReal_getJson(value.duration);
	json_object_object_add(js, "duration", durationObject);

	return js;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoVideoFrame_getJson(const VuoVideoFrame value)
{
	return VuoVideoFrame_getJsonInternal(value, false);
}

/**
 * Returns a JSON object containing an interprocess handle for the specified video frame's texture.
 */
json_object *VuoVideoFrame_getInterprocessJson(const VuoVideoFrame value)
{
	return VuoVideoFrame_getJsonInternal(value, true);
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoVideoFrame_getSummary(const VuoVideoFrame value)
{
	return VuoText_format("%s<br />Timestamp: <tt>%.3f</tt> seconds<br />Frame duration: <tt>%.3f</tt> seconds", VuoImage_getSummary(value.image), value.timestamp, value.duration);
}

/**
 * Returns true if the frames are identical.
 */
bool VuoVideoFrame_areEqual(VuoVideoFrame value1, VuoVideoFrame value2)
{
	// Maybe this should just check image?
	return VuoReal_areEqual(value1.timestamp, value2.timestamp)
		&& VuoReal_areEqual(value1.duration, value2.duration)
		&& VuoImage_areEqual(value1.image, value2.image);
}

/**
 * Returns true if `a < b`.
 * @version200New
 */
bool VuoVideoFrame_isLessThan(const VuoVideoFrame a, const VuoVideoFrame b)
{
	VuoType_returnInequality(VuoReal,  a.timestamp, b.timestamp);
	VuoType_returnInequality(VuoReal,  a.duration,  b.duration);
	VuoType_returnInequality(VuoImage, a.image,     b.image);
	return false;
}
