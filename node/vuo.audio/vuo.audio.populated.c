/**
 * @file
 * vuo.audio.populated node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioSamples.h"

VuoModuleMetadata({
					  "title" : "Are Samples Populated",
					  "keywords" : [ "sound", "amplitudes", "empty", "non-empty", "nonempty" ],
					  "version" : "1.0.0",
					  "node": {
						  "isDeprecated": true,
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
