/**
 * @file
 * VuoDisplacement C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoDisplacement_struct { void *l; } * VuoList_VuoDisplacement;
#define VuoList_VuoDisplacement_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDisplacement VuoDisplacement
 * The direction in which to move vertices.
 *
 * @{
 */

/**
 * The direction in which to move vertices.
 */
typedef enum {
	VuoDisplacement_Transverse,
	VuoDisplacement_Longitudinal
} VuoDisplacement;

VuoDisplacement VuoDisplacement_makeFromJson(struct json_object * js);
struct json_object * VuoDisplacement_getJson(const VuoDisplacement value);
VuoList_VuoDisplacement VuoDisplacement_getAllowedValues(void);
char * VuoDisplacement_getSummary(const VuoDisplacement value);

/**
 * Automatically generated function.
 */
///@{
VuoDisplacement VuoDisplacement_makeFromString(const char *str);
char * VuoDisplacement_getString(const VuoDisplacement value);
void VuoDisplacement_retain(VuoDisplacement value);
void VuoDisplacement_release(VuoDisplacement value);
///@}

/**
 * @}
 */

