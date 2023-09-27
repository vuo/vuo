/**
 * @file
 * vuo.time.equal.dateTime node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTime.h"
#include "VuoTimeUnit.h"

VuoModuleMetadata({
					  "title" : "Are Equal (Time of Day)",
					  "keywords" : [ "==", "same", "identical", "equivalent", "match", "compare", "approximate", "tolerance", "conditional" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoList_VuoTime) times,
	VuoInputData(VuoReal, {"default":1,"suggestedMin":0.0}) tolerance,
	VuoInputData(VuoTimeUnit, {"default":"second",
							   "includeValues": ['hour', 'half-hour', 'quarter-hour', 'minute', 'second']
							  }) toleranceUnit,
	VuoOutputData(VuoBoolean) equal
)
{
	*equal = VuoTime_areTimesOfDayEqualWithinTolerance(times, tolerance, toleranceUnit);
}
