/**
 * @file
 * vuo.scene.transform node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Transform 3D Object",
					 "keywords" : [ "scenegraph", "composite", "rotate", "scale", "translate" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoSceneObject) transformedObject
)
{
	(*transformedObject) = object;
	(*transformedObject).transform = VuoTransform_composite(object.transform, transform);
}
