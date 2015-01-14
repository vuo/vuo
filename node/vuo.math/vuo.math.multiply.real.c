/**
 * @file
 * vuo.math.multiply.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Multiply",
					 "keywords" : [ "product", "times", "*", "arithmetic", "calculate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) terms,
		VuoOutputData(VuoReal) product
)
{
	unsigned long termsCount = VuoListGetCount_VuoReal(terms);
	if (termsCount == 0)
	{
		*product = 0;
	}
	else
	{
		*product = 1;
		for (unsigned long i = 1; i <= termsCount; ++i)
			*product *= VuoListGetValueAtIndex_VuoReal(terms, i);
	}
}
