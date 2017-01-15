/**
 * @file
 * vuo.audio.make.input.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioInputDevice.h"

VuoModuleMetadata({
					  "title" : "Make Audio Input from Name",
					  "keywords" : [ "sound", "input", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) name,
		VuoOutputData(VuoAudioInputDevice) device
)
{
	*device = VuoAudioInputDevice_make(-1, name, -1);
}
