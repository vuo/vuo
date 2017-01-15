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
#include "VuoMeshUtility.h"

VuoModuleMetadata({
					 "title" : "Make Cone",
					 "keywords" : [ "3D", "point", "hat", "wizard", "pyramid" ],
					 "version" : "1.0.0",
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

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":3, "suggestedMax": 256}) columns,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":2, "suggestedMax": 256}) rows,
		VuoOutputData(VuoSceneObject) cone
)
{
	unsigned int c = MIN(MAX(3, columns), 256) + 1;	// add one for seam (the last vertex is unused)
	unsigned int r = MIN(MAX(2, rows), 256) + 1;	// add one for base
	unsigned int vertexCount = c * r + 1; // add one for base center vertex and one for tip top

	VuoPoint4d* positions = (VuoPoint4d*) malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* textures = (VuoPoint4d*) malloc(sizeof(VuoPoint4d) * vertexCount);

	// sides + (bottom + ?top)
	unsigned int elementCount = (c-1)*(r-2)*6 + ((c-1)*3);
	unsigned int* elements = (unsigned int*) malloc(sizeof(unsigned int) * elementCount );

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

			positions[v] 	= VuoPoint4d_make(x, y, z, 1);
			textures[v] 	= VuoPoint4d_make((x+.5), (z+.5), 0, 1);
			v++;
		}
	}

	// Base center vertex
	positions[vertexCount-1] = VuoPoint4d_make(0,-.5,0,1);
	textures [vertexCount-1] = VuoPoint4d_make(.5,.5,0,1);

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

	VuoSubmesh submesh;
	submesh.vertexCount = vertexCount;
	submesh.positions = positions;
	submesh.normals = NULL;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.elementCount = elementCount;
	submesh.elements = elements;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	submesh.faceCullingMode = GL_BACK;

	VuoMeshUtility_calculateNormals(&submesh);

	/**
	 * average seams of sides
	 */
	for(int i = 1; i < r; i++)
	{
		unsigned int a = i*c, b = a+c-1;

		VuoPoint4d avg = VuoPoint4d_multiply(
							VuoPoint4d_add(	submesh.normals[a],
											submesh.normals[b]
										),
									.5);

		submesh.normals[a] = avg;
		submesh.normals[b] = avg;
	}

	// make top vertex point up
	for(int i = 0; i < c; i++)
	{
		submesh.normals[(r-1)*c+i] = VuoPoint4d_make(0,1,0,1);
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

	VuoMeshUtility_calculateTangents(&submesh);

	*cone = VuoSceneObject_make(	VuoMesh_makeFromSingleSubmesh(submesh),
									VuoShader_make_VuoGenericType1(material),
									transform,
									NULL);

}
