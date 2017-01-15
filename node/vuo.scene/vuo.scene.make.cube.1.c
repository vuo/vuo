/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoShader.h"

VuoModuleMetadata({
					"title" : "Make Cube",
					"keywords" : [ "3D", "box", "d6", "hexahedron", "Platonic", "rectangular", "square" ],
					"version" : "1.0.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
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
	VuoShader shader = VuoShader_make_VuoGenericType1(material);
	*cube = VuoSceneObject_makeCube(transform, shader, shader, shader, shader, shader, shader);
}
