/**
 * @file
 * vuo.audio.get.output node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Get Audio Output Values",
					  "keywords" : [ "sound", "output", "speaker", "music", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioOutputDevice) device,
		VuoOutputData(VuoInteger, {"name":"ID"}) id,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoInteger) channelCount
)
{
	*id = device.id;
	*name = device.name;
	*channelCount = device.channelCount;
}
