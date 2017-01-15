/**
 * @file
 * vuo.scene.make.grid.lines node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoGridType.h"

VuoModuleMetadata({
					"title" : "Make Grid Lines Object",
					"keywords" : [ "heightmap", "plane", "subdivision", "square", "rectangle" ],
					"version" : "1.0.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					"node": {
						"exampleCompositions" : [ "MakeRuggedTerrain.vuo", "PinchRectangles.vuo" ]
					}
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":2, "suggestedMax":256}) columns,
		VuoInputData(VuoGridType, {"default":"horizontal-vertical"}) gridType,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedMax":0.5}) lineWidth,
		VuoOutputData(VuoSceneObject) object
)
{
	float w = 1, h = 1;	// mesh width / height
	float half_w = w/2, half_h = h/2;

	int rows2 = MIN(512, MAX(2, rows));
	int columns2 = MIN(512, MAX(2, columns));

	unsigned int vertexCount = rows2 * columns2;

	VuoPoint4d* vertices = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* normals = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* tangents = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* bitangents = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	unsigned int i = 0;
	for(int y = 0; y < rows2; y++)
	{
		float yp = ((y/(float)(rows2-1)) * h) - half_h;

		for(int x = 0; x < columns2; x++)
		{
			vertices[i] = VuoPoint4d_make( ((x/(float)(columns2-1)) * w) -  half_w, yp, 0, 1);
			normals[i] = VuoPoint4d_make( 0, 0, 1, 1);
			tangents[i] = VuoPoint4d_make( 1, 0, 0, 1);
			bitangents[i] = VuoPoint4d_make( 0, 1, 0, 1);
			textures[i++] = VuoPoint4d_make( x/(float)(columns2-1), y/(float)(columns2-1), 0, 1);
		}
	}

	unsigned int horizontalLineCount = ((columns2-1) * 2) * rows2;
	unsigned int verticalLineCount = ((rows2-1) * 2) * columns2;

	unsigned int elementCount = gridType == VuoGridType_HorizontalAndVertical ? horizontalLineCount + verticalLineCount :
								(gridType == VuoGridType_Horizontal ? horizontalLineCount : verticalLineCount);

	unsigned int* elements = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	unsigned int n = 0;

	if(gridType == VuoGridType_Horizontal || gridType == VuoGridType_HorizontalAndVertical)
	{
		for(int y = 0; y < rows2; y++)
		{
			for(int x = 0; x < (columns2-1); x++)
			{
				elements[n++] = (y*columns2) + x;
				elements[n++] = (y*columns2) + x+1;
			}
		}
	}

	if(gridType == VuoGridType_Vertical || gridType == VuoGridType_HorizontalAndVertical)
	{
		unsigned int step = columns2;

		for(int y = 0; y < (rows2-1); y++)
		{
			for(int x = 0; x < columns2; x++)
			{
				elements[n++] = (y*columns2) + x;
				elements[n++] = (y*columns2) + x+step;
			}
		}
	}

	// build the mesh!
	VuoSubmesh submesh;

	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.normals = normals;
	submesh.tangents = tangents;
	submesh.bitangents = bitangents;
	submesh.textureCoordinates = textures;
	submesh.faceCullingMode = GL_NONE;
	submesh.elementCount = elementCount;
	submesh.primitiveSize = lineWidth;
	submesh.elements = elements;
	submesh.elementAssemblyMethod = VuoMesh_IndividualLines;

	VuoMesh mesh = VuoMesh_makeFromSingleSubmesh(submesh);

	VuoShader shader = VuoShader_make_VuoGenericType1(material);
	*object = VuoSceneObject_make(mesh, shader, transform, NULL);
}
