/**
 * @file
 * vuo.vertices.make.square node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Square Vertices",
					 "description" :
						 "<p>Creates 4 vertices in the shape of a square.</p> \
						 <p>The square is on the XY plane, has a width and height of 1, and is centered at (0,0,0).</p>",
					 "keywords" : [ "quad", "rectangle", "plane", "4-gon", "4gon", "shape", "billboard", "sprite" ],
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
	VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
	*vertices = verticesList;
}
