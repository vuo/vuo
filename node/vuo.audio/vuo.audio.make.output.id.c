/**
 * @file
 * vuo.audio.make.output.id node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Make Audio Output from ID",
					  "keywords" : [ "sound", "output", "speaker", "music", "device" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"name":"ID"}) id,
		VuoOutputData(VuoAudioOutputDevice) device
)
{
	*device = VuoAudioOutputDevice_make(id, VuoText_make(""), -1);
}
