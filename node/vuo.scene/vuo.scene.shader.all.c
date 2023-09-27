/**
 * @file
 * vuo.scene.shader.all node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObject.h"

VuoModuleMetadata({
					  "title" : "Change All Shaders",
					  "keywords" : [ "swap", "replace", "texture", "material" ],
					  "version" : "2.0.1",
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

	VuoShader s = shader;
	if (!s)
		s = VuoShader_makeDefaultShader();

	VuoSceneObject_apply(copy, ^(VuoSceneObject currentObject, float modelviewMatrix[16]){
		if (!VuoSceneObject_getShader(currentObject))
			return;

		VuoSceneObject_setShader(currentObject, s);
	});

	*shadedObject = copy;
}
