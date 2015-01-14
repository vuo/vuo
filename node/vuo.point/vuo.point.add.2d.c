/**
 * @file
 * vuo.point.add.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Add 2D Points",
					  "keywords" : [ "sum", "plus", "+", "vector" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint2d) terms,
		VuoOutputData(VuoPoint2d) sum
)
{
	*sum = (VuoPoint2d){0,0};
	unsigned long termsCount = VuoListGetCount_VuoPoint2d(terms);
	for (unsigned long i = 0; i < termsCount; ++i)
		*sum = VuoPoint2d_add(*sum, (VuoPoint2d)VuoListGetValueAtIndex_VuoPoint2d(terms, i));
}
