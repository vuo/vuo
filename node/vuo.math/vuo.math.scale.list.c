/**
 * @file
 * vuo.math.scale.list node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Scale List",
					  "keywords" : [ "multiply", "product", "times", "*", "arithmetic", "calculate" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1) scale,
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoList_VuoGenericType1) scaledList
)
{
	*scaledList = VuoListCreate_VuoGenericType1();
	unsigned long listCount = VuoListGetCount_VuoGenericType1(list);
	for (unsigned long i = 1; i <= listCount; ++i)
	{
		VuoGenericType1 product = VuoGenericType1_scale(
						VuoListGetValue_VuoGenericType1(list, i),
						scale);
		VuoListAppendValue_VuoGenericType1(*scaledList, product);
	}
}
