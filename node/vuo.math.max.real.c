/**
 * @file
 * vuo.math.max.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <float.h>

VuoModuleMetadata({
					 "title" : "Find Maximum",
					 "description" :
						"<p>Selects the largest term.</p> \
						<p>If there are no terms, outputs 0.</p>",
					 "keywords" : [ "great", "large", "big", "high", "more", "most", ">", "top", "upper", "peak", "limit", "bound", "range" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) terms,
		VuoOutputData(VuoReal) max
)
{
	unsigned long termsCount = VuoListGetCount_VuoReal(terms);

	if (!termsCount)
	{
		*max = 0;
		return;
	}

	*max = -DBL_MAX;
	for (unsigned long i = 1; i <= termsCount; ++i)
	{
		double value = VuoListGetValueAtIndex_VuoReal(terms, i);
		if (value > *max)
			*max = value;
	}
}
