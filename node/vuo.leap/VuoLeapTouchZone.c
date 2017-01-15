/**
 * @file
 * VuoLeapTouchZone implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoLeapTouchZone.h"
#include "VuoList_VuoLeapTouchZone.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Leap Touch Zone",
					 "description" : "How close a pointable is to the touch zone.",
					 "keywords" : [ "leap", "pointable" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoLeapTouchZone"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoLeapTouchZone
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoLeapTouchZone.
 */
VuoLeapTouchZone VuoLeapTouchZone_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoLeapTouchZone value = VuoLeapTouchZone_None;

	if( !strcmp(valueAsString, "touching") )
		value = VuoLeapTouchZone_Touching;
	else if( !strcmp(valueAsString, "hovering"))
		value = VuoLeapTouchZone_Hovering;
	else
		value = VuoLeapTouchZone_None;

	return value;
}

/**
 * @ingroup VuoLeapTouchZone
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapTouchZone_getJson(const VuoLeapTouchZone value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoLeapTouchZone_None:
			valueAsString = "none";
			break;

		case VuoLeapTouchZone_Hovering:
			valueAsString = "hovering";
			break;

		case VuoLeapTouchZone_Touching:
			valueAsString = "touching";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoLeapTouchZone VuoLeapTouchZone_getAllowedValues(void)
{
	VuoList_VuoLeapTouchZone l = VuoListCreate_VuoLeapTouchZone();
	VuoListAppendValue_VuoLeapTouchZone(l, VuoLeapTouchZone_None);
	VuoListAppendValue_VuoLeapTouchZone(l, VuoLeapTouchZone_Hovering);
	VuoListAppendValue_VuoLeapTouchZone(l, VuoLeapTouchZone_Touching);
	return l;
}

/**
 * @ingroup VuoLeapTouchZone
 * Same as @c %VuoLeapTouchZone_getString()
 */
char * VuoLeapTouchZone_getSummary(const VuoLeapTouchZone value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoLeapTouchZone_None:
			valueAsString = "None";
			break;

		case VuoLeapTouchZone_Hovering:
			valueAsString = "Hovering";
			break;

		case VuoLeapTouchZone_Touching:
			valueAsString = "Touching";
			break;
	}
	return strdup(valueAsString);
}
