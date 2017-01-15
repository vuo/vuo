/**
 * @file
 * VuoScreen implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoScreen.h"
#include "VuoScreenCommon.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Screen",
					  "description" : "Information about a display screen.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoBoolean",
						"VuoInteger",
						"VuoPoint2d",
						"VuoScreenCommon",
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
 *     "type" : "match-name",
 *     "name" : "lcd"
 *   }
 * }
 */
VuoScreen VuoScreen_makeFromJson(json_object *js)
{
	VuoScreen value = {VuoScreenType_Active,-1,"",false,{0,0},0,0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "type", &o))
		value.type = VuoScreen_typeFromCString(json_object_get_string(o));

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	else
		value.name = VuoText_make("");

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoScreen_getJson(const VuoScreen value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "type", json_object_new_string(VuoScreen_cStringForType(value.type)));

	if (value.type == VuoScreenType_MatchName)
	{
		json_object *nameObject = VuoText_getJson(value.name);
		json_object_object_add(js, "name", nameObject);
	}
	else if (value.type == VuoScreenType_MatchId)
	{
		json_object *idObject = VuoInteger_getJson(value.id);
		json_object_object_add(js, "id", idObject);
	}

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoScreen_getSummary(const VuoScreen value)
{
	if (value.isRealized)
		return VuoText_format("Screen \"%s\"<br>%lld x %lld points<br>%lld x %lld DPI", value.name, value.width, value.height, value.dpiHorizontal, value.dpiVertical);

	if (value.type == VuoScreenType_Active)
		return strdup("The screen with the active window.");
	if (value.type == VuoScreenType_Primary)
		return strdup("The primary screen (with the menu bar).");
	else if (value.type == VuoScreenType_Secondary)
		return strdup("A secondary screen.");
	else if (value.type == VuoScreenType_MatchName)
		return VuoText_format("The first screen whose name contains \"%s\"", value.name);
	else if (value.type == VuoScreenType_MatchId)
		return VuoText_format("Screen #%lld", value.id);
	else
		return strdup("(unknown)");
}

/**
 * Returns true if the two screen specifications are identical.
 */
bool VuoScreen_areEqual(VuoScreen value1, VuoScreen value2)
{
	return value1.id == value2.id
		&& VuoText_areEqual(value1.name, value2.name)
		&& value1.isRealized == value2.isRealized
		&& VuoPoint2d_areEqual(value1.topLeft, value2.topLeft)
		&& value1.width == value2.width
		&& value1.height == value2.height
		&& value1.dpiHorizontal == value2.dpiVertical
		&& value1.dpiVertical == value2.dpiVertical;
}

/**
 * Given any VuoScreen structure:
 *
 *    - If `screen` is already realized, copies it into `realizedScreen, and returns true.
 *    - If a matching screen is found, sets `realizedScreen` to match it by ID, fills in all the details, and returns true.
 *    - If no matching screen is found, returns false, leaving `realizedDevice` unset.
 */
bool VuoScreen_realize(VuoScreen screen, VuoScreen *realizedScreen)
{
	// Already realized nothing to do.
	if (screen.isRealized)
	{
		*realizedScreen = screen;
		realizedScreen->name = VuoText_make(screen.name);
		return true;
	}

	// Otherwise, try to find a matching screen.

	if (screen.type == VuoScreenType_Active)
	{
		*realizedScreen = VuoScreen_getActive();
		return true;
	}
	else if (screen.type == VuoScreenType_Primary)
	{
		*realizedScreen = VuoScreen_getPrimary();
		return true;
	}
	else if (screen.type == VuoScreenType_Secondary)
	{
		*realizedScreen = VuoScreen_getSecondary();
		return true;
	}
	else if (screen.type == VuoScreenType_MatchName
		  || screen.type == VuoScreenType_MatchId)
	{
		VuoList_VuoScreen screens = VuoScreen_getList();
		VuoRetain(screens);
		unsigned long screenCount = VuoListGetCount_VuoScreen(screens);

		bool found = false;
		for (unsigned long i = 1; i <= screenCount; ++i)
		{
			VuoScreen s = VuoListGetValue_VuoScreen(screens, i);
			if ( (screen.type == VuoScreenType_MatchName && strstr(s.name, screen.name))
			  || (screen.type == VuoScreenType_MatchId && s.id == screen.id) )
			{
				*realizedScreen = s;
				realizedScreen->type = VuoScreenType_MatchId;
				realizedScreen->name = VuoText_make(s.name);	// Copy, since we're releasing `screens` below.
				found = true;
				break;
			}
		}

		VuoRelease(screens);
		return found;
	}

	return false;
}
