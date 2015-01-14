/**
 * @file
 * vuo.point.multiply.quaternion node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine Quaternion Rotations",
					 "description" :
						"<p>Finds the rotation that would result after rotating by each of the terms.</p> \
						<p>If there are no terms, outputs the identity rotation.</p> \
						<p>The rotations are performed in the order of the terms (rotation by the first quaternion, \
						followed by rotation by the second quaternion, etc.).</p> \
						<p>The output is a unit (normalized) quaternion.</p>",
					 "keywords" : [ "composite", "product", "multiply", "*", "homogenous", "xyzw", "rotation", "angle", "versor" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint4d) terms,
		VuoOutputData(VuoPoint4d) composite
)
{
	*composite = VuoPoint4d_make(0,0,0,1);

	unsigned long quaternionsCount = VuoListGetCount_VuoPoint4d(terms);
	for (unsigned long i = 1; i <= quaternionsCount; ++i)
		*composite = VuoTransform_quaternionComposite(*composite, VuoListGetValueAtIndex_VuoPoint4d(terms, i));
}
