/**
 * @file
 * vuo.point.add node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Add Points",
					  "keywords" : [ "sum", "plus", "+", "vector" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) terms,
		VuoOutputData(VuoGenericType1) sum
)
{
	VuoGenericType1 tmp = {0};
	*sum = tmp;
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(terms);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*sum = VuoGenericType1_add(*sum, (VuoGenericType1)VuoListGetValueAtIndex_VuoGenericType1(terms, i));
}
