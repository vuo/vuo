/**
 * @file
 * vuo.vertices.make.lines node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Line Mesh",
					  "keywords" : [ "segments", "points" ],
					  "version" : "2.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) positions,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedStep":0.001}) lineWidth,
		VuoOutputData(VuoMesh) mesh
)
{
	if (VuoListGetCount_VuoGenericType1(positions) < 2)
	{
		*mesh = NULL;
		return;
	}

	*mesh = VuoMesh_make_VuoGenericType1(positions, VuoMesh_IndividualLines, lineWidth);

	// Round down to an even number of vertices, since each line requires a pair of vertices.
	(*mesh)->submeshes[0].elementCount &= ~(unsigned int)1;
}
