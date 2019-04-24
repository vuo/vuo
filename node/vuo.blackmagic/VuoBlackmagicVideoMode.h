/**
 * @file
 * VuoBlackmagicVideoMode C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoBlackmagicVideoMode;
#define VuoList_VuoBlackmagicVideoMode_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlackmagicVideoMode VuoBlackmagicVideoMode
 * Video resolution and framerate.
 *
 * @{
 */

/**
 * Video resolution and framerate.
 */
typedef enum
{
	VuoBlackmagicVideoMode_NTSC,
	VuoBlackmagicVideoMode_NTSC2398,
	VuoBlackmagicVideoMode_NTSCp,
	VuoBlackmagicVideoMode_PAL,
	VuoBlackmagicVideoMode_PALp,
	VuoBlackmagicVideoMode_HD720p50,
	VuoBlackmagicVideoMode_HD720p5994,
	VuoBlackmagicVideoMode_HD720p60,
	VuoBlackmagicVideoMode_HD1080p2398,
	VuoBlackmagicVideoMode_HD1080p24,
	VuoBlackmagicVideoMode_HD1080p25,
	VuoBlackmagicVideoMode_HD1080p2997,
	VuoBlackmagicVideoMode_HD1080p30,
	VuoBlackmagicVideoMode_HD1080i50,
	VuoBlackmagicVideoMode_HD1080i5994,
	VuoBlackmagicVideoMode_HD1080i6000,
	VuoBlackmagicVideoMode_HD1080p50,
	VuoBlackmagicVideoMode_HD1080p5994,
	VuoBlackmagicVideoMode_HD1080p6000,
	VuoBlackmagicVideoMode_2k2398,
	VuoBlackmagicVideoMode_2k24,
	VuoBlackmagicVideoMode_2k25,
	VuoBlackmagicVideoMode_2kDCI2398,
	VuoBlackmagicVideoMode_2kDCI24,
	VuoBlackmagicVideoMode_2kDCI25,
	VuoBlackmagicVideoMode_4K2160p2398,
	VuoBlackmagicVideoMode_4K2160p24,
	VuoBlackmagicVideoMode_4K2160p25,
	VuoBlackmagicVideoMode_4K2160p2997,
	VuoBlackmagicVideoMode_4K2160p30,
	VuoBlackmagicVideoMode_4K2160p50,
	VuoBlackmagicVideoMode_4K2160p5994,
	VuoBlackmagicVideoMode_4K2160p60,
	VuoBlackmagicVideoMode_4kDCI2398,
	VuoBlackmagicVideoMode_4kDCI24,
	VuoBlackmagicVideoMode_4kDCI25,
} VuoBlackmagicVideoMode;

VuoBlackmagicVideoMode VuoBlackmagicVideoMode_makeFromJson(struct json_object *js);
struct json_object *VuoBlackmagicVideoMode_getJson(const VuoBlackmagicVideoMode value);
VuoList_VuoBlackmagicVideoMode VuoBlackmagicVideoMode_getAllowedValues(void);
char *VuoBlackmagicVideoMode_getSummary(const VuoBlackmagicVideoMode value);

VuoBlackmagicVideoMode VuoBlackmagicVideoMode_makeFromBMDDisplayMode(const uint32_t value);
uint32_t VuoBlackmagicVideoMode_getBMDDisplayMode(const VuoBlackmagicVideoMode value);

#define VuoBlackmagicVideoMode_SUPPORTS_COMPARISON
bool VuoBlackmagicVideoMode_areEqual(const VuoBlackmagicVideoMode valueA, const VuoBlackmagicVideoMode valueB);
bool VuoBlackmagicVideoMode_isLessThan(const VuoBlackmagicVideoMode valueA, const VuoBlackmagicVideoMode valueB);

/**
 * Automatically generated function.
 */
///@{
VuoBlackmagicVideoMode VuoBlackmagicVideoMode_makeFromString(const char *str);
char *VuoBlackmagicVideoMode_getString(const VuoBlackmagicVideoMode value);
void VuoBlackmagicVideoMode_retain(VuoBlackmagicVideoMode value);
void VuoBlackmagicVideoMode_release(VuoBlackmagicVideoMode value);
///@}

/**
 * @}
 */
