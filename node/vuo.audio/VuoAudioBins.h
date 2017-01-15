/**
 * @file
 * VuoAudioBins C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOAUDIOBINS_H
#define VUOAUDIOBINS_H

/// @{
typedef const struct VuoList_VuoAudioBins_struct { void *l; } * VuoList_VuoAudioBins;
#define VuoList_VuoAudioBins_TYPE_DEFINED
/// @}

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
	VuoAudioBins_2047 = 4096 	// 4096
} VuoAudioBins;

VuoAudioBins VuoAudioBins_makeFromJson(struct json_object * js);
struct json_object * VuoAudioBins_getJson(const VuoAudioBins value);
VuoList_VuoAudioBins VuoAudioBins_getAllowedValues(void);
char * VuoAudioBins_getSummary(const VuoAudioBins value);

/// @{
/**
 * Automatically generated function.
 */
VuoAudioBins VuoAudioBins_makeFromString(const char *str);
char * VuoAudioBins_getString(const VuoAudioBins value);
void VuoAudioBins_retain(VuoAudioBins value);
void VuoAudioBins_release(VuoAudioBins value);
/// @}

/**
 * @}
*/

#endif
