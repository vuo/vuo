/**
 * @file
 * VuoInteractionType implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Interaction Type",
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoInteractionType"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoInteractionType
 * Decodes the JSON object @a js, expected to contain a string, to create a new VuoInteractionType.
 *
 * @version200New
 */
VuoInteractionType VuoInteractionType_makeFromJson(json_object *js)
{
	const char *valueAsString = "none";

	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoInteractionType value = VuoInteractionType_None;

	if(strcmp(valueAsString, "none") == 0)
		value = VuoInteractionType_None;
	else if(strcmp(valueAsString, "move") == 0)
		value = VuoInteractionType_Move;
	else if(strcmp(valueAsString, "press") == 0)
		value = VuoInteractionType_Press;
	else if(strcmp(valueAsString, "release") == 0)
		value = VuoInteractionType_Release;
	else if(strcmp(valueAsString, "click") == 0)
		value = VuoInteractionType_Click;
	else if(strcmp(valueAsString, "dragStart") == 0)
		value = VuoInteractionType_DragStart;
	else if(strcmp(valueAsString, "drag") == 0)
		value = VuoInteractionType_Drag;
	else if(strcmp(valueAsString, "dragFinish") == 0)
		value = VuoInteractionType_DragFinish;
	else if(strcmp(valueAsString, "canceled") == 0)
		value = VuoInteractionType_Canceled;

	return value;
}

/**
 * @ingroup VuoInteractionType
 * Encodes @a value as a JSON object.
 *
 * @version200New
 */
json_object * VuoInteractionType_getJson(const VuoInteractionType value)
{
	char *valueAsString = "none";

	if(value == VuoInteractionType_None)
		valueAsString = "none";
	else if(value == VuoInteractionType_Move)
		valueAsString = "move";
	else if(value == VuoInteractionType_Press)
		valueAsString = "press";
	else if(value == VuoInteractionType_Release)
		valueAsString = "release";
	else if(value == VuoInteractionType_Click)
		valueAsString = "click";
	else if(value == VuoInteractionType_DragStart)
		valueAsString = "dragStart";
	else if(value == VuoInteractionType_Drag)
		valueAsString = "drag";
	else if(value == VuoInteractionType_DragFinish)
		valueAsString = "dragFinish";
	else if(value == VuoInteractionType_Canceled)
		valueAsString = "canceled";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 *
 * @version200New
 */
VuoList_VuoInteractionType VuoInteractionType_getAllowedValues(void)
{
	VuoList_VuoInteractionType l = VuoListCreate_VuoInteractionType();

	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_None);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Move);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Press);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Release);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Click);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_DragStart);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Drag);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_DragFinish);
	VuoListAppendValue_VuoInteractionType(l, VuoInteractionType_Canceled);

	return l;
}

/**
 * @ingroup VuoInteractionType
 * Returns a brief description of @a value.
 *
 * @version200New
 */
char * VuoInteractionType_getSummary(const VuoInteractionType value)
{
	char *valueAsString = "";

	if(value == VuoInteractionType_None)
		valueAsString = "None";
	else if(value == VuoInteractionType_Move)
		valueAsString = "Move";
	else if(value == VuoInteractionType_Press)
		valueAsString = "Press";
	else if(value == VuoInteractionType_Release)
		valueAsString = "Release";
	else if(value == VuoInteractionType_Click)
		valueAsString = "Click";
	else if(value == VuoInteractionType_DragStart)
		valueAsString = "DragStart";
	else if(value == VuoInteractionType_Drag)
		valueAsString = "Drag";
	else if(value == VuoInteractionType_DragFinish)
		valueAsString = "DragFinish";
	else if(value == VuoInteractionType_Canceled)
		valueAsString = "Canceled";

	return strdup(valueAsString);
}

/**
 * True if types are the same, false otherwise.
 *
 * @version200New
 */
bool VuoInteractionType_areEqual(const VuoInteractionType a, const VuoInteractionType b)
{
	return a == b;
}
