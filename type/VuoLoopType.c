/**
 * @file
 * VuoLoopType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoLoopType.h"
#include "VuoList_VuoLoopType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Loop Type",
					 "description" : "Loop Enum.",
					 "keywords" : [ "" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoLoopType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoLoopType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoLoopType.
 */
VuoLoopType VuoLoopType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoLoopType value = VuoLoopType_Loop;

	if( !strcmp(valueAsString, "loop") ) {
		value = VuoLoopType_Loop;
	}	else
	if( !strcmp(valueAsString, "mirror") ) {
		value = VuoLoopType_Mirror;
	} 	else
	if( !strcmp(valueAsString, "none") ) {
		value = VuoLoopType_None;
	}
	return value;
}

/**
 * @ingroup VuoLoopType
 * Encodes @c value as a JSON object.
 */
json_object * VuoLoopType_getJson(const VuoLoopType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoLoopType_Loop:
			valueAsString = "loop";
			break;
		case VuoLoopType_Mirror:
			valueAsString = "mirror";
			break;
		case VuoLoopType_None:
			valueAsString = "none";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoLoopType VuoLoopType_getAllowedValues(void)
{
	VuoList_VuoLoopType l = VuoListCreate_VuoLoopType();
	VuoListAppendValue_VuoLoopType(l, VuoLoopType_Loop);
	VuoListAppendValue_VuoLoopType(l, VuoLoopType_Mirror);
	VuoListAppendValue_VuoLoopType(l, VuoLoopType_None);
	return l;
}

/**
 * @ingroup VuoLoopType
 * Same as @c %VuoBlendMode_getString()
 */
char * VuoLoopType_getSummary(const VuoLoopType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoLoopType_Loop:
			valueAsString = "Loop";
			break;
		case VuoLoopType_Mirror:
			valueAsString = "Mirror";
			break;
		case VuoLoopType_None:
			valueAsString = "None";
			break;
	}
	return strdup(valueAsString);
}
