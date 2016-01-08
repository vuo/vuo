/**
 * @file
 * VuoScreen implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoScreen.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Screen",
					  "description" : "Information about a display screen.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoInteger",
						"VuoPoint2d",
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : -1,
 *     "name" : ""
 *   }
 * }
 */
VuoScreen VuoScreen_makeFromJson(json_object *js)
{
	VuoScreen value = {-1,"",{0,0},0,0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	else
		value.name = VuoText_make("");

	if (json_object_object_get_ex(js, "topLeft", &o))
		value.topLeft = VuoPoint2d_makeFromJson(o);

	if (json_object_object_get_ex(js, "width", &o))
		value.width = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "height", &o))
		value.height = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "dpiHorizontal", &o))
		value.dpiHorizontal = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "dpiVertical", &o))
		value.dpiVertical = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoScreen_getJson(const VuoScreen value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_getJson(value.name);
	json_object_object_add(js, "name", nameObject);

	json_object *topLeftObject = VuoPoint2d_getJson(value.topLeft);
	json_object_object_add(js, "topLeft", topLeftObject);

	json_object *widthObject = VuoInteger_getJson(value.width);
	json_object_object_add(js, "width", widthObject);

	json_object *heightObject = VuoInteger_getJson(value.height);
	json_object_object_add(js, "height", heightObject);

	json_object *dpiHorizontalObject = VuoInteger_getJson(value.dpiHorizontal);
	json_object_object_add(js, "dpiHorizontal", dpiHorizontalObject);

	json_object *dpiVerticalObject = VuoInteger_getJson(value.dpiVertical);
	json_object_object_add(js, "dpiVertical", dpiVerticalObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoScreen_getSummary(const VuoScreen value)
{
	if (value.id == -1 && strlen(value.name) == 0)
		return strdup("The default screen");
	else if (value.id == -1)
		return VuoText_format("The first screen whose name contains \"%s\"", value.name);
	else if (strlen(value.name) == 0)
		return VuoText_format("Screen #%lld", value.id);
	else
		// An actual detected screen (rather than abstract criteria).
		return VuoText_format("Screen \"%s\"<br>%lld x %lld points<br>%lld x %lld DPI", value.name, value.width, value.height, value.dpiHorizontal, value.dpiVertical);
}

/**
 * Returns true if the two screen specifications are identical.
 */
bool VuoScreen_areEqual(VuoScreen value1, VuoScreen value2)
{
	return value1.id == value2.id
		&& VuoText_areEqual(value1.name, value2.name)
		&& VuoPoint2d_areEqual(value1.topLeft, value2.topLeft)
		&& value1.width == value2.width
		&& value1.height == value2.height
		&& value1.dpiHorizontal == value2.dpiVertical
		&& value1.dpiVertical == value2.dpiVertical;
}
