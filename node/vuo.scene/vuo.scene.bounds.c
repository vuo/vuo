/**
 * @file
 * vuo.scene.bounds node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPoint3d.h"

VuoModuleMetadata({
					 "title" : "Get 3D Object Bounds",
					 "keywords" : [ "box", "aabb", "axis", "collider", "collision", "volume" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "DisplaySceneWithFloor.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoPoint3d) center,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoReal) height,
		VuoOutputData(VuoReal) depth
)
{
	VuoBox bounds = VuoSceneObject_bounds(object);

	*center = bounds.center;
	*width = bounds.size.x;
	*height = bounds.size.y;
	*depth = bounds.size.z;
}
