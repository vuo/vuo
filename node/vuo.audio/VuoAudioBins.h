/**
 * @file
 * VuoAudioBins C type definition.
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
 * @defgroup VuoAudioBins VuoAudioBins
 * An enum defining different bin sizes for handling audio.
 *
 * @{
 */

/**
 * An enum defining different bin sizes for handling audio.
 */
typedef enum {					// actual samples per-bin
	VuoAudioBins_3 = 8,			// 8
	VuoAudioBins_7 = 16,		// 16
	VuoAudioBins_15 = 32,		// 32
	VuoAudioBins_31 = 64,		// 64
	VuoAudioBins_63 = 128,		// 128
	VuoAudioBins_127 = 256,		// 256
	VuoAudioBins_255 = 512,		// 512
	VuoAudioBins_511 = 1024,	// 1024
	VuoAudioBins_1023 = 2048,	// 2048
	VuoAudioBins_2047 = 4096, 	// 4096
	VuoAudioBins_4095 = 8192, 	// 8192
	VuoAudioBins_8191 = 16384, 	// 16384
} VuoAudioBins;

#include "VuoList_VuoAudioBins.h"

VuoAudioBins VuoAudioBins_makeFromJson(struct json_object * js);
struct json_object * VuoAudioBins_getJson(const VuoAudioBins value);
VuoList_VuoAudioBins VuoAudioBins_getAllowedValues(void);
char * VuoAudioBins_getSummary(const VuoAudioBins value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoAudioBins_getString(const VuoAudioBins value);
void VuoAudioBins_retain(VuoAudioBins value);
void VuoAudioBins_release(VuoAudioBins value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
