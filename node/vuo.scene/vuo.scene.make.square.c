/**
 * @file
 * vuo.scene.make.square node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>	// GL_NONE
#include "VuoMeshUtility.h"

VuoModuleMetadata({
					"title" : "Make 3D Square",
					"keywords" : [ "heightmap", "plane", "subdivision", "rectangle", "grid", "shape" ],
					"version" : "1.0.1",
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

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) rows,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":2, "suggestedMax":256}) columns,
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
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	unsigned int i = 0;
	for(int y = 0; y < rows2; y++)
	{
		float yp = ((y/(float)(rows2-1)) * h) - half_h;

		for(int x = 0; x < columns2; x++)
		{
			vertices[i] = VuoPoint4d_make( ((x/(float)(columns2-1)) * w) -  half_w, yp, 0, 1);
			normals[i] = VuoPoint4d_make( 0, 0, 1, 1);
			textures[i++] = VuoPoint4d_make( x/(float)(columns2-1), y/(float)(rows2-1), 0, 1);
		}
	}

	unsigned int elementCount = (columns2-1) * (rows2-1) * 6;
	unsigned int* elements = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
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

	VuoSubmesh submesh = VuoSubmesh_makeFromBuffers(vertexCount,
													vertices, normals, NULL, NULL, textures,
													elementCount, elements, VuoMesh_IndividualTriangles);

	VuoMeshUtility_calculateTangents(&submesh);

	VuoMesh mesh = VuoMesh_makeFromSingleSubmesh(submesh);

	*object = VuoSceneObject_make(mesh, VuoShader_make_VuoGenericType1(material), transform, NULL);
	object->name = VuoText_make("Square");
}
