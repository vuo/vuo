/**
 * @file
 * vuo.scene.tc.remove node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoPoint3d.h"

VuoModuleMetadata({
					 "title" : "Remove Texture Coordinates",
					 "keywords" : [ "3D" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "TileStarfield.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoOutputData(VuoSceneObject) modifiedObject
)
{
	*modifiedObject = VuoSceneObject_copy(object);
	VuoSceneObject_apply(*modifiedObject, ^(VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoMesh m = VuoMesh_copyShallow(VuoSceneObject_getMesh(currentObject));
		VuoMesh_removeTextureCoordinates(m);
		VuoSceneObject_setMesh(currentObject, m);
	});
}
