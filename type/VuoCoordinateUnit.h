/**
 * @file
 * VuoCoordinateUnit C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoCoordinateUnit;
#define VuoList_VuoCoordinateUnit_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoCoordinateUnit VuoCoordinateUnit
 * The unit a coordinate uses.
 *
 * @{
 */

/**
 * The unit a coordinate uses.
 */
typedef enum
{
	VuoCoordinateUnit_Points,
	VuoCoordinateUnit_Pixels,
} VuoCoordinateUnit;

VuoCoordinateUnit VuoCoordinateUnit_makeFromJson(struct json_object *js);
struct json_object *VuoCoordinateUnit_getJson(const VuoCoordinateUnit value);
VuoList_VuoCoordinateUnit VuoCoordinateUnit_getAllowedValues(void);
char *VuoCoordinateUnit_getSummary(const VuoCoordinateUnit value);

#define VuoCoordinateUnit_SUPPORTS_COMPARISON
bool VuoCoordinateUnit_areEqual(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB);
bool VuoCoordinateUnit_isLessThan(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB);

/**
 * Automatically generated function.
 */
///@{
VuoCoordinateUnit VuoCoordinateUnit_makeFromString(const char *str);
char *VuoCoordinateUnit_getString(const VuoCoordinateUnit value);
void VuoCoordinateUnit_retain(VuoCoordinateUnit value);
void VuoCoordinateUnit_release(VuoCoordinateUnit value);
///@}

/**
 * @}
 */
