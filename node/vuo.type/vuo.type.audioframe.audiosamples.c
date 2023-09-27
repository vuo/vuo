/**
 * @file
 * vuo.type.audioframe.audiosamples node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioFrame.h"
#include "VuoList_VuoAudioSamples.h"

VuoModuleMetadata({
					 "title" : "Convert Frame to Audio",
					 "keywords" : [ "timestamp" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoAudioFrame) frame,
		VuoOutputData(VuoList_VuoAudioSamples) channels
)
{
	*channels = frame.channels;
}
