/**
 * @file
 * VuoDurationType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDurationType.h"
#include "VuoList_VuoDurationType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Duration Type",
					 "description" : "Duration Enum.",
					 "keywords" : [ "" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoDurationType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoDurationType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoDurationType.
 */
VuoDurationType VuoDurationType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDurationType value = VuoDurationType_Single;

	if( !strcmp(valueAsString, "single") ) {
		value = VuoDurationType_Single;
	}	else
	if( !strcmp(valueAsString, "until next") ) {
		value = VuoDurationType_UntilNext;
	} 	else
	if( !strcmp(valueAsString, "until reset") ) {
		value = VuoDurationType_UntilReset;
	}
	return value;
}

/**
 * @ingroup VuoDurationType
 * Encodes @c value as a JSON object.
 */
json_object * VuoDurationType_getJson(const VuoDurationType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoDurationType_Single:
			valueAsString = "single";
			break;
		case VuoDurationType_UntilNext:
			valueAsString = "until next";
			break;
		case VuoDurationType_UntilReset:
			valueAsString = "until reset";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoDurationType VuoDurationType_getAllowedValues(void)
{
	VuoList_VuoDurationType l = VuoListCreate_VuoDurationType();
	VuoListAppendValue_VuoDurationType(l, VuoDurationType_Single);
	VuoListAppendValue_VuoDurationType(l, VuoDurationType_UntilNext);
	VuoListAppendValue_VuoDurationType(l, VuoDurationType_UntilReset);
	return l;
}

/**
 * @ingroup VuoDurationType
 * Same as @c %VuoBlendMode_getString()
 */
char * VuoDurationType_getSummary(const VuoDurationType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoDurationType_Single:
			valueAsString = "Single";
			break;
		case VuoDurationType_UntilNext:
			valueAsString = "Until Next";
			break;
		case VuoDurationType_UntilReset:
			valueAsString = "Until Reset";
			break;
	}
	return strdup(valueAsString);
}
