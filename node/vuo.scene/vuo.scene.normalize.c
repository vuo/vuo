/**
 * @file
 * vuo.scene.normalize node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoPoint3d.h"

VuoModuleMetadata({
					 "title" : "Normalize 3D Object",
					 "keywords" : [ "constrain", "scale", "size", "fit", "center", "rectify", "shrink" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoBoolean, {"default":true}) center,
		VuoInputData(VuoBoolean, {"default":true}) fit,
		VuoOutputData(VuoSceneObject) normalizedObject
)
{
	VuoSceneObject copy = VuoSceneObject_copy(object);

	if(center)
		VuoSceneObject_center(copy);

	if(fit)
		VuoSceneObject_normalize(copy);

	*normalizedObject = copy;
}
