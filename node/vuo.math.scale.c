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
					  "description" :
						  "<p>Converts a number from one unit of measure to another.</p> \
						  <ul> \
						  <li>`value` — The number to scale.</li> \
						  <li>`start`, `end` — Two numbers in the same units as `value`.</li> \
						  <li>`scaledStart`, `scaledEnd` — Two corresponding numbers in the same units as `scaledValue`.</li> \
						  </ul> \
						  <p>Example: To convert 2 hours to minutes, set `value` to 2, `start` to 0, `end` to 1, \
						  `scaledStart` to 0, and `scaledEnd` to 60. The output is 120 (minutes).</p> \
						  <p>Example: To convert a value that falls between 0 and 100 to a value that falls between 20 and 30, \
						  set `start` to 0, `end` to 100, `scaledStart` to 20, and `scaledEnd` to 30.</p> \
						  ",
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
