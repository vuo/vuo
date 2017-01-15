/**
 * @file
 * vuo.scene.make.tube node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMeshUtility.h"
#include <OpenGL/CGLMacro.h>
#include "VuoList_VuoSceneObject.h"

VuoModuleMetadata({
					"title" : "Make Tube",
					"keywords" : [ "3D", "pipe", "cylinder", "capsule", "barrel", "tubular" ],
					"version" : "1.0.0",
					"genericTypes" : {
						"VuoGenericType1" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						},
						"VuoGenericType2" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						},
						"VuoGenericType3" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						},
						"VuoGenericType4" : {
							"compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						}
					},
					"dependencies": [
						"VuoMeshUtility"
					],
					"node": {
						"exampleCompositions" : [ "MoveBeadsOnString.vuo" ]
					}
				 });

static const double PI = 3.14159265359;

#define WINDING_CW 0
#define WINDING_CCW 1

/**
 * Create an array of points circling around 0,0,0.
 */
static VuoPoint4d* makeCircleTemplate(const unsigned int columns, const float radius)
{
	VuoPoint4d* points = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * columns);

	for(int i = 0; i < columns; i++)
	{
		points[i] = VuoPoint4d_make(
			radius * cos( ( i / (float)columns ) * (2 * PI) ),
			0,
			radius * sin( ( i / (float)columns ) * (2 * PI) ),
			1 );
	}

	return points;
}

/**
 * Generates a submesh tube with @columns columns and @rows rows.
 */
static VuoSubmesh makePipeShell(	const unsigned int columns,
							const unsigned int rows,
							const float radius,
							const float height,
							const unsigned int winding,
							const int culling)
{
	VuoPoint4d* circle = makeCircleTemplate(columns, radius);

	// wrap shell
	unsigned int vertexCount = (columns+1) * (rows+1);
	unsigned int elementCount = 6 * columns * rows;

	VuoPoint4d* vertices = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* triangles = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	VuoPoint4d p = VuoPoint4d_make(0,0,0,0), t = VuoPoint4d_make(0,0,0,0);
	unsigned int n = 0;

	float textureWidth = 1;//radius/uvRadius;
	float textureHeight = 1.;//(1 / (2 * PI * uvRadius));

	for(int y = 0; y < rows+1; y++)
	{
		t.x = 0;
		t.y = y/(float)rows * textureHeight;

		for(int x = 0; x < columns; x++)
		{
			p = circle[x];
			p.y = (y/(float)rows) * height - height / (float)2;
			t.x = textureWidth - ((x / (float)columns) * textureWidth);

			textures[n] = t;
			vertices[n++] = p;
		}

		// add a vert at the vertical seam
		p = circle[0];
		p.y = (y/(float)rows) * height - height / (float)2;

		t.x = 0;//textureWidth;
		textures[n] = t;
		vertices[n++] = p;
	}

	unsigned int step = columns+1;

	unsigned int index = 0;
	for(n = 0; n < elementCount; n+=6)
	{
		switch( winding )
		{
			case WINDING_CCW:
				{
					triangles[n+0] = index+step;
					triangles[n+1] = index;
					triangles[n+2] = index+1;

					triangles[n+3] = index+1;
					triangles[n+4] = index+step+1;
					triangles[n+5] = index+step;
				}
				break;

			default:
				{
					triangles[n+2] = index+step;
					triangles[n+1] = index+0;
					triangles[n+0] = index+1;

					triangles[n+5] = index+1;
					triangles[n+4] = index+step+1;
					triangles[n+3] = index+step;
				}
				break;
		}

		index += 1;
		if( (index+1) % step == 0 ) index++;	// don't use seam vertices as indices
	}

	free(circle);

	VuoSubmesh submesh;
	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.normals = NULL;//normals;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.faceCullingMode = culling;
	submesh.elementCount = elementCount;
	submesh.elements = triangles;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;

	VuoMeshUtility_calculateNormals(&submesh);

	// Average the normals along seams
	VuoPoint4d* normals = submesh.normals;

	for(unsigned int y = 0; y < rows+1; y++)
	{
		unsigned int i0 = y * step;
		unsigned int i1 = y * step + columns;

		VuoPoint4d avg = VuoPoint4d_multiply(VuoPoint4d_add(normals[i0], normals[i1]), .5);

		normals[i0] = avg;
		normals[i1] = avg;
	}

	VuoMeshUtility_calculateTangents(&submesh);

	return submesh;
}

/**
 * Builds a submesh for a pipe cap.  If tube has no inner shell, use makeCylinderCap instead.
 */
static VuoSubmesh makePipeCap(	const unsigned int columns,
						const float radius,
						const float height,
						const float thickness,
						const unsigned int winding)
{
	unsigned int vertexCount = columns * 2;
	unsigned int elementCount = 6 * columns;

	VuoPoint4d* vertices = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* triangles = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	// build vertices
	VuoPoint4d* outer = makeCircleTemplate(columns, radius);
	VuoPoint4d* inner = makeCircleTemplate(columns, radius-thickness);

	memcpy(vertices, outer, sizeof(VuoPoint4d) * columns);
	memcpy(&vertices[columns], inner, sizeof(VuoPoint4d) * columns);

	float uvMax = 1;//(1 / (float)(2 * PI * radius));
	float diameter = radius * 2;

	for(int i = 0; i < vertexCount; i++)
	{
		vertices[i].y = height;
		textures[i] = VuoPoint4d_make( ((vertices[i].x + radius)/diameter) * uvMax, ((vertices[i].z + radius)/diameter) * uvMax, 0, 1);
	}

	free(outer);
	free(inner);

	unsigned int step = columns, index = 0;

	for(unsigned int n = 0; n < elementCount; n+=6)
	{
		unsigned int t0 = index + 0;
		unsigned int t1 = index < columns-1 ? index + 1 : 0;
		unsigned int t2 = index + step + 0;
		unsigned int t3 = index < columns-1 ? index + + step + 1 : step;

		switch( winding )
		{
			case WINDING_CCW:
				{
					triangles[n+0] = t2;
					triangles[n+1] = t0;
					triangles[n+2] = t1;

					triangles[n+3] = t1;
					triangles[n+4] = t3;
					triangles[n+5] = t2;
				}
				break;

			default:
				{
					triangles[n+2] = t2;
					triangles[n+1] = t0;
					triangles[n+0] = t1;

					triangles[n+5] = t1;
					triangles[n+4] = t3;
					triangles[n+3] = t2;
				}
				break;
		}
		index++;
	}

	VuoSubmesh submesh;
	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.normals = NULL;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.faceCullingMode = GL_BACK;
	submesh.elementCount = elementCount;
	submesh.elements = triangles;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;

	VuoMeshUtility_calculateNormals(&submesh);
	VuoMeshUtility_calculateTangents(&submesh);

	return submesh;
}

/**
 * Generates a cap submesh for a cylinder.
 */
static VuoSubmesh makeCylinderCap(const unsigned int columns, const float radius, const float height, const unsigned int winding)
{
	unsigned int vertexCount = columns + 1;
	unsigned int elementCount = 3 * columns;

	VuoPoint4d* vertices = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* triangles = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	// build vertices
	VuoPoint4d* outer = makeCircleTemplate(columns, radius);
	vertices[0] = VuoPoint4d_make(0, height, 0, 1);
	memcpy(&vertices[1], outer, sizeof(VuoPoint4d) * columns);

	float uvMax = 1;//(1 / (float)(2 * PI * radius));
	float diameter = radius * 2;

	for(int i = 0; i < vertexCount; i++)
	{
		vertices[i].y = height;
		textures[i] = VuoPoint4d_make( ((vertices[i].x + radius)/diameter) * uvMax, ((vertices[i].z + radius)/diameter) * uvMax, 0, 1);
	}

	free(outer);

	for(unsigned int n = 0, index = 1; n < elementCount; n+=3, index++)
	{
		switch(winding)
		{
			case WINDING_CCW:
				triangles[n+0] = 0;
				triangles[n+1] = index;
				triangles[n+2] = index < columns ? index+1 : 1;
				break;

			default:
				triangles[n+2] = 0;
				triangles[n+1] = index;
				triangles[n+0] = index < columns ? index+1 : 1;
				break;
		}
	}

	VuoSubmesh submesh;
	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.normals = NULL;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.faceCullingMode = GL_BACK;
	submesh.elementCount = elementCount;
	submesh.elements = triangles;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;

	VuoMeshUtility_calculateNormals(&submesh);
	VuoMeshUtility_calculateTangents(&submesh);

	return submesh;
}

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) outsideMaterial,
		VuoInputData(VuoGenericType2, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) insideMaterial,
		VuoInputData(VuoGenericType3, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) topMaterial,
		VuoInputData(VuoGenericType4, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) bottomMaterial,
		VuoInputData(VuoInteger, {"default":16, "suggestedMin":1, "suggestedMax":128}) rows,
		VuoInputData(VuoInteger, {"default":32, "suggestedMin":3, "suggestedMax":256}) columns,
		VuoInputData(VuoReal, {"default":0.3, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) thickness,
		VuoOutputData(VuoSceneObject) object
)
{
	float radius = .5, height = 1;

	// Make sure inputs are sane
	float thickness2 = fmin(1, fmax(0, thickness)) * radius;
	int rows2 = MIN(512, MAX(1, rows));
	int columns2 = MIN(512, MAX(3, columns));

	VuoSubmesh outer, inner, topCap, bottomCap;

	unsigned int submeshCount = 1;
	unsigned int curSubmesh = 0;

	if( VuoReal_areEqual(thickness2, 0) )
		submeshCount = 1;
	else if( VuoReal_areEqual(thickness2, radius) )
		submeshCount = 3;
	else
		submeshCount = 4;

	VuoList_VuoSceneObject meshes = VuoListCreate_VuoSceneObject();

	// generate the outer shell
	outer = makePipeShell(columns2, rows2, radius, height, WINDING_CW, VuoReal_areEqual(thickness2, 0) ? GL_NONE : GL_BACK);
	VuoListAppendValue_VuoSceneObject(meshes,	VuoSceneObject_make(VuoMesh_makeFromSingleSubmesh(outer),
												VuoShader_make_VuoGenericType1(outsideMaterial),
												VuoTransform_makeIdentity(),
												NULL ));

	// if thickness isn't 0 or 1, generate an inner surface (wound counterclockwise)
	if( !VuoReal_areEqual(thickness2, 0) && !VuoReal_areEqual(thickness2, radius) )
	{
		inner = makePipeShell(columns2, rows2, radius-thickness2, height, WINDING_CCW, GL_BACK);
		VuoListAppendValue_VuoSceneObject(meshes,	VuoSceneObject_make(VuoMesh_makeFromSingleSubmesh(inner),
																		VuoShader_make_VuoGenericType2(insideMaterial),
																		VuoTransform_makeIdentity(),
																		NULL) );
	}

	// generate caps (if thickness is > 0)
	if( !VuoReal_areEqual(thickness2, 0) )
	{
		// if no inner surface, just add a single vertex in the center.
		if( VuoReal_areEqual(thickness2, radius) )
		{
			topCap = makeCylinderCap(columns2, radius, height/(float)2, WINDING_CW);
			bottomCap = makeCylinderCap(columns2, radius, -height/(float)2, WINDING_CCW);
		}
		else
		{
			topCap = makePipeCap(columns2, radius, height/(float)2, thickness2, WINDING_CW);
			bottomCap = makePipeCap(columns2, radius, -height/(float)2, thickness2, WINDING_CCW);
		}

		VuoListAppendValue_VuoSceneObject(meshes,	VuoSceneObject_make(VuoMesh_makeFromSingleSubmesh(topCap),
													VuoShader_make_VuoGenericType3(topMaterial),
													VuoTransform_makeIdentity(),
													NULL ));

		VuoListAppendValue_VuoSceneObject(meshes,	VuoSceneObject_make(VuoMesh_makeFromSingleSubmesh(bottomCap),
													VuoShader_make_VuoGenericType4(bottomMaterial),
													VuoTransform_makeIdentity(),
													NULL ));
	}

	*object = VuoSceneObject_make(NULL, NULL, transform, meshes);
}
