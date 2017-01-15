/**
 * @file
 * vuo.scene.make.icosphere node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include <OpenGL/CGLMacro.h>
#include "VuoMeshParametric.h"
#include "VuoMeshUtility.h"

VuoModuleMetadata({
					  "title" : "Make Icosphere",
					  "keywords" : [ "mesh", "3d", "scene", "sphere", "soccer", "shape", "ball", "futbol", "football" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoShader", "VuoColor", "VuoImage" ]
						  }
					  },
					  "dependencies" : [ "VuoMeshParametric", "VuoMeshUtility" ],
					  "node" : {
						  "exampleCompositions" : []
					  }
				  });

static const float PHI = 1.618033988749895;
/// Constant providing the ratio of a circle's circumference to its diameter
static const float PI = 3.14159265359;
static const float ICO_RADIUS = .5f;

#define IS_POLE(v) (fabs(v.y) > .9999)

static inline VuoPoint4d VuoPoint4d_multiply3d(VuoPoint4d a, float b)
{
	VuoPoint4d v =
	{
		a.x * b,
		a.y * b,
		a.z * b,
		a.w
	};

	return v;
}

/**
 * Subdivides a set of vertices (wound as individual triangles) on an icosphere.
 *
 *	 /\			 /\
 * 	/  \	-> 	/--\
 * /____\	   /_\/_\
 *
 * Returns an array 4* larger than the provided.
 */
static VuoPoint4d* SubdivideIcosahedron(VuoPoint4d* vertices, unsigned int vertexCount)
{
	VuoPoint4d* v = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount * 4);

	unsigned int index = 0;

	VuoPoint4d 	p0,	//	    5
				p1,	//    3   4
				p2,	//	0,  1,  2
				p3,
				p4,
				p5;

	for(int i = 0; i < vertexCount; i+=3)
	{
		p0 = vertices[i+0];
		p2 = vertices[i+1];
		p5 = vertices[i+2];

		p1 = VuoPoint4d_normalize3d(VuoPoint4d_multiply(VuoPoint4d_add(p0, p2), .5f));
		p3 = VuoPoint4d_normalize3d(VuoPoint4d_multiply(VuoPoint4d_add(p0, p5), .5f));
		p4 = VuoPoint4d_normalize3d(VuoPoint4d_multiply(VuoPoint4d_add(p2, p5), .5f));

		v[index++] = p0;
		v[index++] = p1;
		v[index++] = p3;

		v[index++] = p1;
		v[index++] = p2;
		v[index++] = p4;

		v[index++] = p1;
		v[index++] = p4;
		v[index++] = p3;

		v[index++] = p3;
		v[index++] = p4;
		v[index++] = p5;
	}

	return v;
}

static inline VuoPoint4d cross2d(VuoPoint4d a, VuoPoint4d b, VuoPoint4d c)
{
	a.z = 1;
	b.z = 1;
	c.z = 1;

	VuoPoint4d a0 = VuoPoint4d_make(b.x - a.x, b.y - a.y, 1, 1);
	VuoPoint4d a1 = VuoPoint4d_make(c.x - a.x, c.y - a.y, 1, 1);

	return VuoPoint4d_crossProduct(a0, a1);
}

void nodeEvent
(
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoInteger, {"default":3,"suggestedMin":0, "suggestedMax":7}) subdivisions,
	VuoOutputData(VuoSceneObject) object
)
{
	unsigned int i = 0;

	// Template
	VuoPoint4d ico[12] =
	{
		VuoPoint4d_normalize3d(VuoPoint4d_make(-1,  PHI,  0, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 1,  PHI,  0, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make(-1, -PHI,  0, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 1, -PHI,  0, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 0, -1,  PHI, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 0,  1,  PHI, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 0, -1, -PHI, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( 0,  1, -PHI, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make(  PHI, 0, -1, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make(  PHI, 0,  1, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( -PHI, 0, -1, 1)),
		VuoPoint4d_normalize3d(VuoPoint4d_make( -PHI, 0,  1, 1))
	};

	unsigned int tri[60] =
	{
		0, 11, 5,
		0, 5, 1,
		0, 1, 7,
		0, 7, 10,
		0, 10, 11,

		1, 5, 9,
		5, 11, 4,
		11, 10, 2,
		10, 7, 6,
		7, 1, 8,

		3, 9, 4,
		3, 4, 2,
		3, 2, 6,
		3, 6, 8,
		3, 8, 9,

		4, 9, 5,
		2, 4, 11,
		6, 2, 10,
		8, 6, 7,
		9, 8, 1
	};

	unsigned int vertexCount = 60;
	VuoPoint4d* vertices = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	for(i = 0; i < vertexCount; i++)
		vertices[i] = ico[tri[i]];

	unsigned int sub = MIN(MAX(0, subdivisions), 7);

	for(i = 0; i < sub; i++)
	{
		VuoPoint4d* v = SubdivideIcosahedron(vertices, vertexCount);
		vertexCount *= 4;
		free(vertices);
		vertices = v;
	}

	VuoPoint4d* normals = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* textures = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	for(int i = 0; i < vertexCount; ++i)
	{
		normals[i] = vertices[i];
		normals[i].w = 1;
		textures[i] = (VuoPoint4d)
		{
			1 - (.5 + (atan2(normals[i].z, normals[i].x) / (2 * PI))),
			1. - (.5 - (asin(normals[i].y) / PI)),
			0.,
			1.
		};
		vertices[i] = VuoPoint4d_multiply3d(vertices[i],  ICO_RADIUS);
	}

	unsigned int* elements = (unsigned int*)malloc(sizeof(unsigned int) * vertexCount);

	for(i = 0; i < vertexCount; i+=3)
	{
		elements[i+0] = i+0;
		elements[i+1] = i+1;
		elements[i+2] = i+2;
	}

	// build the mesh!
	VuoSubmesh submesh;

	submesh.vertexCount = vertexCount;
	submesh.positions = vertices;
	submesh.elements = elements;
	submesh.normals = normals;
	submesh.tangents = NULL;
	submesh.bitangents = NULL;
	submesh.textureCoordinates = textures;
	submesh.elementCount = vertexCount;
	submesh.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	submesh.faceCullingMode = GL_BACK;

	VuoPoint2d min, max;

	for(int i = 0; i < submesh.elementCount; i+=3)
	{
		int a = elements[i+0],
			b = elements[i+1],
			c = elements[i+2];

		VuoPoint4d cross = cross2d(textures[a], textures[b], textures[c]);

		if(cross.z < 0)
		{
			if(textures[a].x < .2) textures[a].x += 1;
			if(textures[b].x < .2) textures[b].x += 1;
			if(textures[c].x < .2) textures[c].x += 1;
		}

		if( IS_POLE(normals[a]) )
			textures[a].x = (textures[b].x + textures[c].x) * .5;
		else
		if( IS_POLE(normals[b]) )
			textures[b].x = (textures[a].x + textures[c].x) * .5;
		else
		if( IS_POLE(normals[c]) )
			textures[c].x = (textures[a].x + textures[b].x) * .5;

		if(i == 0)
		{
			min = (VuoPoint2d) { textures[i].x, textures[i].y };
			max = (VuoPoint2d) { textures[i].x, textures[i].y };
		}

		min.x = fmin(min.x, textures[i].x);
		min.y = fmin(min.y, textures[i].y);

		max.x = fmax(max.x, textures[i].x);
		max.y = fmax(max.y, textures[i].y);
	}

	VuoPoint2d scale = { 1 / (max.x - min.x), 1 / (max.y - min.y) };

	for(int i = 0; i < submesh.vertexCount; i++)
	{
		textures[i].x -= min.x;
		textures[i].y -= min.y;
		textures[i].x *= scale.x;
		textures[i].y *= scale.y;
	}


	submesh.textureCoordinates = textures;
	VuoMeshUtility_calculateTangents(&submesh);

	VuoMesh mesh = VuoMesh_makeFromSingleSubmesh(submesh);

	// Rotate so the seam is in the back (just like the lat/lon sphere).
	VuoTransform t2 = VuoTransform_composite(VuoTransform_makeEuler((VuoPoint3d){0,0,0}, (VuoPoint3d){0,-PI/2,0}, (VuoPoint3d){1,1,1}), transform);

	*object = VuoSceneObject_make(mesh, VuoShader_make_VuoGenericType1(material), t2, NULL);
}
