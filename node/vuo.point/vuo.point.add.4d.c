/**
 * @file
 * vuo.point.add.4d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Add 4D Points",
					  "keywords" : [ "sum", "plus", "+", "vector" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint4d) terms,
		VuoOutputData(VuoPoint4d) sum
)
{
	*sum = (VuoPoint4d){0,0,0,0};
	unsigned long termsCount = VuoListGetCount_VuoPoint4d(terms);
	for (unsigned long i = 0; i < termsCount; ++i)
		*sum = VuoPoint4d_add(*sum, (VuoPoint4d)VuoListGetValueAtIndex_VuoPoint4d(terms, i));
}
