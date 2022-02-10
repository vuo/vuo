/**
 * @file
 * VuoOrientation C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoOrientation_struct { void *l; } * VuoList_VuoOrientation;
#define VuoList_VuoOrientation_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoOrientation VuoOrientation
 * Horizontal or vertical alignment.
 *
 * @{
 */

/**
 * Horizontal or vertical alignment.
 *
 * @version200New
 */
typedef enum
{
	VuoOrientation_Horizontal,
	VuoOrientation_Vertical
} VuoOrientation;

VuoOrientation VuoOrientation_makeFromJson(struct json_object * js);
struct json_object * VuoOrientation_getJson(const VuoOrientation value);
VuoList_VuoOrientation VuoOrientation_getAllowedValues(void);
char * VuoOrientation_getSummary(const VuoOrientation value);

#define VuoOrientation_SUPPORTS_COMPARISON
bool VuoOrientation_areEqual(const VuoOrientation valueA, const VuoOrientation valueB);
bool VuoOrientation_isLessThan(const VuoOrientation valueA, const VuoOrientation valueB);

/**
 * Automatically generated function.
 */
///@{
char * VuoOrientation_getString(const VuoOrientation value);
void VuoOrientation_retain(VuoOrientation value);
void VuoOrientation_release(VuoOrientation value);
///@}

/**
 * @}
 */


