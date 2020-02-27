/**
 * @file
 * VuoControlCode C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef void * VuoList_VuoControlCode;
#define VuoList_VuoControlCode_TYPE_DEFINED
/// @}

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

VuoControlCode VuoControlCode_makeFromJson(struct json_object *js);
struct json_object *VuoControlCode_getJson(const VuoControlCode value);
VuoList_VuoControlCode VuoControlCode_getAllowedValues(void);
char *VuoControlCode_getSummary(const VuoControlCode value);
VuoText VuoControlCode_makeText(VuoControlCode code);

#define VuoControlCode_SUPPORTS_COMPARISON
bool VuoControlCode_areEqual(const VuoControlCode valueA, const VuoControlCode valueB);
bool VuoControlCode_isLessThan(const VuoControlCode valueA, const VuoControlCode valueB);

/**
 * Automatically generated function.
 */
///@{
VuoControlCode VuoControlCode_makeFromString(const char *str);
char *VuoControlCode_getString(const VuoControlCode value);
void VuoControlCode_retain(VuoControlCode value);
void VuoControlCode_release(VuoControlCode value);
///@}

/**
 * @}
 */


