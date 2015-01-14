/**
 * @file
 * vuo.math.areEqual.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are Equal",
					 "keywords" : [ "same", "identical", "equivalent", "match", "compare", "approximate", "tolerance", "conditional" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) terms,
		VuoInputData(VuoReal, {"default":0.01}) tolerance,
		VuoOutputData(VuoBoolean) equal
)
{
	unsigned long termsCount = VuoListGetCount_VuoReal(terms);
	if (termsCount > 1)
	{
		VuoReal min, max;
		min = max = VuoListGetValueAtIndex_VuoReal(terms, 1);
		for (unsigned long i = 2; i <= termsCount; ++i)
		{
			VuoReal term = VuoListGetValueAtIndex_VuoReal(terms, i);
			if (term < min)
				min = term;
			if (term > max)
				max = term;
		}
		*equal = (max - min <= tolerance);
	}
	else
	{
		*equal = true;
	}
}
