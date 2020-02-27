/**
 * @file
 * vuo.time.relative.get node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoRelativeTime.h"

VuoModuleMetadata({
					  "title" : "Get Relative Date-Time Values",
					  "keywords" : [ "split", "components" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : []
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoRelativeTime) relativeTime,
	VuoOutputData(VuoInteger) years,
	VuoOutputData(VuoInteger) months,
	VuoOutputData(VuoInteger) days,
	VuoOutputData(VuoInteger) hours,
	VuoOutputData(VuoInteger) minutes,
	VuoOutputData(VuoReal) seconds
)
{
	VuoRelativeTime_getComponents(relativeTime, years, months, days, hours, minutes, seconds);
}
