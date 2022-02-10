/**
 * @file
 * vuo.time.fetch node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Fetch Date-Time",
					  "keywords" : [
						  "get",
						  "wall", "clock", "watch",
						  "current", "now", "today",
					  ],
					  "version" : "1.1.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ "ShowSecondTicks.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputEvent() fetch,
	VuoOutputData(VuoTime) time
)
{
	// This conditional is commented out to enable compositions
	// using the deprecated refresh port to continue working.
//	if (fetch)
	*time = VuoTime_getCurrent();
}
