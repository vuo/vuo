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
 *     "tipVelocity" : {"x":1,"y":1,"z":1}
 *   }
 * }
 */
VuoLeapPointable VuoLeapPointable_valueFromJson(json_object * js)
{
	VuoLeapPointable pointable = {-1,VuoLeapPointableType_Finger,0,0,VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,0),VuoPoint3d_make(0,0,0)};
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

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapPointable_summaryFromValue(const VuoLeapPointable value)
{
	const char *format = "%d";
	int size = snprintf(NULL,0,format,value.id);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value.id);
	return valueAsString;
}

/**
 * @ingroup VuoLeapPointable
 * Creates a new pointable from the specified values.
 */
VuoLeapPointable VuoLeapPointable_make(VuoInteger id, VuoLeapPointableType type, VuoReal length, VuoReal width, VuoPoint3d direction, VuoPoint3d tipPosition, VuoPoint3d tipVelocity)
{
	VuoLeapPointable pointable;

	pointable.id = id;
	pointable.type = type;
	pointable.length = length;
	pointable.width = width;
	pointable.direction = direction;
	pointable.tipPosition = tipPosition;
	pointable.tipVelocity = tipVelocity;

	return pointable;
}
