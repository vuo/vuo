/**
 * @file
 * VuoLeapHand implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "VuoLeapHand.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapPointable.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Leap Hand",
					 "description" : "Data for a Leap Hand",
					 "keywords" : [ "gesture", "controller", "hand", "finger", "motion" ],
					 "version" : "1.0.0",
					  "dependencies" : [],
					 "node": {
						 "isInterface" : false
					 }
				 });
#endif
/// @}

/**
 * @ingroup VuoLeapHand
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *	/// @todo write VuoLeapHand json example
 *	}
 */
VuoLeapHand VuoLeapHand_valueFromJson(json_object * js)
{
	VuoLeapHand hand = VuoLeapHand_make(
					0,									// id
					(VuoPoint3d){0,0,0},				// direction
					(VuoPoint3d){0,0,0},				// palmNormal
					(VuoPoint3d){0,0,0},				// palmPosition
					(VuoPoint3d){0,0,0},				// palmVelocity
					0.,									// sphereRadius
					(VuoPoint3d){0,0,0},				// sphereCenter
					VuoListCreate_VuoLeapPointable()	// pointables
				);

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		hand.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "direction", &o))
		hand.direction = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmNormal", &o))
		hand.palmNormal = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmPosition", &o))
		hand.palmPosition = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmVelocity", &o))
		hand.palmVelocity = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "sphereRadius", &o))
		hand.sphereRadius = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "sphereCenter", &o))
		hand.sphereCenter = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "pointables", &o))
		hand.pointables = VuoList_VuoLeapPointable_valueFromJson(o);

	return hand;
}

/**
 * @ingroup VuoLeapHand
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapHand_jsonFromValue(const VuoLeapHand value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_jsonFromValue(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *directionObject = VuoPoint3d_jsonFromValue(value.direction);
	json_object_object_add(js, "direction", directionObject);

	json_object *palmNormalObject = VuoPoint3d_jsonFromValue(value.palmNormal);
	json_object_object_add(js, "palmNormal", palmNormalObject);

	json_object *palmPositionObject = VuoPoint3d_jsonFromValue(value.palmPosition);
	json_object_object_add(js, "palmPosition", palmPositionObject);

	json_object *palmVelocityObject = VuoPoint3d_jsonFromValue(value.palmVelocity);
	json_object_object_add(js, "palmVelocity", palmVelocityObject);

	json_object *sphereRadiusObject = VuoReal_jsonFromValue(value.sphereRadius);
	json_object_object_add(js, "sphereRadius", sphereRadiusObject);

	json_object *sphereCenterObject = VuoPoint3d_jsonFromValue(value.sphereCenter);
	json_object_object_add(js, "sphereCenter", sphereCenterObject);

	json_object *pointablesObject = VuoList_VuoLeapPointable_jsonFromValue(value.pointables);
	json_object_object_add(js, "pointables", pointablesObject);

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapHand_summaryFromValue(const VuoLeapHand value)
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
VuoLeapHand VuoLeapHand_make(VuoInteger id, VuoPoint3d direction, VuoPoint3d palmNormal, VuoPoint3d palmPosition, VuoPoint3d palmVelocity, VuoReal sphereRadius, VuoPoint3d sphereCenter, VuoList_VuoLeapPointable pointables)
{
	VuoLeapHand hand;

	hand.id = id;
	hand.direction = direction;
	hand.palmNormal = palmNormal;
	hand.palmPosition = palmPosition;
	hand.palmVelocity = palmVelocity;
	hand.sphereRadius = sphereRadius;
	hand.sphereCenter = sphereCenter;
	hand.pointables = pointables;

	return hand;
}
