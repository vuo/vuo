/**
 * @file
 * VuoMovieImageEncoding implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMovieImageEncoding.h"
#include "VuoList_VuoMovieImageEncoding.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Movie Image Encoding",
					 "description" : "VuoMovieImageEncoding Type Enum.",
					 "keywords" : [ "encoding", "h264", "image", "format" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoMovieImageEncoding"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMovieImageEncoding
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoMovieImageEncoding.
 */
VuoMovieImageEncoding VuoMovieImageEncoding_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoMovieImageEncoding value = VuoMovieImageEncoding_JPEG;

	if( !strcmp(valueAsString, "JPEG"))
		value = VuoMovieImageEncoding_JPEG;
	else if( !strcmp(valueAsString, "H264"))
		value = VuoMovieImageEncoding_H264;
	else if (!strcmp(valueAsString, "ProRes4444"))
		value = VuoMovieImageEncoding_ProRes4444;
	else if(!strcmp(valueAsString, "ProRes422"))
		value = VuoMovieImageEncoding_ProRes422;

	return value;
}

/**
 * @ingroup VuoMovieImageEncoding
 * Encodes @c value as a JSON object.
 */
json_object * VuoMovieImageEncoding_getJson(const VuoMovieImageEncoding value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoMovieImageEncoding_JPEG:
			valueAsString = "JPEG";
			break;

		case VuoMovieImageEncoding_H264:
			valueAsString = "H264";
			break;

		case VuoMovieImageEncoding_ProRes4444:
			valueAsString = "ProRes4444";
			break;

		case VuoMovieImageEncoding_ProRes422:
			valueAsString = "ProRes422";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoMovieImageEncoding VuoMovieImageEncoding_getAllowedValues(void)
{
	VuoList_VuoMovieImageEncoding l = VuoListCreate_VuoMovieImageEncoding();
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_JPEG);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_H264);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_ProRes4444);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_ProRes422);
	return l;
}
/**
 * @ingroup VuoMovieImageEncoding
 * Same as @c %VuoMovieImageEncoding_getString()
 */
char * VuoMovieImageEncoding_getSummary(const VuoMovieImageEncoding value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoMovieImageEncoding_JPEG:
			valueAsString = "JPEG";
			break;

		case VuoMovieImageEncoding_H264:
			valueAsString = "H.264";
			break;

		case VuoMovieImageEncoding_ProRes4444:
			valueAsString = "ProRes 4444";
			break;

		case VuoMovieImageEncoding_ProRes422:
			valueAsString = "ProRes 422";
			break;
	}

	return strdup(valueAsString);
}
