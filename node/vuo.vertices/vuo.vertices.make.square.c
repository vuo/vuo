/**
 * @file
 * vuo.vertices.make.square node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Square Vertices",
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
