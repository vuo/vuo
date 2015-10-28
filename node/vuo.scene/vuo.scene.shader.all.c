/**
 * @file
 * vuo.scene.shader.all node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObject.h"

VuoModuleMetadata({
					  "title" : "Change All Shaders",
					  "keywords" : [ "swap" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "ReplaceShaderForScene.vuo" ]
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
