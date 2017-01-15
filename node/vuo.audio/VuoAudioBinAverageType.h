/**
 * @file
 * VuoAudioBinAverageType C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOAUDIOBINAVERAGETYPE_H
#define VUOAUDIOBINAVERAGETYPE_H

/// @{
typedef const struct VuoList_VuoAudioBinAverageType_struct { void *l; } * VuoList_VuoAudioBinAverageType;
#define VuoList_VuoAudioBinAverageType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoAudioBinAverageType VuoAudioBinAverageType
 * An enum defining different averaging types for a set of audio samples.
 *
 * @{
 */

/**
 * An enum defining different averaging types for a set of audio samples.
 */
typedef enum {
	VuoAudioBinAverageType_None,		///< aka - linear
	VuoAudioBinAverageType_Quadratic,
	VuoAudioBinAverageType_Logarithmic 
} VuoAudioBinAverageType;

VuoAudioBinAverageType VuoAudioBinAverageType_makeFromJson(struct json_object * js);
struct json_object * VuoAudioBinAverageType_getJson(const VuoAudioBinAverageType value);
VuoList_VuoAudioBinAverageType VuoAudioBinAverageType_getAllowedValues(void);
char * VuoAudioBinAverageType_getSummary(const VuoAudioBinAverageType value);

/// @{
/**
 * Automatically generated function.
 */
VuoAudioBinAverageType VuoAudioBinAverageType_makeFromString(const char *str);
char * VuoAudioBinAverageType_getString(const VuoAudioBinAverageType value);
void VuoAudioBinAverageType_retain(VuoAudioBinAverageType value);
void VuoAudioBinAverageType_release(VuoAudioBinAverageType value);
/// @}

/**
 * @}
*/

#endif
