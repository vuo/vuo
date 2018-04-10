/**
 * @file
 * vuo.time.equal.dateTime node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"
#include "VuoTimeUnit.h"

VuoModuleMetadata({
					  "title" : "Are Equal (Date-Time)",
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
	VuoInputData(VuoTimeUnit, {"default":"second"}) toleranceUnit,
	VuoOutputData(VuoBoolean) equal
)
{
	*equal = VuoTime_areEqualWithinTolerance(times, tolerance, toleranceUnit);
}
