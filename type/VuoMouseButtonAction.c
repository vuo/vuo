/**
 * @file
 * VuoMouseButtonAction implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "VuoMouseButtonAction.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Mouse Button Action",
					 "description" : "A press, release, or click of one of the mouse buttons.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMouseButtonAction
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "button" : "left",
 *     "type" : "press",
 *     "position" : { "x" : 100, "y" : 100 }
 *   }
 * }
 */
VuoMouseButtonAction VuoMouseButtonAction_valueFromJson(json_object * js)
{
	VuoMouseButtonAction e = VuoMouseButtonAction_make( VuoMouseButton_Left, VuoMouseButtonActionType_Press, VuoPoint2d_make(0, 0) );
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "button", &o))
	{
		const char *buttonName = json_object_get_string(o);

		if (! strcmp(buttonName, "left"))
			e.button = VuoMouseButton_Left;
		else if (! strcmp(buttonName, "middle"))
			e.button = VuoMouseButton_Middle;
		else if (! strcmp(buttonName, "right"))
			e.button = VuoMouseButton_Right;
	}

	if (json_object_object_get_ex(js, "type", &o))
	{
		const char *typeName = json_object_get_string(o);

		if (! strcmp(typeName, "press"))
			e.type = VuoMouseButtonActionType_Press;
		else if (! strcmp(typeName, "release"))
			e.type = VuoMouseButtonActionType_Release;
		else if (! strcmp(typeName, "single-click"))
			e.type = VuoMouseButtonActionType_SingleClick;
		else if (! strcmp(typeName, "double-click"))
			e.type = VuoMouseButtonActionType_DoubleClick;
		else if (! strcmp(typeName, "triple-click"))
			e.type = VuoMouseButtonActionType_TripleClick;
	}

	if (json_object_object_get_ex(js, "position", &o))
		e.position = VuoPoint2d_valueFromJson(o);

	return e;
}

/**
 * @ingroup VuoMouseButtonAction
 * Returns a string representation of the mouse button.
 */
static const char * VuoMouseButtonAction_nameForButton(VuoMouseButton button)
{
	const char *buttonName = "";
	switch (button)
	{
		case VuoMouseButton_Left:
			buttonName = "left";
			break;
		case VuoMouseButton_Middle:
			buttonName = "middle";
			break;
		case VuoMouseButton_Right:
			buttonName = "right";
			break;
	}
	return buttonName;
}

/**
 * @ingroup VuoMouseButtonAction
 * Returns a string representation of the action type.
 */
static const char * VuoMouseButtonAction_nameForActionType(VuoMouseButtonActionType type)
{
	const char *typeName = "";
	switch (type)
	{
		case VuoMouseButtonActionType_Press:
			typeName = "press";
			break;
		case VuoMouseButtonActionType_Release:
			typeName = "release";
			break;
		case VuoMouseButtonActionType_SingleClick:
			typeName = "single-click";
			break;
		case VuoMouseButtonActionType_DoubleClick:
			typeName = "double-click";
			break;
		case VuoMouseButtonActionType_TripleClick:
			typeName = "triple-click";
			break;
	}
	return typeName;
}

/**
 * @ingroup VuoMouseButtonAction
 * Encodes @c value as a JSON object.
 */
json_object * VuoMouseButtonAction_jsonFromValue(const VuoMouseButtonAction e)
{
	json_object *js = json_object_new_object();

	const char *buttonName = VuoMouseButtonAction_nameForButton(e.button);
	json_object *buttonObject = json_object_new_string(buttonName);
	json_object_object_add(js, "button", buttonObject);

	const char *typeName = VuoMouseButtonAction_nameForActionType(e.type);
	json_object *typeObject = json_object_new_string(typeName);
	json_object_object_add(js, "type", typeObject);

	json_object *positionObject = VuoPoint2d_jsonFromValue(e.position);
	json_object_object_add(js, "position", positionObject);

	return js;
}

/**
 * @ingroup VuoMouseButtonAction
 * Returns a compact string representation of @c value.
 */
char * VuoMouseButtonAction_summaryFromValue(const VuoMouseButtonAction e)
{
	const char *format = "%s %s at %s";

	const char *buttonName = VuoMouseButtonAction_nameForButton(e.button);
	const char *typeName = VuoMouseButtonAction_nameForActionType(e.type);
	const char *position = VuoPoint2d_summaryFromValue(e.position);

	int size = snprintf(NULL, 0, format, buttonName, typeName, position);
	char *summary = (char *)malloc(size+1);
	snprintf(summary, size+1, format, buttonName, typeName, position);

	return summary;
}
