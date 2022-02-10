/**
 * @file
 * VuoMovieImageEncoding implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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

	if( !strcasecmp(valueAsString, "jpeg"))
		value = VuoMovieImageEncoding_JPEG;
	else if( !strcasecmp(valueAsString, "h264"))
		value = VuoMovieImageEncoding_H264;
	else if (!strcasecmp(valueAsString, "prores4444"))
		value = VuoMovieImageEncoding_ProRes4444;
	else if(!strcasecmp(valueAsString, "prores422"))
		value = VuoMovieImageEncoding_ProRes422;
	else if (!strcmp(valueAsString, "hevc"))
		value = VuoMovieImageEncoding_HEVC;
	else if (!strcmp(valueAsString, "hevc-alpha"))
		value = VuoMovieImageEncoding_HEVCAlpha;
	else if(!strcasecmp(valueAsString, "prores422-hq"))
		value = VuoMovieImageEncoding_ProRes422HQ;
	else if(!strcasecmp(valueAsString, "prores422-lt"))
		value = VuoMovieImageEncoding_ProRes422LT;
	else if(!strcasecmp(valueAsString, "prores422-proxy"))
		value = VuoMovieImageEncoding_ProRes422Proxy;

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
			valueAsString = "jpeg";
			break;

		case VuoMovieImageEncoding_H264:
			valueAsString = "h264";
			break;

		case VuoMovieImageEncoding_ProRes4444:
			valueAsString = "prores4444";
			break;

		case VuoMovieImageEncoding_ProRes422:
			valueAsString = "prores422";
			break;

		case VuoMovieImageEncoding_HEVC:
			valueAsString = "hevc";
			break;

		case VuoMovieImageEncoding_HEVCAlpha:
			valueAsString = "hevc-alpha";
			break;

		case VuoMovieImageEncoding_ProRes422HQ:
			valueAsString = "prores422-hq";
			break;

		case VuoMovieImageEncoding_ProRes422LT:
			valueAsString = "prores422-lt";
			break;

		case VuoMovieImageEncoding_ProRes422Proxy:
			valueAsString = "prores422-proxy";
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
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_ProRes422HQ);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_ProRes422LT);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_ProRes422Proxy);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_HEVC);
	VuoListAppendValue_VuoMovieImageEncoding(l, VuoMovieImageEncoding_HEVCAlpha);
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

		case VuoMovieImageEncoding_HEVC:
			valueAsString = "H.265 (HEVC)";
			break;

		case VuoMovieImageEncoding_HEVCAlpha:
			valueAsString = "H.265 (HEVC) with alpha channel";
			break;

		case VuoMovieImageEncoding_ProRes422HQ:
			valueAsString = "ProRes 422 HQ";
			break;

		case VuoMovieImageEncoding_ProRes422LT:
			valueAsString = "ProRes 422 LT";
			break;

		case VuoMovieImageEncoding_ProRes422Proxy:
			valueAsString = "ProRes 422 Proxy";
			break;
	}

	return strdup(valueAsString);
}
