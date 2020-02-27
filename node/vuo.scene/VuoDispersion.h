/**
 * @file
 * VuoDispersion C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoDispersion_struct { void *l; } * VuoList_VuoDispersion;
#define VuoList_VuoDispersion_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDispersion VuoDispersion
 * The pattern over which a function is applied.
 *
 * @{
 */

/**
 * The pattern over which a function is applied.
 */
typedef enum {
	VuoDispersion_Linear,
	VuoDispersion_Radial
} VuoDispersion;

VuoDispersion VuoDispersion_makeFromJson(struct json_object * js);
struct json_object * VuoDispersion_getJson(const VuoDispersion value);
VuoList_VuoDispersion VuoDispersion_getAllowedValues(void);
char * VuoDispersion_getSummary(const VuoDispersion value);

/**
 * Automatically generated function.
 */
///@{
VuoDispersion VuoDispersion_makeFromString(const char *str);
char * VuoDispersion_getString(const VuoDispersion value);
void VuoDispersion_retain(VuoDispersion value);
void VuoDispersion_release(VuoDispersion value);
///@}

/**
 * @}
 */
