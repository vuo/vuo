/**
 * @file
 * VuoSpeechVoice C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoSpeechVoice VuoSpeechVoice
 * An identifier for a speech synthesizer voice.
 *
 * @{
 */

/**
 * An identifier for a speech synthesizer voice.
 */
typedef VuoText VuoSpeechVoice;

#define VuoSpeechVoice_SUPPORTS_COMPARISON
#include "VuoList_VuoSpeechVoice.h"

VuoSpeechVoice VuoSpeechVoice_makeFromJson(struct json_object *js);
struct json_object *VuoSpeechVoice_getJson(const VuoSpeechVoice value);
VuoList_VuoSpeechVoice VuoSpeechVoice_getAllowedValues(void);
char *VuoSpeechVoice_getSummary(const VuoSpeechVoice value);

bool VuoSpeechVoice_areEqual(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB);
bool VuoSpeechVoice_isLessThan(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoSpeechVoice_getString(const VuoSpeechVoice value);
void VuoSpeechVoice_retain(VuoSpeechVoice value);
void VuoSpeechVoice_release(VuoSpeechVoice value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
