/**
 * @file
 * vuo.scene.make.square node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoMeshUtility.h"

VuoModuleMetadata({
					"title" : "Make 3D Square",
					"keywords" : [ "heightmap", "plane", "subdivision", "rectangle", "grid", "shape" ],
					"version" : "1.1.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					"dependencies": [
						"VuoMeshUtility"
					],
					"node": {
						"exampleCompositions" : [ "PinchRectangles.vuo", "RewindCheckerboardExplosion.vuo" ]
					}
				 });

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
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
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) columns,
		VuoOutputData(VuoSceneObject) object
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	float w = 1, h = 1;	// mesh width / height
	float half_w = w/2, half_h = h/2;

	int rows2 = MIN(512, MAX(2, rows));
	int columns2 = MIN(512, MAX(2, columns));

	unsigned int vertexCount = rows2 * columns2;
	unsigned int elementCount = (columns2-1) * (rows2-1) * 6;

	float *positions, *normals, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, &normals, &textureCoordinates, NULL, elementCount, &elements);

	unsigned int i = 0;
	for(int y = 0; y < rows2; y++)
	{
		float yp = ((y/(float)(rows2-1)) * h) - half_h;

		for(int x = 0; x < columns2; x++)
		{
			positions[i * 3    ] = ((x / (float)(columns2 - 1)) * w) -  half_w;
			positions[i * 3 + 1] = yp;
			positions[i * 3 + 2] = 0;
			normals[i * 3    ] = 0;
			normals[i * 3 + 1] = 0;
			normals[i * 3 + 2] = 1;
			textureCoordinates[i * 2    ] = x / (float)(columns2 - 1);
			textureCoordinates[i * 2 + 1] = y / (float)(rows2 - 1);
			++i;
		}
	}

	unsigned int n = 0;

	for(int y = 0; y < (rows2-1); y++)
	{
		for(int x = 0; x < (columns2-1); x++)
		{
			unsigned int a = (y*columns2) + x+0;
			unsigned int b = (y*columns2) + x+1;
			unsigned int c = ((y+1)*columns2) + x+0;
			unsigned int d = ((y+1)*columns2) + x+1;

			elements[n++] = c;
			elements[n++] = a;
			elements[n++] = b;

			elements[n++] = b;
			elements[n++] = d;
			elements[n++] = c;
		}
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, normals, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), transform);
	VuoSceneObject_setName(*object, VuoText_make("Square"));

	(*context)->rows = rows;
	(*context)->columns = columns;
}
