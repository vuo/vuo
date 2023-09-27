/**
 * @file
 * VuoControlCode C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoControlCode VuoControlCode
 * Defines commonly used invisible characters (new line, tab, space, etc).
 *
 * @{
 */

/**
 * Defines commonly used invisible characters (new line, tab, space, etc).
 */
typedef enum
{
	VuoControlCode_NewLineUnix,
	VuoControlCode_NewLineWindows,
	VuoControlCode_NewLineMacOS9,
	VuoControlCode_Tab,
	VuoControlCode_Space,
	VuoControlCode_EmSpace,
	VuoControlCode_EnSpace
} VuoControlCode;

#define VuoControlCode_SUPPORTS_COMPARISON
#include "VuoList_VuoControlCode.h"

VuoControlCode VuoControlCode_makeFromJson(struct json_object *js);
struct json_object *VuoControlCode_getJson(const VuoControlCode value);
VuoList_VuoControlCode VuoControlCode_getAllowedValues(void);
char *VuoControlCode_getSummary(const VuoControlCode value);
VuoText VuoControlCode_makeText(VuoControlCode code);

bool VuoControlCode_areEqual(const VuoControlCode valueA, const VuoControlCode valueB);
bool VuoControlCode_isLessThan(const VuoControlCode valueA, const VuoControlCode valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoControlCode_getString(const VuoControlCode value);
void VuoControlCode_retain(VuoControlCode value);
void VuoControlCode_release(VuoControlCode value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
