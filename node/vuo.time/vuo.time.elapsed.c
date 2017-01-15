/**
 * @file
 * vuo.time.elapsed node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRelativeTime.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Subtract Times",
					  "keywords" : [ "date", "difference", "duration", "elapsed" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ "ShowElapsedTime.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) a,
	VuoInputData(VuoTime) b,
	VuoOutputData(VuoRelativeTime) elapsedTime
)
{
	*elapsedTime = b - a;
}
