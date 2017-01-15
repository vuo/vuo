/**
 * @file
 * vuo.audio.make.input.id node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioInputDevice.h"

VuoModuleMetadata({
					  "title" : "Make Audio Input from ID",
					  "keywords" : [ "sound", "input", "microphone", "music", "listen", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"name":"ID"}) id,
		VuoOutputData(VuoAudioInputDevice) device
)
{
	*device = VuoAudioInputDevice_make(id, VuoText_make(""), -1);
}
