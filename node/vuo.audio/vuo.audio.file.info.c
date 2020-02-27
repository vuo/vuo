/**
 * @file
 * vuo.audio.file.info node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioFile.h"

VuoModuleMetadata({
					  "title" : "Get Audio File Info",
					  "keywords" : [
						  "information",
						  "sound", "music", "listen",
						  "wave",
						  "aiff",
						  "mp3", "mp2",
						  "aac", "m4a", "ac3",
						  "3gp", "amr"
					  ],
					  "version" : "1.0.1",
					  "dependencies" : [
						  "VuoAudioFile"
					  ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoOutputData(VuoReal) duration,
		VuoOutputData(VuoInteger) channelCount,
		VuoOutputData(VuoReal) sampleRate
//		VuoOutputData(VuoDictionary_VuoText) metadata
)
{
	if (!VuoAudioFile_getInfo(url, duration, channelCount, sampleRate))
	{
		VUserLog("Error: Couldn't read '%s'.", url);
		*duration = 0;
		*channelCount = 0;
		*sampleRate = 0;
	}
}
