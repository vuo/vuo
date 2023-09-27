/**
 * @file
 * vuo.scene.combine node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Combine 3D Objects",
					 "keywords" : [ "scenegraph", "group", "join", "together", "merge" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoList_VuoSceneObject) childObjects,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeGroup(childObjects, transform);
}
