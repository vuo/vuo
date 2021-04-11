/**
 * @file
 * VuoSpeechVoice C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef void * VuoList_VuoSpeechVoice;
#define VuoList_VuoSpeechVoice_TYPE_DEFINED
/// @}

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

VuoSpeechVoice VuoSpeechVoice_makeFromJson(struct json_object *js);
struct json_object *VuoSpeechVoice_getJson(const VuoSpeechVoice value);
VuoList_VuoSpeechVoice VuoSpeechVoice_getAllowedValues(void);
char *VuoSpeechVoice_getSummary(const VuoSpeechVoice value);

#define VuoSpeechVoice_SUPPORTS_COMPARISON
bool VuoSpeechVoice_areEqual(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB);
bool VuoSpeechVoice_isLessThan(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB);

/**
 * Automatically generated function.
 */
///@{
VuoSpeechVoice VuoSpeechVoice_makeFromString(const char *str);
char *VuoSpeechVoice_getString(const VuoSpeechVoice value);
void VuoSpeechVoice_retain(VuoSpeechVoice value);
void VuoSpeechVoice_release(VuoSpeechVoice value);
///@}

/**
 * @}
 */
