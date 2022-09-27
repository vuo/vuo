/**
 * @file
 * VuoSpeechVoice implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSpeechVoice.h"
#include "VuoList_VuoSpeechVoice.h"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Speech Voice",
	"description" : "An identifier for a speech synthesizer voice.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoText",
		"VuoList_VuoSpeechVoice"
	]
});
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "com.apple.speech.synthesis.voice.samantha.premium"
 * }
 */
VuoSpeechVoice VuoSpeechVoice_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);
	return VuoText_make(valueAsString);
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoSpeechVoice_getJson(const VuoSpeechVoice value)
{
	return VuoText_getJson(value);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoSpeechVoice VuoSpeechVoice_getAllowedValues(void)
{
	NSArray *voices = [NSSpeechSynthesizer availableVoices];
	int voiceCount = [voices count];

	VuoList_VuoSpeechVoice l = VuoListCreate_VuoSpeechVoice();
	for (int i = 0; i < voiceCount; ++i)
	{
		NSDictionary *voiceAttributes = [NSSpeechSynthesizer attributesForVoice:[voices objectAtIndex:i]];

		// I tried filtering by VoiceShowInFullListOnly,
		// but it doesn't seem to have any correlation to whether the voice's checkbox is checked in
		// System Settings > Accessibility > Spoken Content > System Voice > Manage Voices.
//		NSNumber *n = [voiceAttributes objectForKey:@"VoiceShowInFullListOnly"];
//		if (n && [n integerValue] == 0)
//			continue;

		VuoListAppendValue_VuoSpeechVoice(l, VuoText_make([[voiceAttributes objectForKey:NSVoiceIdentifier] UTF8String]));
	}

	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoSpeechVoice_getSummary(const VuoSpeechVoice value)
{
	if (VuoText_isEmpty(value))
		goto bad;

	NSString *voiceName = [NSString stringWithUTF8String:value];
	if (!voiceName)
		goto bad;

	NSDictionary *voiceAttributes = [NSSpeechSynthesizer attributesForVoice:voiceName];
	if (!voiceAttributes)
		goto bad;

	const char *actualVoiceName = [[voiceAttributes objectForKey:NSVoiceName] UTF8String];
	if (!actualVoiceName)
		goto bad;

	return strdup(actualVoiceName);

bad:
	return strdup("No voice");
}

/**
 * Returns true if the two values are equal.
 */
bool VuoSpeechVoice_areEqual(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB)
{
	return VuoText_areEqual(valueA, valueB);
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoSpeechVoice_isLessThan(const VuoSpeechVoice valueA, const VuoSpeechVoice valueB)
{
	return VuoText_isLessThan(valueA, valueB);
}
