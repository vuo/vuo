/**
 * @file
 * vuo.type.audiosamples.audioframe node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioFrame.h"
#include "VuoList_VuoAudioSamples.h"

VuoModuleMetadata({
					 "title" : "Convert Audio to Frame",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoAudioSamples) channels,
		VuoOutputData(VuoAudioFrame) frame
)
{
	*frame = VuoAudioFrame_make(channels, VuoAudioFrame_NoTimestamp);
}
