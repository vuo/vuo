/**
 * @file
 * vuo.scene.make.grid.lines node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoGridType.h"

VuoModuleMetadata({
					"title" : "Make Grid Lines Object",
					"keywords" : [ "heightmap", "plane", "subdivision", "square", "rectangle", "shape" ],
					"version" : "1.1.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					"node": {
						"exampleCompositions" : [ "MakeRuggedTerrain.vuo", "PinchRectangles.vuo" ]
					}
				 });

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
	VuoGridType gridType;
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
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":2, "suggestedMax":256}) columns,
		VuoInputData(VuoGridType, {"default":"horizontal-vertical"}) gridType,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedMax":0.5}) lineWidth,
		VuoOutputData(VuoSceneObject) object
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns
	 && gridType == (*context)->gridType)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		VuoMesh m = VuoMesh_copyShallow(VuoSceneObject_getMesh(*object));
		VuoMesh_setPrimitiveSize(m, lineWidth);
		VuoSceneObject_setMesh(*object, m);
		return;
	}

	float w = 1, h = 1;	// mesh width / height
	float half_w = w/2, half_h = h/2;

	int rows2 = MIN(512, MAX(2, rows));
	int columns2 = MIN(512, MAX(2, columns));

	unsigned int vertexCount = rows2 * columns2;

	float *vertices           = (float *)malloc(sizeof(float) * 3 * vertexCount);
	float *normals            = (float *)malloc(sizeof(float) * 3 * vertexCount);
	float *textureCoordinates = (float *)malloc(sizeof(float) * 2 * vertexCount);

	unsigned int i = 0;
	for(int y = 0; y < rows2; y++)
	{
		float yp = ((y/(float)(rows2-1)) * h) - half_h;

		for(int x = 0; x < columns2; x++)
		{
			vertices[i * 3    ] = ((x / (float)(columns2 - 1)) * w) -  half_w;
			vertices[i * 3 + 1] = yp;
			vertices[i * 3 + 2] = 0;
			normals[i * 3    ] = 0;
			normals[i * 3 + 1] = 0;
			normals[i * 3 + 2] = 1;
			textureCoordinates[i * 2    ] = x / (float)(columns2 - 1);
			textureCoordinates[i * 2 + 1] = y / (float)(rows2 - 1);
			++i;
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

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		vertices, normals, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualLines);
	VuoMesh_setFaceCulling(mesh, VuoMesh_CullNone);
	VuoMesh_setPrimitiveSize(mesh, lineWidth);

	VuoShader shader = VuoShader_make_VuoGenericType1(material);
	*object = VuoSceneObject_makeMesh(mesh, shader, transform);

	(*context)->rows = rows;
	(*context)->columns = columns;
	(*context)->gridType = gridType;
}
