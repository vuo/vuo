/**
 * @file
 * vuo.audio.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoAudioSamples.h"

VuoModuleMetadata({
					  "title" : "Are Samples Populated",
					  "keywords" : [ "sound", "amplitudes", "non-empty", "nonempty" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoAudioSamples) samples,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoAudioSamples_isPopulated(samples);
}
