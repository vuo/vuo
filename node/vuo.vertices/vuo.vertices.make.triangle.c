/**
 * @file
 * vuo.vertices.make.triangle node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Triangle Vertices",
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
