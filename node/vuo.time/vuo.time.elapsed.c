/**
 * @file
 * vuo.time.elapsed node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRelativeTime.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Subtract Date-Times",
					  "keywords" : [ "difference", "duration", "relative" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : []
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
