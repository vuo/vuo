/**
 * @file
 * vuo.scene.tc.remove node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
	VuoSceneObject_apply(modifiedObject, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
		if (currentObject->mesh)
			for (unsigned int i = 0; i < currentObject->mesh->submeshCount; ++i)
			{
				if (currentObject->mesh->submeshes[i].textureCoordinates)
				{
					free(currentObject->mesh->submeshes[i].textureCoordinates);
					currentObject->mesh->submeshes[i].textureCoordinates = NULL;
				}

				// Clearing `textureCoordinateOffset` may change the stride if it's automatically calculated, so preserve the current stride.
				currentObject->mesh->submeshes[i].glUpload.combinedBufferStride = VuoSubmesh_getStride(currentObject->mesh->submeshes[i]);

				currentObject->mesh->submeshes[i].glUpload.textureCoordinateOffset = NULL;
			}
	});
}
