/**
 * @file
 * VuoDmxColorMap C type definition.
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
 * @defgroup VuoDmxColorMap VuoDmxColorMap
 * How to convert between a VuoColor and a set of DMX channels.
 *
 * @{
 */

/**
 * How to convert between a VuoColor and a set of DMX channels.
 */
typedef enum
{
	VuoDmxColorMap_RGB,
	VuoDmxColorMap_RGBA,
	VuoDmxColorMap_RGBAW,
	VuoDmxColorMap_RGBW,
	VuoDmxColorMap_WWCW,
	VuoDmxColorMap_CMY,
	VuoDmxColorMap_HSL,
} VuoDmxColorMap;

#include "VuoList_VuoDmxColorMap.h"

VuoDmxColorMap VuoDmxColorMap_makeFromJson(struct json_object *js);
struct json_object *VuoDmxColorMap_getJson(const VuoDmxColorMap value);
VuoList_VuoDmxColorMap VuoDmxColorMap_getAllowedValues(void);
char *VuoDmxColorMap_getSummary(const VuoDmxColorMap value);

/**
 * Automatically generated function.
 */
///@{
char *VuoDmxColorMap_getString(const VuoDmxColorMap value);
void VuoDmxColorMap_retain(VuoDmxColorMap value);
void VuoDmxColorMap_release(VuoDmxColorMap value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
