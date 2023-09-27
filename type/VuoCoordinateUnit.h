/**
 * @file
 * VuoCoordinateUnit C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoCoordinateUnit_h
#define VuoCoordinateUnit_h

#ifdef __cplusplus
extern "C" {
#endif

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
	VuoCoordinateUnit_VuoCoordinates,
} VuoCoordinateUnit;

#define VuoCoordinateUnit_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoCoordinateUnit.h"

VuoCoordinateUnit VuoCoordinateUnit_makeFromJson(struct json_object *js);
struct json_object *VuoCoordinateUnit_getJson(const VuoCoordinateUnit value);
VuoList_VuoCoordinateUnit VuoCoordinateUnit_getAllowedValues(void);
char *VuoCoordinateUnit_getSummary(const VuoCoordinateUnit value);

bool VuoCoordinateUnit_areEqual(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB);
bool VuoCoordinateUnit_isLessThan(const VuoCoordinateUnit valueA, const VuoCoordinateUnit valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoCoordinateUnit_getString(const VuoCoordinateUnit value);
void VuoCoordinateUnit_retain(VuoCoordinateUnit value);
void VuoCoordinateUnit_release(VuoCoordinateUnit value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
