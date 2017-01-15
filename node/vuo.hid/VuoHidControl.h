/**
 * @file
 * VuoHidControl C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoHidControl VuoHidControl
 * Information about a control on a USB HID device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"


/**
 * Information about a control on a USB HID device.
 */
typedef struct
{
	VuoText name;		///< The control's display name.
	VuoInteger value;	///< The control's current or default value.
	VuoInteger min;		///< The minimum possible value the control can take.
	VuoInteger max;		///< The maximum possible value the control can take.
} VuoHidControl;

VuoHidControl VuoHidControl_makeFromJson(struct json_object *js);
struct json_object *VuoHidControl_getJson(const VuoHidControl value);
char *VuoHidControl_getSummary(const VuoHidControl value);

#define VuoHidControl_SUPPORTS_COMPARISON
bool VuoHidControl_areEqual(const VuoHidControl valueA, const VuoHidControl valueB);
bool VuoHidControl_isLessThan(const VuoHidControl valueA, const VuoHidControl valueB);

/**
 * Automatically generated function.
 */
///@{
VuoHidControl VuoHidControl_makeFromString(const char *str);
char *VuoHidControl_getString(const VuoHidControl value);
void VuoHidControl_retain(VuoHidControl value);
void VuoHidControl_release(VuoHidControl value);
///@}

/**
 * @}
 */

