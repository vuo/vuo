/**
 * @file
 * vuo.vertices.make.lines node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Line Mesh",
					  "keywords" : [ "segments", "points" ],
					  "version" : "2.2.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	VuoList_VuoGenericType1 positions;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoGenericType1) positions,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedStep":0.001}) lineWidth,
		VuoOutputData(VuoMesh) mesh
)
{
	// If the list hasn't changed, just reuse the existing GPU mesh data.
	if (positions
	 && positions == (*context)->positions)
	{
		// Copy the CPU mesh structure, so we don't modify the mesh we already output in previous invocations.
		*mesh = VuoMesh_copyShallow(*mesh);
		VuoMesh_setPrimitiveSize(*mesh, lineWidth);
		return;
	}

	if (VuoListGetCount_VuoGenericType1(positions) < 2)
	{
		*mesh = NULL;
		return;
	}

	*mesh = VuoMesh_make_VuoGenericType1(positions, NULL, VuoMesh_IndividualLines, lineWidth);

	VuoRetain(positions);
	VuoRelease((*context)->positions);
	(*context)->positions = positions;
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->positions);
}
