/**
 * @file
 * vuo.video.make.audioframe node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioFrame.h"

VuoModuleMetadata({
					 "title" : "Make Audio Frame",
					  "keywords" : [
						  "timestamp"
					  ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) channels,
		VuoInputData(VuoReal, { "default":0, "auto":"-inf", "autoSupersedesDefault":true }) timestamp,
		VuoOutputData(VuoAudioFrame) audioFrame
)
{
	*audioFrame = VuoAudioFrame_make(channels, timestamp);
}
