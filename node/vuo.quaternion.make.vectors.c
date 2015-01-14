/**
 * @file
 * vuo.quaternion.make.vectors node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Quaternion from Vectors",
					 "description" :
						"<p>Calculates a quaternion that represents a rotation in 3D space.</p> \
						<p>Quaternions are useful because, unlike rotations expressed in Euler angles \
						(yaw, pitch, and roll; or angles about the x, y, and z axis), \
						they avoid the problem of gimbal lock. A quaternion is calculated from a 3D point \
						(the axis) and an angle of rotation about the axis.</p> \
						<p><ul> \
						<li>`fromVector` — A direction in 3D space.</li> \
						<li>`toVector` — A direction in 3D space.</li> \
						<li>`quaternion` — A unit (normalized) quaternion representing the difference between the directions.</li> \
						</ul></p>",
					 "keywords" : [ "homogenous", "xyzw", "rotation", "angle", "versor", "from", "vectors" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
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
