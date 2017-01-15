/**
 * @file
 * VuoMovieFormat implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMovieFormat.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Movie Format",
					  "description" : "Movie encoding attributes.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudioEncoding",
						  "VuoMovieImageEncoding",
						  "VuoReal",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "imageEncoding" : "",
 *     "imageQuality" : ""
 *     "audioEncoding" : ""
 *     "audioQuality" : ""
 *   }
 * }
 */
VuoMovieFormat VuoMovieFormat_makeFromJson(json_object *js)
{
	VuoMovieFormat value = { VuoMovieImageEncoding_JPEG, 1, VuoAudioEncoding_LinearPCM, 1 };
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "imageEncoding", &o))
		value.imageEncoding = VuoMovieImageEncoding_makeFromJson(o);
	else
		value.imageEncoding = VuoMovieImageEncoding_JPEG;

	if (json_object_object_get_ex(js, "imageQuality", &o))
		value.imageQuality = VuoReal_makeFromJson(o);
	else
		value.imageQuality = 1.f;

	if (json_object_object_get_ex(js, "audioEncoding", &o))
		value.audioEncoding = VuoAudioEncoding_makeFromJson(o);
	else
		value.audioEncoding = VuoAudioEncoding_LinearPCM;

	if (json_object_object_get_ex(js, "audioQuality", &o))
		value.audioQuality = VuoReal_makeFromJson(o);
	else
		value.audioQuality = 1.f;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoMovieFormat_getJson(const VuoMovieFormat value)
{
	json_object *js = json_object_new_object();

	json_object *imageEncoding = VuoMovieImageEncoding_getJson(value.imageEncoding);
	json_object_object_add(js, "imageEncoding", imageEncoding);

	json_object *imageQuality = VuoReal_getJson(value.imageQuality);
	json_object_object_add(js, "imageQuality", imageQuality);

	json_object *audioEncoding = VuoAudioEncoding_getJson(value.audioEncoding);
	json_object_object_add(js, "audioEncoding", audioEncoding);

	json_object *audioQuality = VuoReal_getJson(value.audioQuality);
	json_object_object_add(js, "audioQuality", audioQuality);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoMovieFormat_getSummary(const VuoMovieFormat value)
{
	return VuoText_format("Image encoding: %s<br>Image quality: %g<br>Audio encoding: %s<br>Audio quality: %g",
						  VuoMovieImageEncoding_getSummary(value.imageEncoding),
						  value.imageQuality,
						  VuoAudioEncoding_getSummary(value.audioEncoding),
						  value.audioQuality);
}

/**
 * Returns true if the two movie format specifications are identical.
 */
bool VuoMovieFormat_areEqual(VuoMovieFormat value1, VuoMovieFormat value2)
{
	return (value1.imageEncoding == value2.imageEncoding &&
			VuoReal_areEqual(value1.imageQuality, value2.imageQuality) &&
			value1.audioEncoding == value2.audioEncoding &&
			VuoReal_areEqual(value1.audioQuality, value2.audioQuality)
			);
}
