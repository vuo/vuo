/**
 * @file
 * VuoProjectionType C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoProjectionType_struct { void *l; } * VuoList_VuoProjectionType;
#define VuoList_VuoProjectionType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoProjectionType VuoProjectionType
 * An enum defining different projection mapping techniques.
 *
 * @{
 */

/**
 * An enum defining different projection mapping techniques.
 */
typedef enum {
	VuoProjectionType_Perspective,
	VuoProjectionType_Affine
} VuoProjectionType;

VuoProjectionType VuoProjectionType_makeFromJson(struct json_object * js);
struct json_object * VuoProjectionType_getJson(const VuoProjectionType value);
VuoList_VuoProjectionType VuoProjectionType_getAllowedValues(void);
char * VuoProjectionType_getSummary(const VuoProjectionType value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoProjectionType_getString(const VuoProjectionType value);
void VuoProjectionType_retain(VuoProjectionType value);
void VuoProjectionType_release(VuoProjectionType value);
/// @}

/**
 * @}
*/
