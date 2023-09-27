﻿/**
 * @file
 * vuo.time.bpm.seconds node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Convert Beats Per Minute to Seconds",
					  "keywords" : [ "tempo", "music" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal, {"name":"Beats per Minute", "default":120.,"suggestedMin":0.0001,"suggestedMax":500.}) beatsPerMinute,
		VuoOutputData(VuoReal, {"name":"Seconds per Beat"}) secondsPerBeat
)
{
	*secondsPerBeat = 60./beatsPerMinute;
}
