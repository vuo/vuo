/**
 * @file
 * vuo.audio.make.input.model node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioInputDevice.h"

VuoModuleMetadata({
					  "title" : "Specify Audio Input by Model",
					  "keywords" : [ "sound", "input", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) modelCode,
		VuoOutputData(VuoAudioInputDevice) device
)
{
	*device = VuoAudioInputDevice_make(-1, modelCode, NULL, -1);
}
