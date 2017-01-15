/**
 * @file
 * vuo.time.relative.make node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRelativeTime.h"

VuoModuleMetadata({
					 "title" : "Make Relative Date-Time",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) years,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":11}) months,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":30}) days,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":23}) hours,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":59}) minutes,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":59.99}) seconds,
		VuoOutputData(VuoRelativeTime) relativeTime
)
{
	*relativeTime = VuoRelativeTime_make(years, months, days, hours, minutes, seconds);
}
