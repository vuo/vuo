/**
 * @file
 * vuo.scene.make node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Object",
					 "keywords" : [ "mesh", "model", "vertices", "shader", "texture", "draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "2.0.0",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoMesh) mesh,
		VuoInputData(VuoShader) shader,
		VuoInputData(VuoTransform) transform,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeMesh(mesh, shader, transform);
}
