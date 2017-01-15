/**
 * @file
 * VuoWindowProperty implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoWindowProperty.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Window Property",
					  "description" : "A window setting, such as its title, or whether it is full-screen.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoBoolean",
						"VuoCoordinateUnit",
						"VuoCursor",
						"VuoInteger",
						"VuoReal",
						"VuoScreen",
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
 *     "title" : "My Composition"
 *   }
 * }
 */
VuoWindowProperty VuoWindowProperty_makeFromJson(json_object * js)
{
	VuoWindowProperty value = {-1};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "unit", &o))
		value.unit = VuoCoordinateUnit_makeFromJson(o);

	if (json_object_object_get_ex(js, "title", &o))
	{
		value.type = VuoWindowProperty_Title;
		value.title = VuoText_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "fullScreen", &o))
	{
		value.type = VuoWindowProperty_FullScreen;
		value.fullScreen = VuoBoolean_makeFromJson(o);
		if (json_object_object_get_ex(js, "screen", &o))
			value.screen = VuoScreen_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "left", &o))
	{
		value.type = VuoWindowProperty_Position;
		value.left = VuoInteger_makeFromJson(o);
		if (json_object_object_get_ex(js, "top", &o))
			value.top = VuoInteger_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "width", &o))
	{
		value.type = VuoWindowProperty_Size;
		value.width = VuoInteger_makeFromJson(o);
		if (json_object_object_get_ex(js, "height", &o))
			value.height = VuoInteger_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "aspectRatio", &o))
	{
		value.type = VuoWindowProperty_AspectRatio;
		value.aspectRatio = VuoReal_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "aspectRatioReset", &o))
	{
		value.type = VuoWindowProperty_AspectRatioReset;
		return value;
	}
	else if (json_object_object_get_ex(js, "resizable", &o))
	{
		value.type = VuoWindowProperty_Resizable;
		value.resizable = VuoBoolean_makeFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "cursor", &o))
	{
		value.type = VuoWindowProperty_Cursor;
		value.cursor = VuoCursor_makeFromJson(o);
		return value;
	}

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoWindowProperty_getJson(const VuoWindowProperty value)
{
	json_object *js = json_object_new_object();

	if (value.type == VuoWindowProperty_Title)
		json_object_object_add(js, "title", VuoText_getJson(value.title));
	else if (value.type == VuoWindowProperty_FullScreen)
	{
		json_object_object_add(js, "fullScreen", VuoBoolean_getJson(value.fullScreen));
		json_object_object_add(js, "screen", VuoScreen_getJson(value.screen));
	}
	else if (value.type == VuoWindowProperty_Position)
	{
		json_object_object_add(js, "left", VuoInteger_getJson(value.left));
		json_object_object_add(js, "top",  VuoInteger_getJson(value.top));
		json_object_object_add(js, "unit", VuoCoordinateUnit_getJson(value.unit));
	}
	else if (value.type == VuoWindowProperty_Size)
	{
		json_object_object_add(js, "width",  VuoInteger_getJson(value.width));
		json_object_object_add(js, "height", VuoInteger_getJson(value.height));
		json_object_object_add(js, "unit", VuoCoordinateUnit_getJson(value.unit));
	}
	else if (value.type == VuoWindowProperty_AspectRatio)
		json_object_object_add(js, "aspectRatio", VuoReal_getJson(value.aspectRatio));
	else if (value.type == VuoWindowProperty_AspectRatioReset)
		json_object_object_add(js, "aspectRatioReset", VuoBoolean_getJson(true));
	else if (value.type == VuoWindowProperty_Resizable)
		json_object_object_add(js, "resizable", VuoBoolean_getJson(value.resizable));
	else if (value.type == VuoWindowProperty_Cursor)
		json_object_object_add(js, "cursor", VuoCursor_getJson(value.cursor));

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoWindowProperty_getSummary(const VuoWindowProperty value)
{
	if (value.type == VuoWindowProperty_Title)
		return VuoText_format("Change Window Title: \"%s\"", value.title);
	else if (value.type == VuoWindowProperty_FullScreen)
	{
		if (value.fullScreen)
		{
			char *screenSummary = VuoScreen_getSummary(value.screen);
			char *summary = VuoText_format("Change to Fullscreen<br>%s", screenSummary);
			free(screenSummary);
			return summary;
		}
		else
			return strdup("Change to Windowed");
	}
	else if (value.type == VuoWindowProperty_Position)
	{
		char *unit = VuoCoordinateUnit_getSummary(value.unit);
		char *t = VuoText_format("Change Window Position: (%lld, %lld) %s", value.left, value.top, unit);
		free(unit);
		return t;
	}
	else if (value.type == VuoWindowProperty_Size)
	{
		char *unit = VuoCoordinateUnit_getSummary(value.unit);
		char *t = VuoText_format("Change Window Size: %lldx%lld %s", value.width, value.height, unit);
		free(unit);
		return t;
	}
	else if (value.type == VuoWindowProperty_AspectRatio)
		return VuoText_format("Change Window Aspect Ratio: %g", value.aspectRatio);
	else if (value.type == VuoWindowProperty_AspectRatioReset)
		return VuoText_format("Reset Window Aspect Ratio");
	else if (value.type == VuoWindowProperty_Resizable)
		return value.resizable ? strdup("Enable Window Resizing") : strdup("Disable Window Resizing");
	else if (value.type == VuoWindowProperty_Cursor)
	{
		char *cursorSummary = VuoCursor_getSummary(value.cursor);
		char *summary = VuoText_format("Change mouse cursor to %s", cursorSummary);
		free(cursorSummary);
		return summary;
	}

	return strdup("(unknown window property)");
}
