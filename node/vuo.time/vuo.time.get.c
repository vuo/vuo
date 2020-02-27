/**
 * @file
 * vuo.time.get node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Get Date-Time Values",
					  "keywords" : [ "split", "components" ],
					  "version" : "1.0.1",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ "ShowSecondTicks.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) time,
	VuoOutputData(VuoInteger) year,
	VuoOutputData(VuoInteger, {"name":"Day of Year"}) dayOfYear,
	VuoOutputData(VuoInteger) month,
	VuoOutputData(VuoInteger, {"name":"Day of Month"}) dayOfMonth,
	VuoOutputData(VuoInteger) week,
	VuoOutputData(VuoWeekday, {"name":"Day of Week"}) dayOfWeek,
	VuoOutputData(VuoInteger) hour,
	VuoOutputData(VuoInteger) minute,
	VuoOutputData(VuoReal) second
)
{
	if (!VuoTime_getComponents(time, year, dayOfYear, month, dayOfMonth, week, dayOfWeek, hour, minute, second))
	{
		*year = 0;
		*dayOfYear = 0;
		*month = 0;
		*dayOfMonth = 0;
		*week = 0;
		*dayOfWeek = VuoWeekday_Sunday;
		*hour = 0;
		*minute = 0;
		*second = 0;
	}
}
