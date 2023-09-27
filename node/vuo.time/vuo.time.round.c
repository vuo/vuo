/**
 * @file
 * vuo.time.round node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRoundingMethod.h"
#include "VuoTime.h"
#include "VuoTimeUnit.h"

VuoModuleMetadata({
					  "title" : "Round Date-Time",
					  "keywords" : [
						  "near", "close", "approximate", "integer", "whole",
						  "millennium", "century", "decade", "year", "quarter", "month",
						  "week", "sunday", "monday", "day", "hour", "half-hour", "quarter-hour",
						  "minute", "second"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ "ShowSecondTicks.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) time,
	VuoInputData(VuoTimeUnit, {"default":"hour"}) unit,
	VuoInputData(VuoRoundingMethod, {"default":"nearest"}) roundingMethod,
	VuoOutputData(VuoTime) roundedTime
)
{
	*roundedTime = VuoTime_round(time, unit, roundingMethod);
}
