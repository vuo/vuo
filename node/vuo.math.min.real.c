/**
 * @file
 * vuo.math.min.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <float.h>

VuoModuleMetadata({
					 "title" : "Find Minimum",
					 "description" :
						"<p>Selects the smallest term.</p> \
						<p>If there are no terms, outputs 0.</p>",
					 "keywords" : [ "less", "least", "small", "few", "low", "<", "bottom", "lower", "limit", "bound", "range" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) terms,
		VuoOutputData(VuoReal) min
)
{
	unsigned long termsCount = VuoListGetCount_VuoReal(terms);

	if (!termsCount)
	{
		*min = 0;
		return;
	}

	*min = DBL_MAX;
	for (unsigned long i = 1; i <= termsCount; ++i)
	{
		double value = VuoListGetValueAtIndex_VuoReal(terms, i);
		if (value < *min)
			*min = value;
	}
}
