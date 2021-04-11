/**
 * @file
 * VuoInteraction implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
// @todo
// #include "VuoMouseUtility.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "UI Interaction",
					 "description" : "Stores information about a UI device's input.",
					 "keywords" : [ "gui" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBoolean",
						"VuoList_VuoInteraction",
						"VuoReal",
						"VuoText",
						"VuoUuid",
						"VuoPoint2d",
						"VuoInteractionType"
						// "VuoMouseUtility"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoInteraction
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *		"@todo"
 *   }
 * }
 *
 * @version200New
 */
VuoInteraction VuoInteraction_makeFromJson(json_object * js)
{
	VuoInteraction interaction = VuoInteraction_make();

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "uuid", &o))
		interaction.uuid = VuoUuid_makeFromJson(o);

	if (json_object_object_get_ex(js, "position", &o))
		interaction.position = VuoPoint2d_makeFromJson(o);

	if (json_object_object_get_ex(js, "isPressed", &o))
		interaction.isPressed = json_object_get_boolean(o);

	if (json_object_object_get_ex(js, "type", &o))
		interaction.type = VuoInteractionType_makeFromJson(o);

	if( json_object_object_get_ex(js, "origin", &o))
		interaction.origin = VuoPoint2d_makeFromJson(o);

	if( json_object_object_get_ex(js, "timestamp", &o))
		interaction.timestamp = json_object_get_double(o);

	if( json_object_object_get_ex(js, "clickCount", &o))
		interaction.clickCount = VuoInteger_makeFromJson(o);

	return interaction;
}

/**
 * @ingroup VuoInteraction
 * Encodes @c value as a JSON object.
 *
 * @version200New
 */
json_object * VuoInteraction_getJson(const VuoInteraction value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "uuid", VuoUuid_getJson(value.uuid));
	json_object_object_add(js, "position", VuoPoint2d_getJson(value.position));
	json_object_object_add(js, "isPressed", json_object_new_boolean(value.isPressed));
	json_object_object_add(js, "type", VuoInteractionType_getJson(value.type));
	json_object_object_add(js, "origin", VuoPoint2d_getJson(value.origin));
	json_object_object_add(js, "timestamp", json_object_new_double(value.timestamp));
	json_object_object_add(js, "clickCount", VuoInteger_getJson(value.clickCount));

	return js;
}

/**
 * @ingroup VuoInteraction
 * Returns a compact string representation of @c value (comma-separated components).
 *
 * @version200New
 */
char * VuoInteraction_getSummary(const VuoInteraction value)
{
	char* typeStr = VuoInteractionType_getSummary(value.type);

	char* sum = VuoText_format("<div>Type: %s</div><div>Position: %.2f, %.2f</div><div>Is Pressed: %s</div><div>Click Count: %lli</div>",
		typeStr,
		value.position.x,
		value.position.y,
		(value.isPressed ? "Yes" : "No"),
		value.clickCount);

	free(typeStr);
	return sum;
}

/**
 * Returns true if both interactions are identical (value-wise).  Interaction uuids may differ.
 *
 * @version200New
 */
bool VuoInteraction_areEqual(const VuoInteraction value1, const VuoInteraction value2)
{
	return	VuoPoint2d_areEqual(value1.position, value2.position) &&
			VuoBoolean_areEqual(value1.isPressed, value2.isPressed) &&
			VuoInteractionType_areEqual(value1.type, value2.type) &&
			VuoPoint2d_areEqual(value1.origin, value2.origin) &&
			VuoReal_areEqual(value1.timestamp, value2.timestamp);
}

/**
 * Returns true if a < b.
 *
 * @version200New
 */
bool VuoInteraction_isLessThan(const VuoInteraction a, const VuoInteraction b)
{
	// @todo
	return (int)&a < (int)&b;
}

#define MAX_CLICK_DELTA .2 ///< The amount of time a press can be held before it's no longer considered a click.  @todo Expose this as a parameter.

#define MIN_DRAG_DISTANCE .08 ///< The minimum distance a position can wander while pressed before being considered a drag.  @todo Expose this as a parameter

/**
 *	Test if a current interaction is a drag based on previous interaction.
 *
 * @version200New
 */
static bool VuoInteraction_isDrag(const VuoInteraction previous, const bool isPressed, const VuoPoint2d position)
{
	// if this frame isn't pressed and neither was the previous one this definitely isn't a drag
	if(!isPressed || !previous.isPressed)
		return false;

	// If the last frame was a drag and the device is still engaged, it's still a drag
	if(previous.type == VuoInteractionType_Drag || previous.type == VuoInteractionType_DragStart)
		return true;

	// if the device has been pressed for a long-ish time and moved slightly it could be a fine-grained drag
	if( VuoLogGetTime() - previous.timestamp > MAX_CLICK_DELTA && fabs(VuoPoint2d_distance(previous.origin, position)) > .001 )
		return true;
	else
		return fabs(VuoPoint2d_distance(position, previous.origin)) > MIN_DRAG_DISTANCE;
}

/**
 *	@todo - use VuoMouseUtility
 *
 * @version200New
 */
VuoReal VuoMouseUtility_getDoubleClickInterval()
{
	return .5;
}

/**
 * Update an interaction with new input position and isPressed values.  Returns true if the interaction has changed since the last update.
 *
 * @version200New
 */
bool VuoInteraction_update(const VuoPoint2d position, const VuoBoolean isPressed, VuoInteraction* interaction)
{
	bool changed = false;
	VuoInteraction prev = *interaction;
	interaction->type = VuoInteractionType_None;

	bool isDrag = VuoInteraction_isDrag(prev, isPressed, position);

	// type is a little hierarchy; sometimes multiple events can occur in a single interaction (move & release for example)
	// release/click/press/drag take priority over move in these cases.
	if(isDrag || !VuoPoint2d_areEqual(position, interaction->position))
	{
		changed = true;

		if( isDrag )
			interaction->type = (prev.type == VuoInteractionType_Drag || prev.type == VuoInteractionType_DragStart) ? VuoInteractionType_Drag : VuoInteractionType_DragStart;
		else
			interaction->type = VuoInteractionType_Move;

		interaction->position = position;
		interaction->clickCount = 0;
	}

	if( interaction->isPressed != isPressed )
	{
		changed = true;

		if(isPressed)
		{
			interaction->type = VuoInteractionType_Press;
			interaction->origin = position;
			interaction->timestamp = VuoLogGetTime();

			if(prev.clickCount < 1 || ((interaction->timestamp - prev.timestamp) < VuoMouseUtility_getDoubleClickInterval()))
			{
				interaction->clickCount = prev.clickCount + 1;
			}
			else
			{
				interaction->clickCount = 1;
			}
		}
		else
		{
			// is drag?
			// is click?
			// is release.
			if( prev.type == VuoInteractionType_DragStart || prev.type == VuoInteractionType_Drag )
			{
				interaction->clickCount = 0;
				interaction->type = VuoInteractionType_DragFinish;
			}
			else if( VuoLogGetTime() - interaction->timestamp < MAX_CLICK_DELTA )
			{
				interaction->clickCount = prev.clickCount;
				interaction->type = VuoInteractionType_Click;
			}
			else
			{
				interaction->clickCount = prev.clickCount;
				interaction->type = VuoInteractionType_Release;
			}
		}

		interaction->isPressed = isPressed;
	}

	return changed;
}

