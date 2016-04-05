/**
 * @file
 * vuo.vertices.make.sphere node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoMeshParametric.h"

VuoModuleMetadata({
					 "title" : "Make Sphere Mesh",
					 "keywords" : [ "mesh", "3d", "scene", "sphere", "ball", "round", "ellipsoid", "circle", "globe", "shape" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoMeshParametric"
					 ],
					 "node": {
						  "exampleCompositions" : [ ],
						  "isDeprecated": true
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":2}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":2}) columns,
		VuoOutputData(VuoMesh) mesh
)
{
	char *xExp = "sin((u-.5)*360) * cos((v-.5)*180) / 2.";
	char *yExp = "sin((v-.5)*180) / 2.";
	char *zExp = "cos((u-.5)*360) * cos((v-.5)*180) / 2.";

	*mesh = VuoMeshParametric_generate(0,
													 xExp, yExp, zExp,
													 columns, rows,
													 true,		// close u
													 0, 1,
													 false,		// close v
													 0, 1
												);
}
