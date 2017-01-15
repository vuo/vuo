/**
 * @file
 * vuo.audio.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					 "title" : "List Audio Devices",
					 "keywords" : [ "sound", "input", "microphone", "music", "listen" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoAudio"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoAudioInputDevice) inputDevices,
		VuoOutputData(VuoList_VuoAudioOutputDevice) outputDevices
)
{
	*inputDevices = VuoAudio_getInputDevices();
	*outputDevices = VuoAudio_getOutputDevices();
}
