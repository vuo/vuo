/**
 * @file
 * VuoLeapFrame C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPFRAME_H
#define VUOLEAPFRAME_H

#include "VuoInteger.h"

/// @{
#ifndef VuoList_VuoLeapPointable_TYPE_DEFINED
typedef void * VuoList_VuoLeapPointable;
#define VuoList_VuoLeapPointable_TYPE_DEFINED
#endif
typedef void * VuoList_VuoLeapHand;
#define VuoList_VuoLeapHand_TYPE_DEFINED
/// @}

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

VuoLeapFrame VuoLeapFrame_valueFromJson(struct json_object * js);
struct json_object * VuoLeapFrame_jsonFromValue(const VuoLeapFrame value);
char * VuoLeapFrame_summaryFromValue(const VuoLeapFrame value);

VuoLeapFrame VuoLeapFrame_make(VuoInteger id, VuoList_VuoLeapHand hands, VuoList_VuoLeapPointable pointables);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapFrame VuoLeapFrame_valueFromString(const char * initializer);
char * VuoLeapFrame_stringFromValue(const VuoLeapFrame value);
 /// @}

/**
 * @}
 */

#endif
