/**
 * @file
 * vuo.time.format node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Format Date-Time",
					  "keywords" : [ "text" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ "ShowSecondTicks.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) time,
	VuoInputData(VuoTimeFormat, {"default":"datetime-long-12"}) format,
	VuoOutputData(VuoText) formattedTime
)
{
	*formattedTime = VuoTime_format(time, format);
}
