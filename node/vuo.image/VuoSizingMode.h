/**
 * @file
 * VuoSizingMode C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoSizingMode VuoSizingMode
 * An enum defining different image fill modes.
 *
 * @{
 */

/**
 * An enum defining different image fill modes.
 */
typedef enum {
	VuoSizingMode_Stretch,
	VuoSizingMode_Fit,
	VuoSizingMode_Fill,
	VuoSizingMode_Proportional,
} VuoSizingMode;

#include "VuoList_VuoSizingMode.h"

VuoSizingMode VuoSizingMode_makeFromJson(struct json_object * js);
struct json_object * VuoSizingMode_getJson(const VuoSizingMode value);
VuoList_VuoSizingMode VuoSizingMode_getAllowedValues(void);
char * VuoSizingMode_getSummary(const VuoSizingMode value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoSizingMode_getString(const VuoSizingMode value);
void VuoSizingMode_retain(VuoSizingMode value);
void VuoSizingMode_release(VuoSizingMode value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
