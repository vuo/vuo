/**
 * @file
 * vuo.audio.get.output node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioOutputDevice.h"
#include "VuoAudio.h"

VuoModuleMetadata({
					  "title" : "Get Audio Output Values",
					  "keywords" : [ "sound", "output", "speaker", "music", "device" ],
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
		VuoInputData(VuoAudioOutputDevice) device,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText) modelCode,
		VuoOutputData(VuoInteger) channelCount
)
{
	VuoAudioOutputDevice realizedDevice;
	if (VuoAudioOutputDevice_realize(device, &realizedDevice))
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
