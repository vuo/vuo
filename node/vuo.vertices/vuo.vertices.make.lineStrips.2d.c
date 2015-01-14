/**
 * @file
 * vuo.vertices.make.lineStrips.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Line Strip Vertices",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "BounceLineStrip.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint2d) positions,
		VuoOutputData(VuoVertices) vertices
)
{
	*vertices = VuoVertices_makeFrom2dPoints(positions, VuoVertices_LineStrip);
}
