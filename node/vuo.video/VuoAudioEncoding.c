/**
 * @file
 * VuoAudioEncoding implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioEncoding.h"
#include "VuoList_VuoAudioEncoding.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Audio Encoding",
					 "description" : "VuoAudioEncoding Type Enum.",
					 "keywords" : [ "PCM", "AAC", "format" ],
					 "version" : "1.0.0",
					 "dependencies" : [
							"VuoList_VuoAudioEncoding"
						 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoAudioEncoding
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoAudioEncoding.
 */
VuoAudioEncoding VuoAudioEncoding_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoAudioEncoding value = VuoAudioEncoding_LinearPCM;

	if( !strcmp(valueAsString, "LinearPCM"))
		value = VuoAudioEncoding_LinearPCM;
	else if( !strcmp(valueAsString, "AAC"))
		value = VuoAudioEncoding_AAC;

	return value;
}

/**
 * @ingroup VuoAudioEncoding
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioEncoding_getJson(const VuoAudioEncoding value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioEncoding_LinearPCM:
			valueAsString = "LinearPCM";
			break;

		case VuoAudioEncoding_AAC:
			valueAsString = "AAC";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoAudioEncoding VuoAudioEncoding_getAllowedValues(void)
{
	VuoList_VuoAudioEncoding l = VuoListCreate_VuoAudioEncoding();
	VuoListAppendValue_VuoAudioEncoding(l, VuoAudioEncoding_LinearPCM);
	VuoListAppendValue_VuoAudioEncoding(l, VuoAudioEncoding_AAC);
	return l;
}
/**
 * @ingroup VuoAudioEncoding
 * Same as @c %VuoAudioEncoding_getString()
 */
char * VuoAudioEncoding_getSummary(const VuoAudioEncoding value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioEncoding_LinearPCM:
			valueAsString = "Linear PCM";
			break;

		case VuoAudioEncoding_AAC:
			valueAsString = "AAC";
			break;
	}

	return strdup(valueAsString);
}
