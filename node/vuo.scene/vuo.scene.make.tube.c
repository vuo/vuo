/**
 * @file
 * vuo.scene.make.tube node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMeshUtility.h"
#include <OpenGL/CGLMacro.h>
#include "VuoList_VuoSceneObject.h"

VuoModuleMetadata({
					"title" : "Make Tube",
					"keywords" : [ "3D", "pipe", "cylinder", "capsule", "barrel", "tubular", "shape" ],
					"version" : "1.1.1",
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
static float *makeCircleTemplate(const unsigned int columns, const float radius)
{
	float *points = (float *)malloc(sizeof(float) * 3 * columns);

	for(int i = 0; i < columns; i++)
	{
		points[i * 3    ] = radius * cos( ( i / (float)columns ) * (2 * PI) );
		points[i * 3 + 1] = 0;
		points[i * 3 + 2] = radius * sin( ( i / (float)columns ) * (2 * PI) );
	}

	return points;
}

/**
 * Generates a submesh tube with @columns columns and @rows rows.
 */
static VuoMesh makePipeShell(const unsigned int columns,
							const unsigned int rows,
							const float radius,
							const float height,
							const unsigned int winding,
							VuoMesh_FaceCulling culling)
{
	float *circle = makeCircleTemplate(columns, radius);

	// wrap shell
	unsigned int vertexCount = (columns+1) * (rows+1);
	unsigned int elementCount = 6 * columns * rows;

	float *positions, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, NULL, elementCount, &elements);

	VuoPoint3d p = VuoPoint3d_make(0,0,0);
	VuoPoint2d t = VuoPoint2d_make(0,0);
	unsigned int n = 0;

	float textureWidth = 1;//radius/uvRadius;
	float textureHeight = 1.;//(1 / (2 * PI * uvRadius));
	bool flip = winding == WINDING_CCW;

	for(int y = 0; y < rows+1; y++)
	{
		t.x = 0;
		t.y = y/(float)rows * textureHeight;

		for(int x = 0; x < columns; x++)
		{
			p.x = circle[x * 3];
			p.y = (y/(float)rows) * height - height / (float)2;
			p.z = circle[x * 3 + 2];
			t.x = textureWidth - ((x / (float)columns) * textureWidth);
			if (flip)
				t.x = 1. - t.x;

			VuoPoint2d_setArray(&textureCoordinates[n * 2], t);
			VuoPoint3d_setArray(&positions[n * 3], p);
			++n;
		}

		// add a vert at the vertical seam
		p.x = circle[0];
		p.y = (y/(float)rows) * height - height / (float)2;
		p.z = circle[2];

		t.x = flip ? 1 : 0;//textureWidth;
		VuoPoint2d_setArray(&textureCoordinates[n * 2], t);
		VuoPoint3d_setArray(&positions[n * 3], p);
		++n;
	}

	unsigned int step = columns+1;

	unsigned int index = 0;
	for(n = 0; n < elementCount; n+=6)
	{
		switch( winding )
		{
			case WINDING_CCW:
				{
					elements[n+0] = index+step;
					elements[n+1] = index;
					elements[n+2] = index+1;

					elements[n+3] = index+1;
					elements[n+4] = index+step+1;
					elements[n+5] = index+step;
				}
				break;

			default:
				{
					elements[n+2] = index+step;
					elements[n+1] = index+0;
					elements[n+0] = index+1;

					elements[n+5] = index+1;
					elements[n+4] = index+step+1;
					elements[n+3] = index+step;
				}
				break;
		}

		index += 1;
		if( (index+1) % step == 0 ) index++;	// don't use seam vertices as indices
	}

	free(circle);

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);
	VuoMesh_setFaceCulling(mesh, culling);

	VuoMeshUtility_calculateNormals(mesh);

	// Average the normals along seams
	float *normals;
	VuoMesh_getCPUBuffers(mesh, NULL, NULL, &normals, NULL, NULL, 0, NULL);

	for(unsigned int y = 0; y < rows+1; y++)
	{
		unsigned int i0 = y * step;
		unsigned int i1 = y * step + columns;

		VuoPoint3d n0 = VuoPoint3d_makeFromArray(&normals[i0 * 3]);
		VuoPoint3d n1 = VuoPoint3d_makeFromArray(&normals[i1 * 3]);
		VuoPoint3d avg = (n0 + n1) / 2.f;

		VuoPoint3d_setArray(&normals[i0 * 3], avg);
		VuoPoint3d_setArray(&normals[i1 * 3], avg);
	}

	VuoMesh_setCPUBuffers(mesh, vertexCount,
		positions, normals, textureCoordinates, NULL,
		elementCount, elements);

	return mesh;
}

/**
 * Builds a submesh for a pipe cap.  If tube has no inner shell, use makeCylinderCap instead.
 */
static VuoMesh makePipeCap(const unsigned int columns,
						const float radius,
						const float height,
						const float thickness,
						const unsigned int winding)
{
	unsigned int vertexCount = columns * 2;
	unsigned int elementCount = 6 * columns;

	float *positions, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, NULL, elementCount, &elements);

	// build vertices
	float *outer = makeCircleTemplate(columns, radius);
	float *inner = makeCircleTemplate(columns, radius-thickness);

	memcpy(positions, outer, sizeof(float) * 3 * columns);
	memcpy(&positions[columns * 3], inner, sizeof(float) * 3 * columns);

	float uvMax = 1;//(1 / (float)(2 * PI * radius));
	float diameter = radius * 2;
	bool flip = winding == WINDING_CW;

	for(int i = 0; i < vertexCount; i++)
	{
		positions[i * 3 + 1] = height;
		textureCoordinates[i * 2    ] = ((positions[i * 3] + radius)/diameter) * uvMax;
		textureCoordinates[i * 2 + 1] = flip ? (1. - ((positions[i * 3 + 2] + radius)/diameter) * uvMax) : (((positions[i * 3 + 2] + radius)/diameter) * uvMax);
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
					elements[n + 0] = t2;
					elements[n + 1] = t0;
					elements[n + 2] = t1;

					elements[n + 3] = t1;
					elements[n + 4] = t3;
					elements[n + 5] = t2;
				}
				break;

			default:
				{
					elements[n + 2] = t2;
					elements[n + 1] = t0;
					elements[n + 0] = t1;

					elements[n + 5] = t1;
					elements[n + 4] = t3;
					elements[n + 3] = t2;
				}
				break;
		}
		index++;
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);

	VuoMeshUtility_calculateNormals(mesh);

	return mesh;
}

/**
 * Generates a cap submesh for a cylinder.
 */
static VuoMesh makeCylinderCap(const unsigned int columns, const float radius, const float height, const unsigned int winding)
{
	unsigned int vertexCount = columns + 1;
	unsigned int elementCount = 3 * columns;

	float *positions, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, NULL, elementCount, &elements);

	// build vertices
	float *outer = makeCircleTemplate(columns, radius);
	positions[0] = 0;
	positions[1] = height;
	positions[2] = 0;
	memcpy(&positions[3], outer, sizeof(float) * 3 * columns);

	float uvMax = 1;//(1 / (float)(2 * PI * radius));
	float diameter = radius * 2;
	bool flip = winding == WINDING_CW;

	for(int i = 0; i < vertexCount; i++)
	{
		positions[i * 3 + 1] = height;
		textureCoordinates[i * 2    ] = ((positions[i * 3] + radius)/diameter) * uvMax;
		textureCoordinates[i * 2 + 1] = flip ? (1. - ((positions[i * 3 + 2] + radius)/diameter) * uvMax) : (((positions[i * 3 + 2] + radius)/diameter) * uvMax);
	}

	free(outer);

	for(unsigned int n = 0, index = 1; n < elementCount; n+=3, index++)
	{
		switch(winding)
		{
			case WINDING_CCW:
				elements[n + 0] = 0;
				elements[n + 1] = index;
				elements[n + 2] = index < columns ? index+1 : 1;
				break;

			default:
				elements[n + 2] = 0;
				elements[n + 1] = index;
				elements[n + 0] = index < columns ? index+1 : 1;
				break;
		}
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);

	VuoMeshUtility_calculateNormals(mesh);

	return mesh;
}

struct nodeInstanceData
{
	VuoInteger rows;
	VuoInteger columns;
	VuoReal thickness;
	bool inside;
	bool caps;
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
	// If the structure hasn't changed, just reuse the existing GPU mesh data.
	if (rows == (*context)->rows
	 && columns == (*context)->columns
	 && thickness == (*context)->thickness)
	{
		*object = VuoSceneObject_copy(*object);
		VuoSceneObject_setTransform(*object, transform);

		VuoSceneObject *objects = VuoListGetData_VuoSceneObject(VuoSceneObject_getChildObjects(*object));

		// The outside is always the first child object.
		unsigned long i = 0;
		VuoSceneObject_setShader(objects[i++], VuoShader_make_VuoGenericType1(outsideMaterial));

		if ((*context)->inside)
			VuoSceneObject_setShader(objects[i++], VuoShader_make_VuoGenericType2(insideMaterial));

		if ((*context)->caps)
		{
			VuoSceneObject_setShader(objects[i++], VuoShader_make_VuoGenericType3(topMaterial));
			VuoSceneObject_setShader(objects[i++], VuoShader_make_VuoGenericType4(bottomMaterial));
		}

		return;
	}

	float radius = .5, height = 1;

	// Make sure inputs are sane
	float thickness2 = fmin(1, fmax(0, thickness)) * radius;
	int rows2 = MIN(512, MAX(1, rows));
	int columns2 = MIN(512, MAX(3, columns));

	VuoMesh outer, inner, topCap, bottomCap;

	VuoList_VuoSceneObject meshes = VuoListCreate_VuoSceneObject();

	// generate the outer shell
	outer = makePipeShell(columns2, rows2, radius, height, WINDING_CW, VuoReal_areEqual(thickness2, 0) ? VuoMesh_CullNone : VuoMesh_CullBackfaces);
	VuoListAppendValue_VuoSceneObject(meshes, VuoSceneObject_makeMesh(outer,
		VuoShader_make_VuoGenericType1(outsideMaterial),
		VuoTransform_makeIdentity()));

	// if thickness isn't 0 or 1, generate an inner surface (wound counterclockwise)
	if( !VuoReal_areEqual(thickness2, 0) && !VuoReal_areEqual(thickness2, radius) )
	{
		inner = makePipeShell(columns2, rows2, radius-thickness2, height, WINDING_CCW, VuoMesh_CullBackfaces);
		VuoListAppendValue_VuoSceneObject(meshes, VuoSceneObject_makeMesh(inner,
			VuoShader_make_VuoGenericType2(insideMaterial),
			VuoTransform_makeIdentity()));
		(*context)->inside = true;
	}
	else
		(*context)->inside = false;

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

		VuoListAppendValue_VuoSceneObject(meshes, VuoSceneObject_makeMesh(topCap,
			VuoShader_make_VuoGenericType3(topMaterial),
			VuoTransform_makeIdentity()));

		VuoListAppendValue_VuoSceneObject(meshes, VuoSceneObject_makeMesh(bottomCap,
			VuoShader_make_VuoGenericType4(bottomMaterial),
			VuoTransform_makeIdentity()));

		(*context)->caps = true;
	}
	else
		(*context)->caps = false;

	*object = VuoSceneObject_makeGroup(meshes, transform);
	VuoSceneObject_setName(*object, VuoText_make("Tube"));

	(*context)->rows = rows;
	(*context)->columns = columns;
	(*context)->thickness = thickness;
}
