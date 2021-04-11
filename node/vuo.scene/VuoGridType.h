/**
 * @file
 * VuoGridType C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoGridType_struct { void *l; } * VuoList_VuoGridType;
#define VuoList_VuoGridType_TYPE_DEFINED
/// @}


/**
 * @ingroup VuoTypes
 * @defgroup VuoGridType VuoGridType
 * Defines different ways of displaying a grid.
 *
 * @{
 */

/**
 * Defines different ways of displaying a grid.
 */
typedef enum {
	VuoGridType_Horizontal,
	VuoGridType_Vertical,
	VuoGridType_HorizontalAndVertical
} VuoGridType;

VuoGridType VuoGridType_makeFromJson(struct json_object * js);
struct json_object * VuoGridType_getJson(const VuoGridType value);
VuoList_VuoGridType VuoGridType_getAllowedValues(void);
char * VuoGridType_getSummary(const VuoGridType value);

#define VuoGridType_SUPPORTS_COMPARISON
bool VuoGridType_areEqual(VuoGridType a, VuoGridType b);
bool VuoGridType_isLessThan(VuoGridType a, VuoGridType b);

/**
 * Automatically generated function.
 */
///@{
VuoGridType VuoGridType_makeFromString(const char *str);
char * VuoGridType_getString(const VuoGridType value);
void VuoGridType_retain(VuoGridType value);
void VuoGridType_release(VuoGridType value);
///@}

/**
 * @}
 */

