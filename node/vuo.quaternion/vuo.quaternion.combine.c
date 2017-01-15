/**
 * @file
 * vuo.quaternion.combine node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine Quaternion Rotations",
					 "keywords" : [ "composite", "product", "multiply", "*", "homogenous", "xyzw", "rotation", "angle", "versor", "merge" ],
					 "version" : "3.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint4d) values,
		VuoOutputData(VuoPoint4d) composite
)
{
	*composite = VuoPoint4d_make(0,0,0,1);

	unsigned long quaternionsCount = VuoListGetCount_VuoPoint4d(values);
	for (unsigned long i = 1; i <= quaternionsCount; ++i)
		*composite = VuoTransform_quaternionComposite(*composite, VuoListGetValue_VuoPoint4d(values, i));
}
