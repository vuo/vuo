/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoCubemap.h"
#include "VuoShader.h"

VuoModuleMetadata({
					"title" : "Make Cube",
					"keywords" : [
						"3D", "box", "d6", "hexahedron", "Platonic", "rectangular", "square", "shape", "object",
						"cubemap", "skybox", "environment map", "360", "180", "VR", "360vr",
					],
					"version" : "1.1.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage", "VuoCubemap" ]
						}
					},
					"node": {
						"exampleCompositions" : [ "DisplayCube.vuo" ]
					}
				});

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoOutputData(VuoSceneObject) cube
)
{
	*cube = VuoSceneObject_makeCube_VuoGenericType1(transform, material);
	VuoSceneObject_setName(*cube, VuoText_make("Cube"));
}
