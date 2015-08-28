/**
 * @file
 * VuoWindowReference interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOWINDOWREFERENCE_H
#define VUOWINDOWREFERENCE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoWindowReference VuoWindowReference
 * A unique ID for a window being displayed by the composition.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoReal.h"

/**
 * A unique ID for a window being displayed by the composition.
 */
typedef int64_t VuoWindowReference;

VuoWindowReference VuoWindowReference_make(void *window);

VuoWindowReference VuoWindowReference_valueFromJson(struct json_object * js);
struct json_object * VuoWindowReference_jsonFromValue(const VuoWindowReference value);
char * VuoWindowReference_summaryFromValue(const VuoWindowReference value);

VuoReal VuoWindowReference_getAspectRatio(const VuoWindowReference value);
void VuoWindowReference_getContentSize(const VuoWindowReference value, VuoInteger *width, VuoInteger *height);

/// @{
/**
 * Automatically generated function.
 */
VuoWindowReference VuoWindowReference_valueFromString(const char *str);
char * VuoWindowReference_stringFromValue(const VuoWindowReference value);
void VuoWindowReference_retain(VuoWindowReference value);
void VuoWindowReference_release(VuoWindowReference value);
/// @}

/**
 * @}
 */

#endif
