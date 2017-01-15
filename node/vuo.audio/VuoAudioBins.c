/**
 * @file
 * VuoAudioBins implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAudioBins.h"
#include "VuoList_VuoAudioBins.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Audio Bins",
					 "description" : "How many audio bins to use.",
					 "keywords" : [ "fft", "analyze", "frequency" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoAudioBins"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoAudioBins
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoAudioBins.
 */
VuoAudioBins VuoAudioBins_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoAudioBins value = VuoAudioBins_127;

	if (!strcmp(valueAsString, "3"))
		value = VuoAudioBins_3;
	else if (!strcmp(valueAsString, "7"))
		value = VuoAudioBins_7;
	else if (!strcmp(valueAsString, "15"))
		value = VuoAudioBins_15;
	else if (!strcmp(valueAsString, "31"))
		value = VuoAudioBins_31;
	else if (!strcmp(valueAsString, "63"))
		value = VuoAudioBins_63;
	else if (!strcmp(valueAsString, "127"))
		value = VuoAudioBins_127;
	else if (!strcmp(valueAsString, "255"))
		value = VuoAudioBins_255;
	else if (!strcmp(valueAsString, "511"))
		value = VuoAudioBins_511;
	else if (!strcmp(valueAsString, "1023"))
		value = VuoAudioBins_1023;
	else if (!strcmp(valueAsString, "2047"))
		value = VuoAudioBins_2047;

	return value;
}

/**
 * @ingroup VuoAudioBins
 * Encodes @c value as a JSON object.
 */
json_object * VuoAudioBins_getJson(const VuoAudioBins value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioBins_3:
			valueAsString = "3";
			break;
		case VuoAudioBins_7:
			valueAsString = "7";
			break;
		case VuoAudioBins_15:
			valueAsString = "15";
			break;
		case VuoAudioBins_31:
			valueAsString = "31";
			break;
		case VuoAudioBins_63:
			valueAsString = "63";
			break;
		case VuoAudioBins_127:
			valueAsString = "127";
			break;
		case VuoAudioBins_255:
			valueAsString = "255";
			break;
		case VuoAudioBins_511:
			valueAsString = "511";
			break;
		case VuoAudioBins_1023:
			valueAsString = "1023";
			break;
		case VuoAudioBins_2047:
			valueAsString = "2047";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoAudioBins VuoAudioBins_getAllowedValues(void)
{
	VuoList_VuoAudioBins l = VuoListCreate_VuoAudioBins();

	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_3);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_7);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_15);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_31);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_63);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_127);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_255);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_511);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_1023);
	VuoListAppendValue_VuoAudioBins(l, VuoAudioBins_2047);

	return l;
}
/**
 * @ingroup VuoAudioBins
 * Same as @c %VuoAudioBins_getString()
 */
char * VuoAudioBins_getSummary(const VuoAudioBins value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoAudioBins_3:
			valueAsString = "3";
			break;
		case VuoAudioBins_7:
			valueAsString = "7";
			break;
		case VuoAudioBins_15:
			valueAsString = "15";
			break;
		case VuoAudioBins_31:
			valueAsString = "31";
			break;
		case VuoAudioBins_63:
			valueAsString = "63";
			break;
		case VuoAudioBins_127:
			valueAsString = "127";
			break;
		case VuoAudioBins_255:
			valueAsString = "255";
			break;
		case VuoAudioBins_511:
			valueAsString = "511";
			break;
		case VuoAudioBins_1023:
			valueAsString = "1023";
			break;
		case VuoAudioBins_2047:
			valueAsString = "2047";
			break;
	}

	return strdup(valueAsString);
}
