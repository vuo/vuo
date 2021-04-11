/**
 * @file
 * VuoLeapHand implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoLeapHand.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Leap Hand",
					 "description" : "Data for a Leap Hand",
					 "keywords" : [ "gesture", "controller", "hand", "finger", "motion" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoInteger",
						 "VuoPoint3d",
						 "VuoPoint4d",
						 "VuoReal",
						 "VuoText",
						 "VuoLeapPointable",
						 "VuoList_VuoLeapPointable"
					 ]
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
VuoLeapHand VuoLeapHand_makeFromJson(json_object * js)
{
	return (VuoLeapHand){
		VuoJson_getObjectValue(VuoInteger,               js, "id",            0),
		VuoJson_getObjectValue(VuoPoint4d,               js, "rotation",      (VuoPoint4d){0,0,0,0}),
		VuoJson_getObjectValue(VuoPoint3d,               js, "palmPosition",  (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoPoint3d,               js, "palmVelocity",  (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoReal,                  js, "sphereRadius",  0.),
		VuoJson_getObjectValue(VuoPoint3d,               js, "sphereCenter",  (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoReal,                  js, "palmWidth",     0.),
		VuoJson_getObjectValue(VuoPoint3d,               js, "wristPosition", (VuoPoint3d){0,0,0}),
		VuoJson_getObjectValue(VuoReal,                  js, "pinchAmount",   0.),
		VuoJson_getObjectValue(VuoReal,                  js, "grabAmount",    0.),
		VuoJson_getObjectValue(VuoReal,                  js, "timeVisible",   0.),
		VuoJson_getObjectValue(VuoBoolean,               js, "isLeftHand",    false),
		VuoJson_getObjectValue(VuoReal,                  js, "confidence",    0.),
		VuoJson_getObjectValue(VuoList_VuoLeapPointable, js, "fingers",       NULL)
	};
}

/**
 * @ingroup VuoLeapHand
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapHand_getJson(const VuoLeapHand value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoInteger_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *rotationObject = VuoPoint4d_getJson(value.rotation);
	json_object_object_add(js, "rotation", rotationObject);

	json_object *palmPositionObject = VuoPoint3d_getJson(value.palmPosition);
	json_object_object_add(js, "palmPosition", palmPositionObject);

	json_object *palmVelocityObject = VuoPoint3d_getJson(value.palmVelocity);
	json_object_object_add(js, "palmVelocity", palmVelocityObject);

	json_object *sphereRadiusObject = VuoReal_getJson(value.sphereRadius);
	json_object_object_add(js, "sphereRadius", sphereRadiusObject);

	json_object *sphereCenterObject = VuoPoint3d_getJson(value.sphereCenter);
	json_object_object_add(js, "sphereCenter", sphereCenterObject);

	json_object *palmWidthObject = VuoReal_getJson(value.palmWidth);
	json_object_object_add(js, "palmWidth", palmWidthObject);

	json_object *wristPositionObject = VuoPoint3d_getJson(value.wristPosition);
	json_object_object_add(js, "wristPosition", wristPositionObject);

	json_object *pinchAmountObject = VuoReal_getJson(value.pinchAmount);
	json_object_object_add(js, "pinchAmount", pinchAmountObject);

	json_object *grabAmountObject = VuoReal_getJson(value.grabAmount);
	json_object_object_add(js, "grabAmount", grabAmountObject);

	json_object *timeVisibleObject = VuoReal_getJson(value.timeVisible);
	json_object_object_add(js, "timeVisible", timeVisibleObject);

	json_object *isLeftHandObject = VuoBoolean_getJson(value.isLeftHand);
	json_object_object_add(js, "isLeftHand", isLeftHandObject);

	json_object *confidenceObject = VuoReal_getJson(value.confidence);
	json_object_object_add(js, "confidence", confidenceObject);

	json_object *pointablesObject = VuoList_VuoLeapPointable_getJson(value.fingers);
	json_object_object_add(js, "fingers", pointablesObject);

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapHand_getSummary(const VuoLeapHand value)
{
	return VuoText_format("%lld", value.id);
}

/**
 * @ingroup VuoLeapPointable
 * Creates a new pointable from the specified values.
 */
VuoLeapHand VuoLeapHand_make(	VuoInteger id,
								VuoPoint4d rotation,
								VuoPoint3d palmPosition,
								VuoPoint3d palmVelocity,
								VuoReal sphereRadius,
								VuoPoint3d sphereCenter,
								VuoReal palmWidth,
								VuoPoint3d wristPosition,
								VuoReal pinchAmount,
								VuoReal grabAmount,
								VuoReal timeVisible,
								VuoBoolean isLeftHand,
								VuoReal confidence,
								VuoList_VuoLeapPointable fingers)
{
	VuoLeapHand hand;

	hand.id 					= id;
	hand.rotation               = rotation;
	hand.palmPosition 			= palmPosition;
	hand.palmVelocity 			= palmVelocity;
	hand.sphereRadius 			= sphereRadius;
	hand.sphereCenter 			= sphereCenter;
	hand.palmWidth 				= palmWidth;
	hand.wristPosition 			= wristPosition;
	hand.pinchAmount 			= pinchAmount;
	hand.grabAmount 			= grabAmount;
	hand.timeVisible 			= timeVisible;
	hand.isLeftHand 			= isLeftHand;
	hand.confidence 			= confidence;
	hand.fingers 				= fingers;

	return hand;
}
