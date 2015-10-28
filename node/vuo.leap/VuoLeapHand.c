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
#include "VuoText.h"

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
VuoLeapHand VuoLeapHand_valueFromJson(json_object * js)
{
	VuoLeapHand hand = VuoLeapHand_make(
					0,									// id
					(VuoPoint4d){0,0,0,0},				// rotation
					(VuoPoint3d){0,0,0},				// palmPosition
					(VuoPoint3d){0,0,0},				// palmVelocity
					0.,									// sphereRadius
					(VuoPoint3d){0,0,0},				// sphereCenter
					0., 								// palmWidth
					(VuoPoint3d){0,0,0},				// wristPosition
					0.,		 							// pinchAmount
					0.,		 							// grabAmount
					0.,		 							// timeVisible
					false, 		 						// isLeftHand
					0.,		 							// confidence
					VuoListCreate_VuoLeapPointable()	// fingers
				);

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		hand.id = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "rotation", &o))
		hand.rotation = VuoPoint4d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmPosition", &o))
		hand.palmPosition = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmVelocity", &o))
		hand.palmVelocity = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "sphereRadius", &o))
		hand.sphereRadius = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "sphereCenter", &o))
		hand.sphereCenter = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "palmWidth", &o))
		hand.palmWidth = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "wristPosition", &o))
		hand.wristPosition = VuoPoint3d_valueFromJson(o);

	if (json_object_object_get_ex(js, "pinchAmount", &o))
		hand.pinchAmount = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "grabAmount", &o))
		hand.grabAmount = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "timeVisible", &o))
		hand.timeVisible = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "isLeftHand", &o))
		hand.isLeftHand = VuoBoolean_valueFromJson(o);

	if (json_object_object_get_ex(js, "confidence", &o))
		hand.confidence = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "fingers", &o))
		hand.fingers = VuoList_VuoLeapPointable_valueFromJson(o);

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

	json_object *rotationObject = VuoPoint4d_jsonFromValue(value.rotation);
	json_object_object_add(js, "rotation", rotationObject);

	json_object *palmPositionObject = VuoPoint3d_jsonFromValue(value.palmPosition);
	json_object_object_add(js, "palmPosition", palmPositionObject);

	json_object *palmVelocityObject = VuoPoint3d_jsonFromValue(value.palmVelocity);
	json_object_object_add(js, "palmVelocity", palmVelocityObject);

	json_object *sphereRadiusObject = VuoReal_jsonFromValue(value.sphereRadius);
	json_object_object_add(js, "sphereRadius", sphereRadiusObject);

	json_object *sphereCenterObject = VuoPoint3d_jsonFromValue(value.sphereCenter);
	json_object_object_add(js, "sphereCenter", sphereCenterObject);

	json_object *palmWidthObject = VuoReal_jsonFromValue(value.palmWidth);
	json_object_object_add(js, "palmWidth", palmWidthObject);

	json_object *wristPositionObject = VuoPoint3d_jsonFromValue(value.wristPosition);
	json_object_object_add(js, "wristPosition", wristPositionObject);

	json_object *pinchAmountObject = VuoReal_jsonFromValue(value.pinchAmount);
	json_object_object_add(js, "pinchAmount", pinchAmountObject);

	json_object *grabAmountObject = VuoReal_jsonFromValue(value.grabAmount);
	json_object_object_add(js, "grabAmount", grabAmountObject);

	json_object *timeVisibleObject = VuoReal_jsonFromValue(value.timeVisible);
	json_object_object_add(js, "timeVisible", timeVisibleObject);

	json_object *isLeftHandObject = VuoBoolean_jsonFromValue(value.isLeftHand);
	json_object_object_add(js, "isLeftHand", isLeftHandObject);

	json_object *confidenceObject = VuoReal_jsonFromValue(value.confidence);
	json_object_object_add(js, "confidence", confidenceObject);

	json_object *pointablesObject = VuoList_VuoLeapPointable_jsonFromValue(value.fingers);
	json_object_object_add(js, "fingers", pointablesObject);

	return js;
}

/**
 * @ingroup VuoLeapPointable
 * Returns a compact string representation of @c value.
 */
char * VuoLeapHand_summaryFromValue(const VuoLeapHand value)
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
