/**
 * @file
 * VuoAudioBinAverageType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioBinAverageType.h"
#include "VuoList_VuoAudioBinAverageType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Audio Bin Averaging Type",
					 "description" : "How to average multiple bins together.",
					 "keywords" : [ "fft", "analyze", "frequency" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoAudioBinAverageType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoAudioBinAverageType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoAudioBinAverageType.
 */
VuoAudioBinAverageType VuoAudioBinAverageType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoAudioBinAverageType value = VuoAudioBinAverageType_Quadratic;

	if (!strcmp(valueAsString, "none"))
		value = VuoAudioBinAverageType_None;
	else if (!strcmp(valueAsString, "quadratic"))
		value = VuoAudioBinAverageType_Quadratic;
	else if (!strcmp(valueAsString, "logarithmic"))
		value = VuoAudioBinAverageType_Logarithmic;

	return value;
}

/**
 * @ingroup VuoAudioBinAverageType
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioBinAverageType_getJson(const VuoAudioBinAverageType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioBinAverageType_None:
			valueAsString = "none";
			break;

		case VuoAudioBinAverageType_Quadratic:
			valueAsString = "quadratic";
			break;

		case VuoAudioBinAverageType_Logarithmic:
			valueAsString = "logarithmic";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoAudioBinAverageType VuoAudioBinAverageType_getAllowedValues(void)
{
	VuoList_VuoAudioBinAverageType l = VuoListCreate_VuoAudioBinAverageType();

	VuoListAppendValue_VuoAudioBinAverageType(l, VuoAudioBinAverageType_None);
	VuoListAppendValue_VuoAudioBinAverageType(l, VuoAudioBinAverageType_Quadratic);
	VuoListAppendValue_VuoAudioBinAverageType(l, VuoAudioBinAverageType_Logarithmic);

	return l;
}
/**
 * @ingroup VuoAudioBinAverageType
 * Same as @c %VuoAudioBinAverageType_getString()
 */
char * VuoAudioBinAverageType_getSummary(const VuoAudioBinAverageType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioBinAverageType_None:
			valueAsString = "None";
			break;

		case VuoAudioBinAverageType_Quadratic:
			valueAsString = "Quadratic";
			break;
		case VuoAudioBinAverageType_Logarithmic:
			valueAsString = "Logarithmic";
			break;
	}

	return strdup(valueAsString);
}
