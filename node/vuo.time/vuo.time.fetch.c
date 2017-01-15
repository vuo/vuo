/**
 * @file
 * vuo.time.fetch node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Fetch Date-Time",
					  "keywords" : [ "wall", "clock", "watch", "current" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ "ShowElapsedTime.vuo", "ShowSecondTicks.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoOutputData(VuoTime) time
)
{
	*time = VuoTime_getCurrent();
}
