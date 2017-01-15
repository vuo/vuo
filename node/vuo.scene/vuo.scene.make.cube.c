/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Cube with Materials",
					 "keywords" : [ "3D", "box", "d6", "hexahedron", "Platonic", "rectangular", "square" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

static VuoSubmesh makePlane(unsigned int rows, unsigned int columns)
{
	unsigned int vertexCount = rows * columns;
	unsigned int triangleCount = (rows-1) * (columns-1) * 6;

	VuoPoint4d* positions = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* normals = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* uvs = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* tangents = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* bitangents = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* triangles = (unsigned int*)malloc(sizeof(unsigned int) * triangleCount);

	unsigned int index = 0, t_index = 0;

	for(unsigned int i = 0; i < rows; i++)
	{
		float y = (i/(float)(rows-1)) - .5;

		for(unsigned int n = 0; n < columns; n++)
		{
			float x = (n/(float)(columns-1)) - .5;

			positions[index] = VuoPoint4d_make( x, y, 0, 1);
			normals[index] = VuoPoint4d_make(0,0,1,1);
			uvs[index] = VuoPoint4d_make(x+.5, y+.5, 0, 1);
			tangents[index] = VuoPoint4d_make(1, 0, 0, 1);
			bitangents[index] = VuoPoint4d_make(0, 1, 0, 1);

			if(n < columns-1 && i < rows-1)
			{
				triangles[t_index++] = index+columns;
				triangles[t_index++] = index;
				triangles[t_index++] = index+1;

				triangles[t_index++] = index+1;
				triangles[t_index++] = index+columns+1;
				triangles[t_index++] = index+columns;
			}

			index++;
		}
	}

	VuoSubmesh submesh;

	submesh.positions = positions;
	submesh.normals = normals;
	submesh.textureCoordinates = uvs;
	submesh.tangents = tangents;
	submesh.bitangents = bitangents;
	submesh.elements = triangles;
	submesh.elementCount = triangleCount;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	submesh.vertexCount = vertexCount;
	submesh.faceCullingMode = GL_BACK;
	return submesh;
}

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoShader) frontShader,
		VuoInputData(VuoShader) leftShader,
		VuoInputData(VuoShader) rightShader,
		VuoInputData(VuoShader) backShader,
		VuoInputData(VuoShader) topShader,
		VuoInputData(VuoShader) bottomShader,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) columns,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) slices,
		VuoOutputData(VuoSceneObject) cube
)
{
	VuoList_VuoSceneObject cubeChildObjects = VuoListCreate_VuoSceneObject();

	unsigned int _rows = MAX(2, MIN(512, rows));
	unsigned int _columns = MAX(2, MIN(512, columns));
	unsigned int _slices = MAX(2, MIN(512, slices));

	VuoMesh frontBackMesh = VuoMesh_makeFromSingleSubmesh(makePlane(_rows, _columns));
	VuoMesh leftRightMesh = VuoMesh_makeFromSingleSubmesh(makePlane(_rows, _slices));
	VuoMesh topBottomMesh = VuoMesh_makeFromSingleSubmesh(makePlane(_slices, _columns));

	// Front Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					frontBackMesh,
					frontShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					leftRightMesh,
					leftShader,
					VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					leftRightMesh,
					rightShader,
					VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					frontBackMesh,
					backShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					topBottomMesh,
					topShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					topBottomMesh,
					bottomShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	*cube = VuoSceneObject_make(NULL, NULL, transform, cubeChildObjects);
}
