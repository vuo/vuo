/**
 * @file
 * vuo.audio.get.input node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioInputDevice.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Get Audio Input Values",
					  "keywords" : [ "sound", "input", "microphone", "music", "listen", "device" ],
					  "version" : "1.2.0",
					  "dependencies" : [
						  "VuoAudio",
					  ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioInputDevice) device,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText) modelCode,
		VuoOutputData(VuoInteger) channelCount
)
{
	VuoAudioInputDevice realizedDevice;
	if (VuoAudioInputDevice_realize(device, &realizedDevice))
	{
		*id = realizedDevice.id;
		*name = realizedDevice.name;
		*modelCode = realizedDevice.modelUid;
		*channelCount = realizedDevice.channelCount;
	}
	else
	{
		*id = device.id;
		*name = device.name;
		*modelCode = device.modelUid;
		*channelCount = device.channelCount;
	}
}
