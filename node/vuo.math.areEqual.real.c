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
					 "description" :
						"<p>Outputs <i>true</i> if all terms are close to equal.</p> \
						<p>If there are no terms, outputs <i>true</i>.</p> \
						<p>`tolerance` is the maximum amount by which any two terms can differ to still be considered equal. \
						Because calculations on a computer are often inexact due to rounding errors, you would typically want \
						to make `tolerance` greater than 0.</p>",
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
