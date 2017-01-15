/**
 * @file
 * vuo.time.after.timeOfDay node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Is Time-of-Day After",
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
	VuoOutputData(VuoBoolean) after
)
{
	*after = VuoTime_isTimeOfDayLessThan(b, a, startOfDay);
}
