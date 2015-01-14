/**
 * @file
 * vuo.point.make.4d.quaternion node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Quaternion from Angle",
					 "description" :
						"<p>Calculates a quaternion that represents a rotation in 3D space.</p> \
						<p>Quaternions are useful because, unlike rotations expressed in Euler angles \
						(yaw, pitch, and roll; or angles about the x, y, and z axis), \
						they avoid the problem of gimbal lock. A quaternion is calculated from a 3D point \
						(the axis) and an angle of rotation about the axis.</p> \
						<p><ul> \
						<li>`axis` — A point in 3D space. Only its direction from the origin matters; its magnitude is not important.</li> \
						<li>`angle` — The angle of rotation about the axis, in degrees.</li> \
						<li>`quaternion` — A unit (normalized) quaternion.</li> \
						</ul></p>",
					 "keywords" : [ "homogenous", "xyzw", "rotation", "angle", "versor" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":0,"z":0}}) axis,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoOutputData(VuoPoint4d) quaternion
)
{
	*quaternion = VuoTransform_quaternionFromAxisAngle(axis, angle * M_PI/180.);
}
