/**
 * @file
 * vuo.time.before.timeOfDay node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Is Time-of-Day Before",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) a,
	VuoInputData(VuoTime) b,
	VuoInputData(VuoTime, {"name":"Start of Day", "default":0}) startOfDay,	/// @todo input editor should omit date
	VuoOutputData(VuoBoolean) before
)
{
	*before = VuoTime_isTimeOfDayLessThan(a, b, startOfDay);
}
