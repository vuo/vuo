/**
 * @file
 * vuo.scene.make.triangle node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make 3D Triangle",
					  "keywords" : [ "equilateral", "3-gon", "3gon", "shape" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoOutputData(VuoSceneObject) object
)
{
	VuoMesh mesh = VuoMesh_makeEquilateralTriangle();

	*object = VuoSceneObject_make(mesh, VuoShader_make_VuoGenericType1(material), transform, NULL);
}
