/**
 * @file
 * VuoDistribution3d C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

#define VuoDistribution3d_SUPPORTS_COMPARISON
#include "VuoList_VuoDistribution3d.h"

VuoDistribution3d VuoDistribution3d_makeFromJson(struct json_object *js);
struct json_object *VuoDistribution3d_getJson(const VuoDistribution3d value);
VuoList_VuoDistribution3d VuoDistribution3d_getAllowedValues(void);
char *VuoDistribution3d_getSummary(const VuoDistribution3d value);

bool VuoDistribution3d_areEqual(const VuoDistribution3d valueA, const VuoDistribution3d valueB);
bool VuoDistribution3d_isLessThan(const VuoDistribution3d valueA, const VuoDistribution3d valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoDistribution3d_getString(const VuoDistribution3d value);
void VuoDistribution3d_retain(VuoDistribution3d value);
void VuoDistribution3d_release(VuoDistribution3d value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
