/**
 * @file
 * vuo.transform.make.quaternion node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Quaternion Transform",
					 "keywords" : [ "homogenous", "xyzw", "translation", "rotation", "scale", "shift", "move", "position",
						 "angle", "axis", "size", "grow", "shrink" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) translation,
		VuoInputData(VuoPoint4d, {"default":{"x":0,"y":0,"z":0,"w":1}}) rotation,
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":1,"z":1}}) scale,
		VuoOutputData(VuoTransform) transform
)
{
	*transform = VuoTransform_makeQuaternion(translation, rotation, scale);
}
