/**
 * @file
 * vuo.time.offset node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRelativeTime.h"
#include "VuoTime.h"

VuoModuleMetadata({
					  "title" : "Offset Date-Time",
					  "keywords" : [ "add", "shift", "slide" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTime) time,
	VuoInputData(VuoRelativeTime) offset,
	VuoOutputData(VuoTime) offsetTime
)
{
	*offsetTime = time + offset;
}
