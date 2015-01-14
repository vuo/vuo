/**
 * @file
 * vuo.math.add.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Add",
					 "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) terms,
		VuoOutputData(VuoReal) sum
)
{
	*sum = 0;
	unsigned long termsCount = VuoListGetCount_VuoReal(terms);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*sum += VuoListGetValueAtIndex_VuoReal(terms, i);
}
