/**
 * @file
 * vuo.math.add node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Add",
					 "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate", "vector", "point" ],
					 "version" : "2.0.0",
					 "genericTypes" : {
						 "VuoGenericType1" : {
							"defaultType" : "VuoReal",
							"compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						 }
					 },
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) values,
		VuoOutputData(VuoGenericType1) sum
)
{
	VuoGenericType1 tmp = {0};
	*sum = tmp;
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*sum = VuoGenericType1_add(*sum, (VuoGenericType1)VuoListGetValue_VuoGenericType1(values, i));
}
