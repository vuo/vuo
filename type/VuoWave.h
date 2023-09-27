/**
 * @file
 * VuoWave C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoWave_h
#define VuoWave_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoWave VuoWave
 * An enum defining different waves.
 *
 * @{
 */

/**
 * An enum defining different waves
 */
typedef enum {
	VuoWave_Sine,
	VuoWave_Triangle,
	VuoWave_Sawtooth
} VuoWave;

#include "VuoList_VuoWave.h"

VuoWave VuoWave_makeFromJson(struct json_object * js);
struct json_object * VuoWave_getJson(const VuoWave value);
VuoList_VuoWave VuoWave_getAllowedValues(void);
char * VuoWave_getSummary(const VuoWave value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoWave_getString(const VuoWave value);
void VuoWave_retain(VuoWave value);
void VuoWave_release(VuoWave value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
