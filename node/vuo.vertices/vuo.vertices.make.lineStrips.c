/**
 * @file
 * vuo.vertices.make.lineStrips node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMakeVerticesFromPositions.h"

VuoModuleMetadata({
					  "title" : "Make Line Strip Vertices",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "isInterface" : false,
						  "exampleCompositions" : [ "BounceLineStrip.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) positions,
		VuoOutputData(VuoVertices) vertices
)
{
	*vertices = VuoMakeVerticesFromPositions_VuoGenericType1(positions, VuoVertices_LineStrip);
}
