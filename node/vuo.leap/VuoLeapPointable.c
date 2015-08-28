/**
 * @file
 * VuoLeapPointable implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "VuoLeapPointable.h"
#include "VuoLeapPointableType.h"
#include "VuoLeapTouchZone.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
 VuoModuleMetadata({
					"title" : "Leap Pointable",
					"description" : "Struct of LeapPointable data.",
					"keywords" : ["gesture", "controller", "motion", "hand", "finger", "pointable"],
					"version" : "1.0.0",
					"dependencies" : [
						]
					});
#endif
/// @}

/**
 * @ingroup VuoLeapPointable
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : 42,
 *	   "type" : "finger",
 *     "length" : 4.2,
 *     "width" : 4.2,
 *     "direction" : {"x":1,"y":1,"z":1},
 *     "tipPosition" : {"x":1,"y":1,"z":1},
 *     "tipVelocity" : {"x":1,"y":1,"z":1},
 *     "stabilizedTipPosition" : {"x":1,"y":1,"z":1},
 *	   "timeVisible" : 2.3
 *	   "touchDistance" : 0.3
 *	   "touchZone" : "hovering"
 *   }
 * }
 */
VuoLeapPointable VuoLeapPointable_valueFromJson(json_object * js)
{
	VuoLeapPointable pointable =
	{
		-1,
		VuoLeapPointableType_Finger,
		0,
		0,
		VuoPoint3d_make(0,0,0),
		VuoPoint3d_make(0,0,0),
		VuoPoint3d_make(0,0,0),
		VuoPoint3d_make(0,0,0),
		0,
		0,
		VuoLeapTouchZone_None
	};

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		pointable.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "type", &o))
		pointable.type = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "length", &o))
		pointable.length = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "width", &o))
		pointable.width = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "direction", &o))
		pointable.direction = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "tipPosition", &o))
		pointable.tipPosition = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "tipVelocity", &o))
		pointable.tipVelocity = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "stabilizedTipPosition", &o))
		pointable.stabilizedTipPosition = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "timeVisible", &o))
		pointable.timeVisible = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "touchDistance", &o))
		pointable.touchDistance = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "touchZone", &o))
		pointable.touchZone = VuoReal_valueFromJson(o);

	return pointable;
}

/**
 * @ingroup VuoLeapPointable
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapPointable_jsonFromValue(const VuoLeapPointable value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_jsonFromValue(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *typeObject = VuoInteger_jsonFromValue(value.type);
	json_object_object_add(js, "type", typeObject);

	json_object *lengthObject = VuoReal_jsonFromValue(value.length);
	json_object_object_add(js, "length", lengthObject);

	json_object *widthObject = VuoReal_jsonFromValue(value.width);
	json_object_object_add(js, "width", widthObject);

	json_object *directionObject = VuoPoint3d_jsonFromValue(value.direction);
	json_object_object_add(js, "direction", directionObject);

	json_object *tipPositionObject = VuoPoint3d_jsonFromValue(value.tipPosition);
	json_object_object_add(js, "tipPosition", tipPositionObject);

	json_object *tipVelocityObject = VuoPoint3d_jsonFromValue(value.tipVelocity);
	json_object_object_add(js, "tipVelocity", tipVelocityObject);

	json_object *stabilizedTipPositionObject = VuoPoint3d_jsonFromValue(value.stabilizedTipPosition);
	json_object_object_add(js, "stabilizedTipPosition", stabilizedTipPositionObject);

	json_object *timeVisibleObject = VuoReal_jsonFromValue(value.timeVisible);
	json_object_object_add(js, "timeVisible", timeVisibleObject);

	json_object *touchDistanceObject = VuoReal_jsonFromValue(value.touchDistance);
	json_object_object_add(js, "touchDistance", touchDistanceObject);

	json_object *touchZoneObject = VuoInteger_jsonFromValue(value.touchZone);
	json_object_object_add(js, "touchZone", touchZoneObject);

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapPointable_summaryFromValue(const VuoLeapPointable value)
{
	return VuoText_format("%ld", value.id);
}

/**
 * @ingroup VuoLeapPointable
 * Creates a new pointable from the specified values.
 */
VuoLeapPointable VuoLeapPointable_make(
	VuoInteger id,
	VuoLeapPointableType type,
	VuoReal length,
	VuoReal width,
	VuoPoint3d direction,
	VuoPoint3d tipPosition,
	VuoPoint3d tipVelocity,
	VuoPoint3d stabilizedTipPosition,
	VuoReal timeVisible,
	VuoReal touchDistance,
	VuoLeapTouchZone touchZone)
{
	VuoLeapPointable pointable;

	pointable.id = id;
	pointable.type = type;
	pointable.length = length;
	pointable.width = width;
	pointable.direction = direction;
	pointable.tipPosition = tipPosition;
	pointable.tipVelocity = tipVelocity;
	pointable.stabilizedTipPosition = stabilizedTipPosition;
	pointable.timeVisible = timeVisible;
	pointable.touchDistance = touchDistance;
	pointable.touchZone = touchZone;

	return pointable;
}
