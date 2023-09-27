/**
 * @file
 * vuo.scene.transform node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Transform 3D Object (Transform)",
					 "keywords" : [ "scenegraph", "composite", "rotate", "scale", "translate" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoTransform) transform,
		VuoOutputData(VuoSceneObject) transformedObject
)
{
	*transformedObject = VuoSceneObject_copy(object);
	VuoSceneObject_transform(*transformedObject, transform);
}
