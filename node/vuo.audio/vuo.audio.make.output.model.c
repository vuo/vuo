/**
 * @file
 * vuo.audio.make.output.model node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Specify Audio Output by Model",
					  "keywords" : [ "sound", "output", "speaker", "music", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) modelCode,
		VuoOutputData(VuoAudioOutputDevice) device
)
{
	*device = VuoAudioOutputDevice_make(-1, modelCode, NULL, -1);
}
