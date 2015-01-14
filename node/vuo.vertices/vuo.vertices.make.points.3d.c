/**
 * @file
 * vuo.vertices.make.points.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Point Vertices",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint3d) positions,
		VuoOutputData(VuoVertices) vertices
)
{
	*vertices = VuoVertices_makeFrom3dPoints(positions, VuoVertices_Points);
}
