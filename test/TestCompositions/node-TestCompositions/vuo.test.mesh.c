/**
 * @file
 * vuo.test.mesh node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Test Mesh Types",
	"description" : "",
	"version" : "1.0.0",
});

void nodeEvent(
	VuoInputData(VuoInteger) elementAssemblyMethod,
	VuoInputData(VuoBoolean) includeElements,
	VuoInputData(VuoBoolean) includeNTBTC,
	VuoOutputData(VuoSceneObject) object)
{
	unsigned int vertexCount = 10;
	if (elementAssemblyMethod == VuoMesh_IndividualTriangles
		|| elementAssemblyMethod == VuoMesh_TriangleStrip
		|| elementAssemblyMethod == VuoMesh_TriangleFan)
		vertexCount *= 3;
	else if (elementAssemblyMethod == VuoMesh_IndividualLines
		|| elementAssemblyMethod == VuoMesh_LineStrip)
		vertexCount *= 2;

	VuoPoint3d triangleVertices[3] = {
		{0 ,0,0},
		{1 ,0,0},
		{.5,1,0},
	};

	unsigned int elementCount = 0;
	if (includeElements)
		elementCount = vertexCount * 2;

	float *vertices, *normals, *textureCoordinates, *colors;
	unsigned int *elements = NULL;
	VuoMesh_allocateCPUBuffers(vertexCount, &vertices, &normals, &textureCoordinates, &colors, elementCount, &elements);

	for (unsigned int i = 0; i < vertexCount; ++i)
	{
		VuoPoint3d_setArray(&vertices[i * 3],           triangleVertices[i % 3]);
		VuoPoint3d_setArray(&normals[i * 3],            (VuoPoint3d){0, 0, 1});
		VuoPoint2d_setArray(&textureCoordinates[i * 2], triangleVertices[i % 3].xy);
		VuoPoint4d_setArray(&colors[i * 4],             (VuoPoint4d){1, 1, 1, 1});
	}

	if (includeElements)
		for (unsigned int i = 0; i < elementCount; ++i)
			elements[i] = i % vertexCount;

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		vertices, includeNTBTC ? normals : NULL, includeNTBTC ? textureCoordinates : NULL, includeNTBTC ? colors : NULL,
		elementCount, elements, elementAssemblyMethod);
	VuoMesh_setPrimitiveSize(mesh, 0.1);
	*object = VuoSceneObject_makeMesh(mesh, NULL, VuoTransform_makeIdentity());
}
