/**
 * @file
 * vuo.vertices.make.triangle node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Triangle Vertices",
					 "description" :
						 "<p>Creates 3 vertices in the shape of an equilateral triangle.</p> \
						 <p>The triangle is on the XY plane, with its point upward on the Y axis. \
						 It has sides of length 1 and is centered at (0,0,0).</p>",
					 "keywords" : [ "equilateral", "3-gon", "3gon", "shape" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoVertices) vertices
)
{
	VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
	VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getEquilateralTriangle());
	*vertices = verticesList;
}
