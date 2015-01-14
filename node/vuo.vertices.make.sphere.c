/**
 * @file
 * vuo.vertices.make.sphere node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "VuoVerticesParametric.h"

VuoModuleMetadata({
					 "title" : "Make Sphere Vertices",
					 "description" :
						 "<p>Creates a set of vertices in the shape of a sphere.</p> \
						 <p>The sphere has a diameter of 1 and is centered at (0,0,0).</p> \
						 <p>The sphere has vertices along its latitude (row) and longitude (column) lines. \
						 `rows` and `columns` are the number of latitude and longitude lines. \
						 With more rows and columns, the sphere has a rounder shape but may take longer to render.</p>",
					 "keywords" : [ "mesh", "3d", "scene", "sphere", "ball", "round", "ellipsoid", "circle", "globe", "shape" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoVerticesParametric"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":0}) rows,
		VuoInputData(VuoInteger, {"default":16,"suggestedMin":0}) columns,
		VuoOutputData(VuoList_VuoVertices) vertices
)
{
	char *xExp = "sin(DEG2RAD*((u-.5)*360)) * cos(DEG2RAD*((v-.5)*180)) / 2.";
	char *yExp = "sin(DEG2RAD*((v-.5)*180)) / 2.";
	char *zExp = "cos(DEG2RAD*((u-.5)*360)) * cos(DEG2RAD*((v-.5)*180)) / 2.";

	char *uExp = "u";
	char *vExp = "v";

	*vertices = VuoVerticesParametric_generate( xExp, yExp, zExp, uExp, vExp,
													 rows, columns,
													 false,		// close u
													 false);	// close v
}
