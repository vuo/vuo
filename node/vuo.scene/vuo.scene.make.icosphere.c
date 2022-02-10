/**
 * @file
 * vuo.scene.make.icosphere node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
					  "version" : "1.0.1",
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
static float *subdivideIcosahedron(float *positions, unsigned int vertexCount)
{
	float *p = (float *)malloc(sizeof(float) * 3 * vertexCount * 4);

	unsigned int index = 0;

	VuoPoint3d  p0,  //      5
				p1,  //    3   4
				p2,  //  0   1   2
				p3,
				p4,
				p5;

	for(int i = 0; i < vertexCount; i+=3)
	{
		p0 = VuoPoint3d_makeFromArray(&positions[ i      * 3]);
		p2 = VuoPoint3d_makeFromArray(&positions[(i + 1) * 3]);
		p5 = VuoPoint3d_makeFromArray(&positions[(i + 2) * 3]);

		p1 = VuoPoint3d_normalize(p0 + p2);
		p3 = VuoPoint3d_normalize(p0 + p5);
		p4 = VuoPoint3d_normalize(p2 + p5);

		VuoPoint3d_setArray(&p[index++ * 3], p0);
		VuoPoint3d_setArray(&p[index++ * 3], p1);
		VuoPoint3d_setArray(&p[index++ * 3], p3);

		VuoPoint3d_setArray(&p[index++ * 3], p1);
		VuoPoint3d_setArray(&p[index++ * 3], p2);
		VuoPoint3d_setArray(&p[index++ * 3], p4);

		VuoPoint3d_setArray(&p[index++ * 3], p1);
		VuoPoint3d_setArray(&p[index++ * 3], p4);
		VuoPoint3d_setArray(&p[index++ * 3], p3);

		VuoPoint3d_setArray(&p[index++ * 3], p3);
		VuoPoint3d_setArray(&p[index++ * 3], p4);
		VuoPoint3d_setArray(&p[index++ * 3], p5);
	}

	return p;
}

static inline VuoPoint3d cross2d(VuoPoint2d a, VuoPoint2d b, VuoPoint2d c)
{
	VuoPoint3d a0 = (VuoPoint3d){b.x - a.x, b.y - a.y, 1};
	VuoPoint3d a1 = (VuoPoint3d){c.x - a.x, c.y - a.y, 1};
	return VuoPoint3d_crossProduct(a0, a1);
}

static VuoTransform icoTransform(VuoTransform transform)
{
	// Rotate so the seam is in the back (just like the lat/lon sphere).
	return VuoTransform_composite(VuoTransform_makeEuler((VuoPoint3d){0,0,0}, (VuoPoint3d){0,-PI/2,0}, (VuoPoint3d){1,1,1}), transform);
}

struct nodeInstanceData
{
	VuoInteger subdivisions;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->subdivisions = -1;
	return context;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoTransform) transform,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoColor":{"r":1,"g":1,"b":1,"a":1}}}) material,
	VuoInputData(VuoInteger, {"default":3,"suggestedMin":0, "suggestedMax":7}) subdivisions,
	VuoOutputData(VuoSceneObject) object
)
{
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (subdivisions == (*context)->subdivisions)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, icoTransform(transform));
		VuoSceneObject_setShader(*object, VuoShader_make_VuoGenericType1(material));
		return;
	}

	unsigned int i = 0;

	// Template
	VuoPoint3d ico[] =
	{
		VuoPoint3d_normalize((VuoPoint3d){-1,  PHI,  0}),
		VuoPoint3d_normalize((VuoPoint3d){ 1,  PHI,  0}),
		VuoPoint3d_normalize((VuoPoint3d){-1, -PHI,  0}),
		VuoPoint3d_normalize((VuoPoint3d){ 1, -PHI,  0}),
		VuoPoint3d_normalize((VuoPoint3d){ 0, -1,  PHI}),
		VuoPoint3d_normalize((VuoPoint3d){ 0,  1,  PHI}),
		VuoPoint3d_normalize((VuoPoint3d){ 0, -1, -PHI}),
		VuoPoint3d_normalize((VuoPoint3d){ 0,  1, -PHI}),
		VuoPoint3d_normalize((VuoPoint3d){  PHI, 0, -1}),
		VuoPoint3d_normalize((VuoPoint3d){  PHI, 0,  1}),
		VuoPoint3d_normalize((VuoPoint3d){ -PHI, 0, -1}),
		VuoPoint3d_normalize((VuoPoint3d){ -PHI, 0,  1}),
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
	float *positions;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, NULL, NULL, 0, NULL);
	// Don't allocate normals and textureCoordinates since we don't yet know how many vertices there will be.

	for(i = 0; i < vertexCount; i++)
		VuoPoint3d_setArray(&positions[i * 3], ico[tri[i]]);

	unsigned int sub = MIN(MAX(0, subdivisions), 7);

	for(i = 0; i < sub; i++)
	{
		float *v = subdivideIcosahedron(positions, vertexCount);
		vertexCount *= 4;
		free(positions);
		positions = v;
	}

	float *normals, *textureCoordinates;
	VuoMesh_allocateCPUBuffers(vertexCount, NULL, &normals, &textureCoordinates, NULL, 0, NULL);
	for(int i = 0; i < vertexCount; ++i)
	{
		VuoPoint3d n = VuoPoint3d_makeFromArray(&positions[i * 3]);
		VuoPoint3d_setArray(&normals[i * 3], n);

		textureCoordinates[i * 2    ] = 1. - (.5 + (atan2(n.z, n.x) / (2 * PI)));
		textureCoordinates[i * 2 + 1] = 1. - (.5 - (asin(n.y) / PI));

		VuoPoint3d_setArray(&positions[i * 3], VuoPoint3d_makeFromArray(&positions[i * 3]) * ICO_RADIUS);
	}

	VuoPoint2d min, max;

	for(int i = 0; i < vertexCount; i+=3)
	{
		int a = i+0,
			b = i+1,
			c = i+2;

		VuoPoint2d ta = VuoPoint2d_makeFromArray(&textureCoordinates[a * 2]);
		VuoPoint2d tb = VuoPoint2d_makeFromArray(&textureCoordinates[b * 2]);
		VuoPoint2d tc = VuoPoint2d_makeFromArray(&textureCoordinates[c * 2]);

		VuoPoint3d cross = cross2d(ta, tb, tc);

		if(cross.z < 0)
		{
			if (ta.x < .2) ++textureCoordinates[a * 2];
			if (tb.x < .2) ++textureCoordinates[b * 2];
			if (tc.x < .2) ++textureCoordinates[c * 2];
		}

		if (IS_POLE(VuoPoint3d_makeFromArray(&normals[a * 3])))
			textureCoordinates[a * 2] = (tb.x + tc.x) / 2;
		else if (IS_POLE(VuoPoint3d_makeFromArray(&normals[b * 3])))
			textureCoordinates[b * 2] = (ta.x + tc.x) / 2;
		else if (IS_POLE(VuoPoint3d_makeFromArray(&normals[c * 3])))
			textureCoordinates[c * 2] = (ta.x + tb.x) / 2;

		VuoPoint2d ti = VuoPoint2d_makeFromArray(&textureCoordinates[i * 2]);
		if(i == 0)
			min = max = ti;

		min.x = fmin(min.x, ti.x);
		min.y = fmin(min.y, ti.y);

		max.x = fmax(max.x, ti.x);
		max.y = fmax(max.y, ti.y);
	}

	// let uvs roam free
	// https://b33p.net/kosada/node/12372#comment-53659
	// VuoPoint2d scale = { 1 / (max.x - min.x), 1 / (max.y - min.y) };
	// for(int i = 0; i < submesh.vertexCount; i++)
	// {
	// 	textures[i].x -= min.x;
	// 	textures[i].y -= min.y;
	// 	textures[i].x *= scale.x;
	// 	textures[i].y *= scale.y;
	// }

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, normals, textureCoordinates, NULL,
		0, NULL, VuoMesh_IndividualTriangles);

	*object = VuoSceneObject_makeMesh(mesh, VuoShader_make_VuoGenericType1(material), icoTransform(transform));
	VuoSceneObject_setName(*object, VuoText_make("Icosphere"));

	(*context)->subdivisions = subdivisions;
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
}
