/**
 * @file
 * VuoDmxColorMap C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODMXCOLORMAP_H
#define VUODMXCOLORMAP_H

/// @{
typedef void * VuoList_VuoDmxColorMap;
#define VuoList_VuoDmxColorMap_TYPE_DEFINED
/// @}

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
	VuoDmxColorMap_WWCW
} VuoDmxColorMap;

VuoDmxColorMap VuoDmxColorMap_makeFromJson(struct json_object *js);
struct json_object *VuoDmxColorMap_getJson(const VuoDmxColorMap value);
VuoList_VuoDmxColorMap VuoDmxColorMap_getAllowedValues(void);
char *VuoDmxColorMap_getSummary(const VuoDmxColorMap value);

/**
 * Automatically generated function.
 */
///@{
VuoDmxColorMap VuoDmxColorMap_makeFromString(const char *str);
char *VuoDmxColorMap_getString(const VuoDmxColorMap value);
void VuoDmxColorMap_retain(VuoDmxColorMap value);
void VuoDmxColorMap_release(VuoDmxColorMap value);
///@}

/**
 * @}
 */

#endif // VUODMXCOLORMAP_H
