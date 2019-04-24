/**
 * @file
 * VuoDeinterlacing C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoDeinterlacing;
#define VuoList_VuoDeinterlacing_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDeinterlacing VuoDeinterlacing
 * How to convert interlaced video to progressive video.
 *
 * @{
 */

/**
 * How to convert interlaced video to progressive video.
 */
typedef enum
{
	VuoDeinterlacing_None,
	VuoDeinterlacing_AlternateFields,
	VuoDeinterlacing_BlendFields,
} VuoDeinterlacing;

VuoDeinterlacing VuoDeinterlacing_makeFromJson(struct json_object *js);
struct json_object *VuoDeinterlacing_getJson(const VuoDeinterlacing value);
VuoList_VuoDeinterlacing VuoDeinterlacing_getAllowedValues(void);
char *VuoDeinterlacing_getSummary(const VuoDeinterlacing value);

#define VuoDeinterlacing_SUPPORTS_COMPARISON
bool VuoDeinterlacing_areEqual(const VuoDeinterlacing valueA, const VuoDeinterlacing valueB);
bool VuoDeinterlacing_isLessThan(const VuoDeinterlacing valueA, const VuoDeinterlacing valueB);

/**
 * Automatically generated function.
 */
///@{
VuoDeinterlacing VuoDeinterlacing_makeFromString(const char *str);
char *VuoDeinterlacing_getString(const VuoDeinterlacing value);
void VuoDeinterlacing_retain(VuoDeinterlacing value);
void VuoDeinterlacing_release(VuoDeinterlacing value);
///@}

/**
 * @}
 */
