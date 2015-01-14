/**
 * @file
 * vuo.math.multiply.integer node implementation.
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
		VuoInputData(VuoList_VuoInteger) terms,
		VuoOutputData(VuoInteger) product
)
{
	unsigned long termsCount = VuoListGetCount_VuoInteger(terms);
	if (termsCount == 0)
	{
		*product = 0;
	}
	else
	{
		*product = 1;
		for (unsigned long i = 1; i <= termsCount; ++i)
			*product *= VuoListGetValueAtIndex_VuoInteger(terms, i);
	}
}
