/**
 * @file
 * vuo.time.seconds.bpm node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Convert Seconds to Beats Per Minute",
					  "keywords" : [ "tempo", "music" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal, {"name":"Seconds per Beat", "default":0.5,"suggestedMin":0.0001,"suggestedMax":2.}) secondsPerBeat,
		VuoOutputData(VuoReal, {"name":"Beats per Minute"}) beatsPerMinute
)
{
	*beatsPerMinute = 60./secondsPerBeat;
}
