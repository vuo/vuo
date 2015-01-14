/**
 * @file
 * vuo.math.scale node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <float.h>

VuoModuleMetadata({
					  "title" : "Scale",
					  "keywords" : [ "convert", "unit" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoReal,0.0) value,
		VuoInputData(VuoReal,0.0) start,
		VuoInputData(VuoReal,0.0) end,
		VuoInputData(VuoReal,0.0) scaledStart,
		VuoInputData(VuoReal,0.0) scaledEnd,
		VuoOutputData(VuoReal) scaledValue
)
{
	VuoReal range = end - start;
	if (range == 0)
		range = DBL_MIN;
	VuoReal scaledRange = scaledEnd - scaledStart;
	*scaledValue = (value - start) * scaledRange / range + scaledStart;
}
