/**
 * @file
 * vuo.video.get.audioframe node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoAudioFrame.h"

VuoModuleMetadata({
					 "title" : "Get Frame Values (Audio)",
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
		VuoInputData(VuoAudioFrame) audioFrame,
		VuoOutputData(VuoList_VuoAudioSamples) channels,
		VuoOutputData(VuoReal) timestamp
)
{
	*channels = audioFrame.channels;
	*timestamp = audioFrame.timestamp;
}
