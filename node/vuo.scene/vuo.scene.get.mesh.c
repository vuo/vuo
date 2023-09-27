/**
 * @file
 * vuo.scene.get.mesh node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObject.h"

VuoModuleMetadata({
	"title" : "Get Mesh Values",
	"keywords" : [
		"information",
		"model", "object", "3D", "opengl", "scenegraph", "graphics",
		"vertices", "points", "surface",
	],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions" : [ "DisplayBlobWithNormals.vuo" ]
	}
});

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoList_VuoPoint3d) positions,
		VuoOutputData(VuoList_VuoPoint3d) normals
)
{
	VuoSceneObject flattenedObject = VuoSceneObject_flatten(object);
	VuoLocal(flattenedObject);

	VuoMesh mesh = VuoSceneObject_getMesh(flattenedObject);
	if (!mesh)
	{
		*positions = VuoListCreate_VuoPoint3d();
		*normals = VuoListCreate_VuoPoint3d();
		return;
	}

	unsigned int vertexCount, elementCount, *elements;
	float *positionsBuffer, *normalsBuffer;
	VuoMesh_getCPUBuffers(mesh, &vertexCount, &positionsBuffer, &normalsBuffer, NULL, NULL, &elementCount, &elements);

	unsigned int expandedVertexCount = MAX(vertexCount, elementCount);
	*positions = VuoListCreateWithCount_VuoPoint3d(expandedVertexCount, VuoPoint3d_make(0, 0, 0));
	*normals = VuoListCreateWithCount_VuoPoint3d(expandedVertexCount, VuoPoint3d_make(0, 0, 0));
	VuoPoint3d *positionsListData = VuoListGetData_VuoPoint3d(*positions);
	VuoPoint3d *normalsListData = VuoListGetData_VuoPoint3d(*normals);

	if (elementCount > 0)
	{
		for (unsigned int i = 0; i < elementCount; ++i)
		{
			positionsListData[i] = VuoPoint3d_makeFromArray(&positionsBuffer[3*elements[i]]);
			normalsListData[i] = VuoPoint3d_makeFromArray(&normalsBuffer[3*elements[i]]);
		}
	}
	else
	{
		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			positionsListData[i] = VuoPoint3d_makeFromArray(&positionsBuffer[3*i]);
			normalsListData[i] = VuoPoint3d_makeFromArray(&normalsBuffer[3*i]);
		}
	}

	for (unsigned int i = 0; i < expandedVertexCount; ++i)
		normalsListData[i] = VuoPoint3d_normalize(normalsListData[i]);
}
