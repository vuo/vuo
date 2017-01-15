/**
 * @file
 * VuoLeapHand C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPHAND_H
#define VUOLEAPHAND_H

#include "VuoInteger.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoReal.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapPointable.h"


/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapHand VuoLeapHand
 * Physical characteristics of a detected hand.
 *
 * @{
 */

/**
 * The Hand class reports the physical characteristics of a detected hand.
 */
typedef struct
{
	VuoInteger id;
	VuoPoint4d rotation;		// Quaternion.
	VuoPoint3d palmPosition;
	VuoPoint3d palmVelocity;
	VuoReal sphereRadius;
	VuoPoint3d sphereCenter;

	VuoReal palmWidth;			// VuoCoordinates
	VuoPoint3d wristPosition;	// VuoCoordinates
	VuoReal pinchAmount;		// 0,1 range
	VuoReal grabAmount;			// 0,1 range
	VuoReal timeVisible;
	VuoBoolean isLeftHand;		// Is this *not* your right hand?
	VuoReal confidence;			// 0,1 range

	VuoList_VuoLeapPointable fingers;

} VuoLeapHand;

VuoLeapHand VuoLeapHand_makeFromJson(struct json_object * js);
struct json_object * VuoLeapHand_getJson(const VuoLeapHand value);
char * VuoLeapHand_getSummary(const VuoLeapHand value);

VuoLeapHand VuoLeapHand_make(
		VuoInteger id,
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
		VuoList_VuoLeapPointable fingers);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapHand VuoLeapHand_makeFromString(const char * initializer);
char * VuoLeapHand_getString(const VuoLeapHand value);
void VuoLeapHand_retain(VuoLeapHand value);
void VuoLeapHand_release(VuoLeapHand value);
/// @}

/**
 * @}
 */

#endif
