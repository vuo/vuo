/**
 * @file
 * VuoVerticalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoVerticalAlignment_struct { void *l; } * VuoList_VuoVerticalAlignment;
#define VuoList_VuoVerticalAlignment_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoVerticalAlignment VuoVerticalAlignment
 * Vertical alignment.
 *
 * @{
 */

/**
 * Vertical alignment.
 */
typedef enum
{
	VuoVerticalAlignment_Top,
	VuoVerticalAlignment_Center,
	VuoVerticalAlignment_Bottom
} VuoVerticalAlignment;

VuoVerticalAlignment VuoVerticalAlignment_makeFromJson(struct json_object * js);
struct json_object * VuoVerticalAlignment_getJson(const VuoVerticalAlignment value);
VuoList_VuoVerticalAlignment VuoVerticalAlignment_getAllowedValues(void);
char * VuoVerticalAlignment_getSummary(const VuoVerticalAlignment value);

#define VuoVerticalAlignment_SUPPORTS_COMPARISON
bool VuoVerticalAlignment_areEqual(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB);
bool VuoVerticalAlignment_isLessThan(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB);

/**
 * Automatically generated function.
 */
///@{
VuoVerticalAlignment VuoVerticalAlignment_makeFromString(const char *str);
char * VuoVerticalAlignment_getString(const VuoVerticalAlignment value);
void VuoVerticalAlignment_retain(VuoVerticalAlignment value);
void VuoVerticalAlignment_release(VuoVerticalAlignment value);
///@}

/**
 * @}
 */


