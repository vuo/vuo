/**
 * @file
 * vuo.vertices.make.sphere node implementation.
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
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":4}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":4}) columns,
		VuoOutputData(VuoMesh) mesh
)
{
	char *xExp = "sin((u-.5)*360) * cos((v-.5)*180) / 2.";
	char *yExp = "sin((v-.5)*180) / 2.";
	char *zExp = "cos((u-.5)*360) * cos((v-.5)*180) / 2.";

	unsigned int _rows = MIN(512, MAX(4, rows));
	unsigned int _columns = MIN(512, MAX(4, columns));

	*mesh = VuoMeshParametric_generate(0,
													 xExp, yExp, zExp,
													 _columns, _rows,
													 true,		// close u
													 0, 1,
													 false,		// close v
													 0, 1
												);
}
