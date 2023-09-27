/**
 * @file
 * VuoLeapPointableType C type definition.
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
 * @defgroup VuoLeapPointableType VuoLeapPointableType
 * Whether a pointable is a finger or a tool.
 *
 * @{
 */

/**
 * Whether a pointable is a finger or a tool.
 */
typedef enum {
	VuoLeapPointableType_Finger,
	VuoLeapPointableType_Tool
} VuoLeapPointableType;

#include "VuoList_VuoLeapPointableType.h"

VuoLeapPointableType VuoLeapPointableType_makeFromJson(struct json_object * js);
struct json_object * VuoLeapPointableType_getJson(const VuoLeapPointableType value);
VuoList_VuoLeapPointableType VuoLeapPointableType_getAllowedValues(void);
char * VuoLeapPointableType_getSummary(const VuoLeapPointableType value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoLeapPointableType_getString(const VuoLeapPointableType value);
void VuoLeapPointableType_retain(VuoLeapPointableType value);
void VuoLeapPointableType_release(VuoLeapPointableType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
