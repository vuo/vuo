/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>
#include "VuoMeshUtility.h"

VuoModuleMetadata({
					 "title" : "Make Cone",
					 "keywords" : [ "3D", "point", "hat", "wizard", "pyramid", "shape", "object" ],
					 "version" : "1.1.0",
					 "genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					 "node": {
						  "exampleCompositions" : [ ]
					 },
					 "dependencies": [
						"VuoMeshUtility"
					 ]
				 });


static const double TWOPI = 6.28318530718;

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
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":3, "suggestedMax": 256}) columns,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":2, "suggestedMax": 256}) rows,
		VuoOutputData(VuoSceneObject) cone
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns)
	{
		*cone = VuoSceneObject_copy(*cone);
		VuoSceneObject_setTransform(*cone, transform);
		VuoSceneObject_setShader(*cone, VuoShader_make_VuoGenericType1(material));
		return;
	}

	unsigned int c = MIN(MAX(3, columns), 256) + 1;	// add one for seam (the last vertex is unused)
	unsigned int r = MIN(MAX(2, rows), 256) + 1;	// add one for base
	unsigned int vertexCount = c * r + 1; // add one for base center vertex and one for tip top

	// sides + (bottom + ?top)
	unsigned int elementCount = (c-1)*(r-2)*6 + ((c-1)*3);

	float *positions, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, NULL, elementCount, &elements);

	unsigned int v = 0;

	/**
	 * rows of circles, two on bottom for hard normals.  Top is a circle because otherwise
	 * the normal would be straight upwards and really smooth.
	 */
	for(int n = 0; n < r; n++)
	{
		float per = (MAX(n-1, 0) / (float) (r - 2));
		float y = per - .5;
		// if this is the top, leave a tiny gap so that normals can be calculated properly
		per = n == r - 1 ? (1 - (per * .999)) : (1 - per);

		for(unsigned int i = 0; i < c; i++)
		{
			float x =  (cos((i/(float)(c-1))*TWOPI) * .5) * per;
			float z =  (sin((i/(float)(c-1))*TWOPI) * .5) * per;

			positions[v * 3    ] = x;
			positions[v * 3 + 1] = y;
			positions[v * 3 + 2] = z;
			textureCoordinates[v * 2    ]  = x + .5;
			textureCoordinates[v * 2 + 1]  = n == 0 ? z + .5 : 1. - (z + .5);
			v++;
		}
	}

	// Base center vertex
	positions[(vertexCount - 1) * 3    ] = 0;
	positions[(vertexCount - 1) * 3 + 1] = -.5;
	positions[(vertexCount - 1) * 3 + 2] = 0;
	textureCoordinates[(vertexCount - 1) * 2    ] = .5;
	textureCoordinates[(vertexCount - 1) * 2 + 1] = .5;

	v = 0;

	/**
	 * Wind caps
	 */
	for(unsigned int i = 0; i < c-1; i++)
	{
		// bottom
		elements[v+0] = i;
		elements[v+1] = i+1;
		elements[v+2] = vertexCount-1;

		v += 3;
	}

	/**
	 * Wind sides
	 */
	for(unsigned int i = 1; i < r-1; i++)
	{
		for(unsigned int n = i * c; n < ((i+1) * c) - 1; n++)
		{
			elements[v+0] = n;
			elements[v+1] = n+c;
			elements[v+2] = n+1;

			elements[v+3] = n+1;
			elements[v+4] = n+c;
			elements[v+5] = n+c+1;

			v += 6;
		}
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);

	VuoMeshUtility_calculateNormals(mesh);

	float *normals;
	VuoMesh_getCPUBuffers(mesh, NULL, NULL, &normals, NULL, NULL, NULL, NULL);

	/**
	 * average seams of sides
	 */
	for(int i = 1; i < r; i++)
	{
		unsigned int a = i*c, b = a+c-1;

		VuoPoint3d na = VuoPoint3d_makeFromArray(&normals[a * 3]);
		VuoPoint3d nb = VuoPoint3d_makeFromArray(&normals[b * 3]);
		VuoPoint3d avg = (na + nb) / 2.f;

		VuoPoint3d_setArray(&normals[a * 3], avg);
		VuoPoint3d_setArray(&normals[b * 3], avg);
	}

	// make top vertex point up
	for(int i = 0; i < c; i++)
	{
		normals[((r-1)*c+i) * 3    ] = 0;
		normals[((r-1)*c+i) * 3 + 1] = 1;
		normals[((r-1)*c+i) * 3 + 2] = 0;
	}

	// // Average the seam normals
	// VuoPoint4d avg = VuoPoint4d_multiply(VuoPoint4d_add(submesh.normals[c], submesh.normals[c+c-1]), .5);
	// submesh.normals[c] = avg;
	// submesh.normals[c+c-1] = avg;

	// avg = VuoPoint4d_add(submesh.normals[c+c], submesh.normals[vertexCount-3]);
	// submesh.normals[c+c] = avg;
	// submesh.normals[vertexCount-3] = avg;

	// for(int i = 0; i < c; i++)
	// 	submesh.normals[i] = VuoPoint4d_make(0,-1,0,1);

	*cone = VuoSceneObject_makeMesh(mesh,
									VuoShader_make_VuoGenericType1(material),
									transform);

	VuoSceneObject_setName(*cone, VuoText_make("Cone"));

	(*context)->rows = rows;
	(*context)->columns = columns;
}
