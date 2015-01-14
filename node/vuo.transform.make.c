/**
 * @file
 * vuo.transform.make node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Transform",
					 "description" :
						 "<p>Creates a transform that can change a 3D object's position, rotation, or size.</p> \
						 <p><ul> \
						 <li>`translation` — The amount to shift the object's position, in Vuo coordinates. \
						 If this is (0,0,0), the position will be unchanged.</li> \
						 <li>`rotation` — The amount to rotate the object, in degrees (Euler angles). \
						 If this is (0,0,0), the object's rotation will be unchanged.</li> \
						 <li>`scale` — The scale factor. If this is (1,1,1), the object's size will be unchanged.</li> \
						 </ul></p>",
					 "keywords" : [ "Euler", "translation", "rotation", "scale", "shift", "move", "position",
						 "angle", "yaw", "pitch", "roll", "axis", "size", "grow", "shrink" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) translation,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedMin":{"x":0,"y":0,"z":0},"suggestedMax":{"x":360,"y":360,"z":360},"suggestedStep":{"x":1,"y":1,"z":1}}) rotation,
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":1,"z":1}}) scale,
		VuoOutputData(VuoTransform) transform
)
{
	*transform = VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale);
}
