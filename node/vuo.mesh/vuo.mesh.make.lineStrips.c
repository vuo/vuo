/**
 * @file
 * vuo.vertices.make.lineStrips node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Line Strip Mesh",
					  "keywords" : [ "segments", "points" ],
					  "version" : "2.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "BounceLineStrip.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) positions,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedStep":0.001}) lineWidth,
		VuoOutputData(VuoMesh) mesh
)
{
	*mesh = VuoMesh_make_VuoGenericType1(positions, VuoMesh_LineStrip, lineWidth);
}
