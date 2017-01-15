/**
 * @file
 * vuo.scene.make.sphere node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
					  "title" : "Make Sphere",
					  "keywords" : [ "mesh", "3d", "scene", "sphere", "ball", "round", "ellipsoid", "circle", "globe", "shape" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						  }
					  },
					  "dependencies" : [ "VuoMeshParametric" ],
					  "node" : {
						  "exampleCompositions" : [ "DisplaySphere.vuo", "MoveBeadsOnString.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoInteger, {"default":32,"suggestedMin":4, "suggestedMax":256}) rows,
	VuoInputData(VuoInteger, {"default":32,"suggestedMin":4, "suggestedMax":256}) columns,
	VuoOutputData(VuoSceneObject) object
)
{
	char *xExp = "sin((u-.5)*360) * cos((v-.5)*180) / 2.";
	char *yExp = "sin((v-.5)*180) / 2.";
	char *zExp = "cos((u-.5)*360) * cos((v-.5)*180) / 2.";

	unsigned int r = MAX(4, MIN(512, rows));
	unsigned int c = MAX(4, MIN(512, columns));

	VuoMesh mesh = VuoMeshParametric_generate(	0,
												xExp, yExp, zExp,
												c, r,
												true,		// close u
												0, 1,
												false,		// close v
												0, 1);

	*object = VuoSceneObject_make(mesh, VuoShader_make_VuoGenericType1(material), transform, NULL);
}
