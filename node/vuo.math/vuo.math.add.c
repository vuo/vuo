/**
 * @file
 * vuo.math.add node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Add",
					 "keywords" : [ "sum", "plus", "total", "+", "arithmetic", "calculate", "vector", "point" ],
					 "version" : "2.1.0",
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
	VuoGenericType1 tmp = VuoGenericType1_makeFromJson(NULL);
	*sum = tmp;
	unsigned long termsCount = VuoListGetCount_VuoGenericType1(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*sum = VuoGenericType1_add(*sum, (VuoGenericType1)VuoListGetValue_VuoGenericType1(values, i));
}
