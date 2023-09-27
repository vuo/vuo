/**
 * @file
 * VuoLeapFrame C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoLeapHand.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapHand.h"
#include "VuoList_VuoLeapPointable.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapFrame VuoLeapFrame
 * A frame of Leap Motion data.
 *
 * @{
 */

/**
 * A frame of Leap Motion data.
 */
typedef struct
{
	VuoInteger id;

/// @todo VuoLeapTransform motion;

	VuoList_VuoLeapHand hands;
	VuoList_VuoLeapPointable pointables;

/// @todo VuoList_VuoLeapGestureCircle circleGestures;
/// @todo VuoList_VuoLeapGestureSwipe swipeGestures;
/// @todo VuoList_VuoLeapGestureKeyTap keyTapGestures;
/// @todo VuoList_VuoLeapGestureScreenTap screenTapGestures;

} VuoLeapFrame;

VuoLeapFrame VuoLeapFrame_makeFromJson(struct json_object * js);
struct json_object * VuoLeapFrame_getJson(const VuoLeapFrame value);
char * VuoLeapFrame_getSummary(const VuoLeapFrame value);

VuoLeapFrame VuoLeapFrame_make(VuoInteger id, VuoList_VuoLeapHand hands, VuoList_VuoLeapPointable pointables);

/// @{
/**
 * Automatically generated function.
 */
char * VuoLeapFrame_getString(const VuoLeapFrame value);
void VuoLeapFrame_retain(VuoLeapFrame value);
void VuoLeapFrame_release(VuoLeapFrame value);
 /// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
