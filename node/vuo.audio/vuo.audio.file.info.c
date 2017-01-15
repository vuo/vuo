/**
 * @file
 * vuo.audio.file.info node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioFile.h"

VuoModuleMetadata({
					  "title" : "Get Audio File Info",
					  "keywords" : [
						  "sound", "music", "listen",
						  "wave",
						  "aiff",
						  "mp3", "mp2",
						  "aac", "m4a", "ac3",
						  "3gp", "amr"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoAudioFile"
					  ],
					  "node" : {
						  "isInterface" : true,
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
	VuoAudioFile_getInfo(url, duration, channelCount, sampleRate);
}
