/**
 * @file
 * vuo.scene.shader.all node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObject.h"

VuoModuleMetadata({
					  "title" : "Change All Shaders",
					  "keywords" : [ "swap", "replace", "texture", "material" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "PaintSceneWithCheckerboard.vuo", "CompareCameras.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoShader) shader,
	VuoOutputData(VuoSceneObject) shadedObject
)
{
	VuoSceneObject copy = VuoSceneObject_copy(object);

	VuoSceneObject_apply(&copy, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
							currentObject->shader = shader;
						 });

	*shadedObject = copy;
}
