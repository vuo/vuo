/**
 * @file
 * vuo.point.add.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Add 3D Points",
					  "keywords" : [ "sum", "plus", "+", "vector" ],
					  "version" : "1.0.0",
					  "node" : {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint3d) terms,
		VuoOutputData(VuoPoint3d) sum
)
{
	*sum = (VuoPoint3d){0,0,0};
	unsigned long termsCount = VuoListGetCount_VuoPoint3d(terms);
	for (unsigned long i = 0; i < termsCount; ++i)
		*sum = VuoPoint3d_add(*sum, (VuoPoint3d)VuoListGetValueAtIndex_VuoPoint3d(terms, i));
}
