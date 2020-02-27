/**
 * @file
 * VuoPixelShape C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef void * VuoList_VuoPixelShape;
#define VuoList_VuoPixelShape_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoPixelShape VuoPixelShape
 * The shape to use for enlarged pixels.
 *
 * @{
 */

/**
 * The shape to use for enlarged pixels.
 */
typedef enum
{
	VuoPixelShape_Rectangle,
	VuoPixelShape_Triangle,
	VuoPixelShape_Hexagon
} VuoPixelShape;

VuoPixelShape VuoPixelShape_makeFromJson(struct json_object *js);
struct json_object *VuoPixelShape_getJson(const VuoPixelShape value);
VuoList_VuoPixelShape VuoPixelShape_getAllowedValues(void);
char *VuoPixelShape_getSummary(const VuoPixelShape value);

#define VuoPixelShape_SUPPORTS_COMPARISON
bool VuoPixelShape_areEqual(const VuoPixelShape valueA, const VuoPixelShape valueB);
bool VuoPixelShape_isLessThan(const VuoPixelShape valueA, const VuoPixelShape valueB);

/**
 * Automatically generated function.
 */
///@{
VuoPixelShape VuoPixelShape_makeFromString(const char *str);
char *VuoPixelShape_getString(const VuoPixelShape value);
void VuoPixelShape_retain(VuoPixelShape value);
void VuoPixelShape_release(VuoPixelShape value);
///@}

/**
 * @}
 */
