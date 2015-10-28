/**
 * @file
 * VuoWindowProperty implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
VuoWindowProperty VuoWindowProperty_valueFromJson(json_object * js)
{
	VuoWindowProperty value = {-1};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "title", &o))
	{
		value.type = VuoWindowProperty_Title;
		value.title = VuoText_valueFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "fullScreen", &o))
	{
		value.type = VuoWindowProperty_FullScreen;
		value.fullScreen = VuoBoolean_valueFromJson(o);
		if (json_object_object_get_ex(js, "screen", &o))
			value.screen = VuoScreen_valueFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "left", &o))
	{
		value.type = VuoWindowProperty_Position;
		value.left = VuoInteger_valueFromJson(o);
		if (json_object_object_get_ex(js, "top", &o))
			value.top = VuoInteger_valueFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "width", &o))
	{
		value.type = VuoWindowProperty_Size;
		value.width = VuoInteger_valueFromJson(o);
		if (json_object_object_get_ex(js, "height", &o))
			value.height = VuoInteger_valueFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "aspectRatio", &o))
	{
		value.type = VuoWindowProperty_AspectRatio;
		value.aspectRatio = VuoReal_valueFromJson(o);
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
		value.resizable = VuoBoolean_valueFromJson(o);
		return value;
	}
	else if (json_object_object_get_ex(js, "cursor", &o))
	{
		value.type = VuoWindowProperty_Cursor;
		value.cursor = VuoCursor_valueFromJson(o);
		return value;
	}

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoWindowProperty_jsonFromValue(const VuoWindowProperty value)
{
	json_object *js = json_object_new_object();

	if (value.type == VuoWindowProperty_Title)
		json_object_object_add(js, "title", VuoText_jsonFromValue(value.title));
	else if (value.type == VuoWindowProperty_FullScreen)
	{
		json_object_object_add(js, "fullScreen", VuoBoolean_jsonFromValue(value.fullScreen));
		json_object_object_add(js, "screen", VuoScreen_jsonFromValue(value.screen));
	}
	else if (value.type == VuoWindowProperty_Position)
	{
		json_object_object_add(js, "left", VuoInteger_jsonFromValue(value.left));
		json_object_object_add(js, "top",  VuoInteger_jsonFromValue(value.top));
	}
	else if (value.type == VuoWindowProperty_Size)
	{
		json_object_object_add(js, "width",  VuoInteger_jsonFromValue(value.width));
		json_object_object_add(js, "height", VuoInteger_jsonFromValue(value.height));
	}
	else if (value.type == VuoWindowProperty_AspectRatio)
		json_object_object_add(js, "aspectRatio", VuoReal_jsonFromValue(value.aspectRatio));
	else if (value.type == VuoWindowProperty_AspectRatioReset)
		json_object_object_add(js, "aspectRatioReset", VuoBoolean_jsonFromValue(true));
	else if (value.type == VuoWindowProperty_Resizable)
		json_object_object_add(js, "resizable", VuoBoolean_jsonFromValue(value.resizable));
	else if (value.type == VuoWindowProperty_Cursor)
		json_object_object_add(js, "cursor", VuoCursor_jsonFromValue(value.cursor));

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoWindowProperty_summaryFromValue(const VuoWindowProperty value)
{
	if (value.type == VuoWindowProperty_Title)
		return VuoText_format("Change Window Title: \"%s\"", value.title);
	else if (value.type == VuoWindowProperty_FullScreen)
	{
		if (value.fullScreen)
		{
			char *screenSummary = VuoScreen_summaryFromValue(value.screen);
			char *summary = VuoText_format("Change to Fullscreen<br>%s", screenSummary);
			free(screenSummary);
			return summary;
		}
		else
			return strdup("Change to Windowed");
	}
	else if (value.type == VuoWindowProperty_Position)
		return VuoText_format("Change Window Position: (%lld, %lld)", value.left, value.top);
	else if (value.type == VuoWindowProperty_Size)
		return VuoText_format("Change Window Size: (%lld, %lld)", value.width, value.height);
	else if (value.type == VuoWindowProperty_AspectRatio)
		return VuoText_format("Change Window Aspect Ratio: %g", value.aspectRatio);
	else if (value.type == VuoWindowProperty_AspectRatioReset)
		return VuoText_format("Reset Window Aspect Ratio");
	else if (value.type == VuoWindowProperty_Resizable)
		return value.resizable ? strdup("Enable Window Resizing") : strdup("Disable Window Resizing");
	else if (value.type == VuoWindowProperty_Cursor)
	{
		char *cursorSummary = VuoCursor_summaryFromValue(value.cursor);
		char *summary = VuoText_format("Change mouse cursor to %s", cursorSummary);
		free(cursorSummary);
		return summary;
	}

	return strdup("(unknown window property)");
}
