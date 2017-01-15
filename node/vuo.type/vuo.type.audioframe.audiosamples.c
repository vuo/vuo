/**
 * @file
 * vuo.type.audioframe.audiosamples node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "../vuo.video/VuoAudioFrame.h"
#include "VuoList_VuoAudioSamples.h"

VuoModuleMetadata({
					 "title" : "Convert Frame to Audio",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoAudioFrame) frame,
		VuoOutputData(VuoList_VuoAudioSamples) samples
)
{
	*samples = frame.samples;
}
