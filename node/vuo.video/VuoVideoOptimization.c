/**
 * @file
 * VuoVideoOptimization implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVideoOptimization.h"
#include "VuoList_VuoVideoOptimization.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Video Playback Optimization",
					 "description" : "VuoVideoOptimization Type Enum.",
					 "keywords" : [ "access", "decode", "decoder", "ffmpeg", "avfoundation", "seek", "scrub" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoVideoOptimization"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoVideoOptimization
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoVideoOptimization.
 */
VuoVideoOptimization VuoVideoOptimization_makeFromJson(json_object *js)
{
	const char *valueAsString = "";

	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoVideoOptimization value = VuoVideoOptimization_Forward;

	if( !strcmp(valueAsString, "auto"))
		value = VuoVideoOptimization_Auto;
	else if( !strcmp(valueAsString, "forward"))
		value = VuoVideoOptimization_Forward;
	else if( !strcmp(valueAsString, "random"))
		value = VuoVideoOptimization_Random;

	return value;
}

/**
 * @ingroup VuoVideoOptimization
 * Encodes @c value as a JSON object.
 */
json_object * VuoVideoOptimization_getJson(const VuoVideoOptimization value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoVideoOptimization_Forward:
			valueAsString = "forward";
			break;

		case VuoVideoOptimization_Random:
			valueAsString = "random";
			break;

		default:
			valueAsString = "auto";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoVideoOptimization VuoVideoOptimization_getAllowedValues(void)
{
	VuoList_VuoVideoOptimization l = VuoListCreate_VuoVideoOptimization();
	VuoListAppendValue_VuoVideoOptimization(l, VuoVideoOptimization_Auto);
	VuoListAppendValue_VuoVideoOptimization(l, VuoVideoOptimization_Forward);
	VuoListAppendValue_VuoVideoOptimization(l, VuoVideoOptimization_Random);
	return l;
}
/**
 * @ingroup VuoVideoOptimization
 * Same as @c %VuoVideoOptimization_getString()
 */
char * VuoVideoOptimization_getSummary(const VuoVideoOptimization value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoVideoOptimization_Forward:
			valueAsString = "Forward";
			break;

		case VuoVideoOptimization_Random:
			valueAsString = "Random";
			break;

		default:
			valueAsString = "Auto";
			break;
	}

	return strdup(valueAsString);
}
