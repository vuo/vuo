/**
 * @file
 * VuoDistribution3d C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoDistribution3d;
#define VuoList_VuoDistribution3d_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDistribution3d VuoDistribution3d
 * A distribution of points in 3D space.
 *
 * @{
 */

/**
 * A distribution of points in 3D space.
 */
typedef enum
{
	VuoDistribution3d_CubeVolume,
	VuoDistribution3d_CubeSurface,
	VuoDistribution3d_SphereVolume,
	VuoDistribution3d_SphereSurface,
} VuoDistribution3d;

VuoDistribution3d VuoDistribution3d_makeFromJson(struct json_object *js);
struct json_object *VuoDistribution3d_getJson(const VuoDistribution3d value);
VuoList_VuoDistribution3d VuoDistribution3d_getAllowedValues(void);
char *VuoDistribution3d_getSummary(const VuoDistribution3d value);

#define VuoDistribution3d_SUPPORTS_COMPARISON
bool VuoDistribution3d_areEqual(const VuoDistribution3d valueA, const VuoDistribution3d valueB);
bool VuoDistribution3d_isLessThan(const VuoDistribution3d valueA, const VuoDistribution3d valueB);

/**
 * Automatically generated function.
 */
///@{
VuoDistribution3d VuoDistribution3d_makeFromString(const char *str);
char *VuoDistribution3d_getString(const VuoDistribution3d value);
void VuoDistribution3d_retain(VuoDistribution3d value);
void VuoDistribution3d_release(VuoDistribution3d value);
///@}

/**
 * @}
 */
