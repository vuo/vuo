/**
 * @file
 * VuoLeapPointable implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoLeapPointable.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Leap Pointable",
					  "description" : "Struct of LeapPointable data.",
					  "keywords" : ["gesture", "controller", "motion", "hand", "finger", "pointable"],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoInteger",
						"VuoLeapPointableType",
						"VuoLeapTouchZone",
						"VuoPoint3d",
						"VuoReal",
						"VuoText"
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
 *	   "timeVisible" : 2.3
 *	   "touchDistance" : 0.3
 *	   "touchZone" : "hovering",
 *		"isExtended" : true
 *   }
 * }
 */
VuoLeapPointable VuoLeapPointable_makeFromJson(json_object * js)
{
	return (VuoLeapPointable){
		VuoJson_getObjectValue(VuoInteger,       js, "id",            -1),
		VuoJson_getObjectValue(VuoInteger,       js, "type",          VuoLeapPointableType_Finger),
		VuoJson_getObjectValue(VuoReal,          js, "length",        0),
		VuoJson_getObjectValue(VuoReal,          js, "width",         0),
		VuoJson_getObjectValue(VuoPoint3d,       js, "direction",     (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoPoint3d,       js, "tipPosition",   (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoPoint3d,       js, "tipVelocity",   (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoReal,          js, "timeVisible",   0),
		VuoJson_getObjectValue(VuoReal,          js, "touchDistance", 0),
		VuoJson_getObjectValue(VuoLeapTouchZone, js, "touchZone",     VuoLeapTouchZone_None),
		VuoJson_getObjectValue(VuoBoolean,       js, "isExtended",    false)
	};
}

/**
 * @ingroup VuoLeapPointable
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapPointable_getJson(const VuoLeapPointable value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *typeObject = VuoInteger_getJson(value.type);
	json_object_object_add(js, "type", typeObject);

	json_object *lengthObject = VuoReal_getJson(value.length);
	json_object_object_add(js, "length", lengthObject);

	json_object *widthObject = VuoReal_getJson(value.width);
	json_object_object_add(js, "width", widthObject);

	json_object *directionObject = VuoPoint3d_getJson(value.direction);
	json_object_object_add(js, "direction", directionObject);

	json_object *tipPositionObject = VuoPoint3d_getJson(value.tipPosition);
	json_object_object_add(js, "tipPosition", tipPositionObject);

	json_object *tipVelocityObject = VuoPoint3d_getJson(value.tipVelocity);
	json_object_object_add(js, "tipVelocity", tipVelocityObject);

	json_object *timeVisibleObject = VuoReal_getJson(value.timeVisible);
	json_object_object_add(js, "timeVisible", timeVisibleObject);

	json_object *touchDistanceObject = VuoReal_getJson(value.touchDistance);
	json_object_object_add(js, "touchDistance", touchDistanceObject);

	json_object *touchZoneObject = VuoInteger_getJson(value.touchZone);
	json_object_object_add(js, "touchZone", touchZoneObject);

	json_object *isExtendedObject = VuoBoolean_getJson(value.isExtended);
	json_object_object_add(js, "isExtended", isExtendedObject);

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapPointable_getSummary(const VuoLeapPointable value)
{
	return VuoText_format("%lld", value.id);
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
	VuoReal timeVisible,
	VuoReal touchDistance,
	VuoLeapTouchZone touchZone,
	VuoBoolean isExtended)
{
	VuoLeapPointable pointable;

	pointable.id = id;
	pointable.type = type;
	pointable.length = length;
	pointable.width = width;
	pointable.direction = direction;
	pointable.tipPosition = tipPosition;
	pointable.tipVelocity = tipVelocity;
	pointable.timeVisible = timeVisible;
	pointable.touchDistance = touchDistance;
	pointable.touchZone = touchZone;
	pointable.isExtended = isExtended;

	return pointable;
}
