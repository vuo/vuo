/**
 * @file
 * vuo.time.bpm.seconds node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Convert Beats Per Minute to Seconds",
					  "keywords" : [ "tempo" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":120.,"suggestedMin":0.0001,"suggestedMax":500.}) beatsPerMinute,
		VuoOutputData(VuoReal) secondsPerBeat
)
{
	*secondsPerBeat = 60./beatsPerMinute;
}
