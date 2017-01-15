/**
 * @file
 * vuo.quaternion.make.vectors node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Quaternion from Vectors",
					 "keywords" : [ "homogenous", "xyzw", "rotation", "angle", "versor", "from", "vectors" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":0,"z":0}}) fromVector,
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":0,"z":0}}) toVector,
		VuoOutputData(VuoPoint4d) quaternion
)
{
	*quaternion = VuoTransform_quaternionFromVectors(fromVector, toVector);
}
