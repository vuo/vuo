/**
 * @file
 * VuoMesh implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OpenGL/CGLMacro.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#include "type.h"
#include "VuoMesh.h"
#include "VuoGlPool.h"
#include "VuoTransform.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Mesh",
					 "description" : "A 3D shape represented by a set of vertices with associated normals and other per-vertex data.",
					 "keywords" : [ "mesh", "vertex" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoPoint2d",
						"VuoPoint3d",
						"VuoPoint4d",
						"VuoList_VuoPoint2d",
						"VuoList_VuoPoint3d",
						"VuoText",
						"VuoTransform",
						"VuoGlContext",
						"VuoGlPool"
					 ]
				 });
#endif
/// @}

/**
 * Allocates the vertex (position, normal, ...) and element arrays.
 */
VuoSubmesh VuoSubmesh_make(unsigned int vertexCount, unsigned int elementCount)
{
	VuoSubmesh sm;

	sm.vertexCount = vertexCount;
	sm.positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	sm.normals = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	sm.tangents = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	sm.bitangents = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	sm.textureCoordinates = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	sm.elementCount = elementCount;
	sm.elements = (unsigned int *)malloc(sizeof(unsigned int)*sm.elementCount);
	sm.faceCullingMode = GL_BACK;
	memset(&sm.glUpload, 0, sizeof(sm.glUpload));

	return sm;
}

/**
 * Creates a @c VuoSubmesh consisting of data that's already been uploaded to the GPU.
 */
VuoSubmesh VuoSubmesh_makeGl(unsigned int vertexCount, unsigned int combinedBuffer, unsigned int combinedBufferSize, void *normalOffset, void *tangentOffset, void *bitangentOffset, void *textureCoordinateOffset, unsigned int elementCount, unsigned int elementBuffer, unsigned int elementBufferSize, VuoMesh_ElementAssemblyMethod elementAssemblyMethod)
{
	VuoSubmesh sm;

	sm.vertexCount = vertexCount;
	sm.positions = NULL;
	sm.normals = NULL;
	sm.tangents = NULL;
	sm.bitangents = NULL;
	sm.textureCoordinates = NULL;
	sm.elementCount = elementCount;
	sm.elements = NULL;
	sm.elementAssemblyMethod = elementAssemblyMethod;
	sm.faceCullingMode = GL_BACK;
	sm.glUpload.combinedBuffer = combinedBuffer;
	sm.glUpload.combinedBufferSize = combinedBufferSize;
	sm.glUpload.normalOffset = normalOffset;
	sm.glUpload.tangentOffset = tangentOffset;
	sm.glUpload.bitangentOffset = bitangentOffset;
	sm.glUpload.textureCoordinateOffset = textureCoordinateOffset;
	sm.glUpload.elementBuffer = elementBuffer;
	sm.glUpload.elementBufferSize = elementBufferSize;

	return sm;
}

/**
 * Returns the GL mode (e.g., `GL_TRIANGLES`) that @c submesh should be interpreted as.
 */
unsigned long VuoSubmesh_getGlMode(VuoSubmesh submesh)
{
	if (submesh.elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return GL_TRIANGLES;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleStrip)
		return GL_TRIANGLE_STRIP;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleFan)
		return GL_TRIANGLE_FAN;
	else if (submesh.elementAssemblyMethod == VuoMesh_IndividualLines)
		return GL_LINES;
	else if (submesh.elementAssemblyMethod == VuoMesh_LineStrip)
		return GL_LINE_STRIP;
	else if (submesh.elementAssemblyMethod == VuoMesh_Points)
		return GL_POINTS;

	return GL_TRIANGLES;
}

/**
 * Returns the number of split primitives in @c submesh.
 *
 * For example:
 *
 *    - If the submesh is VuoMesh_IndividualTriangles and has elementCount=6, it would render 2 triangles, so this function returns 2.
 *    - If the submesh is VuoMesh_TriangleStrip       and has elementCount=6, it would render 4 triangles, so this function returns 4.
 */
unsigned long VuoSubmesh_getSplitPrimitiveCount(VuoSubmesh submesh)
{
	if (submesh.elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return submesh.elementCount ? submesh.elementCount/3 : submesh.vertexCount/3;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleStrip)
		return submesh.elementCount ? submesh.elementCount-2 : submesh.vertexCount-2;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleFan)
		return submesh.elementCount ? submesh.elementCount-2 : submesh.vertexCount-2;
	else if (submesh.elementAssemblyMethod == VuoMesh_IndividualLines)
		return submesh.elementCount ? submesh.elementCount/2 : submesh.vertexCount/2;
	else if (submesh.elementAssemblyMethod == VuoMesh_LineStrip)
		return submesh.elementCount ? submesh.elementCount-1 : submesh.vertexCount-1;
	else if (submesh.elementAssemblyMethod == VuoMesh_Points)
		return submesh.elementCount ? submesh.elementCount : submesh.vertexCount;

	return 0;
}

/**
 * Returns the number of split vertices in @c submesh.
 *
 * For example:
 *
 *    - If the submesh is VuoMesh_IndividualTriangles and has elementCount=6, it would render 2 triangles, each of which has 3 vertices, so this function returns 6.
 *    - If the submesh is VuoMesh_TriangleStrip       and has elementCount=6, it would render 4 triangles, each of which has 3 vertices, so this function returns 12.
 */
unsigned long VuoSubmesh_getSplitVertexCount(VuoSubmesh submesh)
{
	if (submesh.elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return submesh.elementCount ? submesh.elementCount : submesh.vertexCount;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleStrip)
		return submesh.elementCount ? (submesh.elementCount-2)*3 : (submesh.vertexCount-2)*3;
	else if (submesh.elementAssemblyMethod == VuoMesh_TriangleFan)
		return submesh.elementCount ? (submesh.elementCount-2)*3 : (submesh.vertexCount-2)*3;
	else if (submesh.elementAssemblyMethod == VuoMesh_IndividualLines)
		return submesh.elementCount ? submesh.elementCount : submesh.vertexCount;
	else if (submesh.elementAssemblyMethod == VuoMesh_LineStrip)
		return submesh.elementCount ? (submesh.elementCount-1)*2 : (submesh.vertexCount-1)*2;
	else if (submesh.elementAssemblyMethod == VuoMesh_Points)
		return submesh.elementCount ? submesh.elementCount : submesh.vertexCount;

	return 0;
}

/**
 * Frees the mesh and each of its submeshes, including the vertex and element arrays within it.
 */
void VuoMesh_free(void *value)
{
	VuoMesh m = (VuoMesh)value;

	for (unsigned int i = 0; i < m->submeshCount; ++i)
	{
		VuoSubmesh sm = m->submeshes[i];
		free(sm.positions);
		free(sm.normals);
		free(sm.tangents);
		free(sm.bitangents);
		free(sm.textureCoordinates);
		free(sm.elements);

		VuoGlPool_release(VuoGlPool_ArrayBuffer, sm.glUpload.combinedBufferSize, sm.glUpload.combinedBuffer);
		if (sm.glUpload.elementBufferSize && sm.glUpload.elementBuffer)
			VuoGlPool_release(VuoGlPool_ElementArrayBuffer, sm.glUpload.elementBufferSize, sm.glUpload.elementBuffer);
	}

	free(m->submeshes);
	free(m);
}

/**
 * Creates and registers a mesh with empty slots for the given number of sub-meshes.
 *
 * After you've populated the sub-meshes, call @ref VuoMesh_upload().
 */
VuoMesh VuoMesh_make(unsigned int submeshCount)
{
	if (submeshCount == 0)
		return NULL;

	VuoMesh m = (VuoMesh)malloc(submeshCount * sizeof(struct _VuoMesh));
	VuoRegister(m, VuoMesh_free);
	m->submeshCount = submeshCount;
	m->submeshes = (VuoSubmesh *)calloc(submeshCount, sizeof(VuoSubmesh));
	return m;
}

/**
 * Uploads @c mesh to the GPU.
 */
void VuoMesh_upload(VuoMesh mesh)
{
	if (!mesh)
		return;

	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

	// Create a temporary Vertex Array Object, so we can bind the buffers in order to upload them.
	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	// For each mesh item in the sceneobject...
	for (unsigned long i = 0; i < mesh->submeshCount; ++i)
	{
		VuoSubmesh meshItem = mesh->submeshes[i];


		// Combine the vertex attribute buffers together, so we only have to upload a single one per submesh:


		// Decide on interleaved positions for vertex attributes.
		// Needs to be interleaved because glTransformFeedbackVaryings is only able to write to 4 buffers, but we have more than 4 buffers.
		unsigned int bufferCount = 0;

		++bufferCount; // position

		unsigned int normalOffset = 0; // Offsets are VuoPoint4ds (not bytes).
		if (meshItem.normals)
			normalOffset = bufferCount++;
		mesh->submeshes[i].glUpload.normalOffset = (void *)(normalOffset*sizeof(VuoPoint4d));

		unsigned int tangentOffset = 0;
		if (meshItem.tangents)
			tangentOffset = bufferCount++;
		mesh->submeshes[i].glUpload.tangentOffset = (void *)(tangentOffset*sizeof(VuoPoint4d));

		unsigned int bitangentOffset = 0;
		if (meshItem.bitangents)
			bitangentOffset = bufferCount++;
		mesh->submeshes[i].glUpload.bitangentOffset = (void *)(bitangentOffset*sizeof(VuoPoint4d));

		unsigned int textureCoordinateOffset = 0;
		if (meshItem.textureCoordinates)
			textureCoordinateOffset = bufferCount++;
		mesh->submeshes[i].glUpload.textureCoordinateOffset = (void *)(textureCoordinateOffset*sizeof(VuoPoint4d));

		unsigned long singleBufferSize = sizeof(VuoPoint4d)*meshItem.vertexCount;
		VuoPoint4d *combinedData = (VuoPoint4d *)malloc(singleBufferSize*bufferCount);


		// Combine vertex attributes into the interleaved client-side buffer.
		for (unsigned long i = 0; i < meshItem.vertexCount; ++i)
		{
			combinedData[i*bufferCount] = meshItem.positions[i];
			if (meshItem.normals)
				combinedData[i*bufferCount+normalOffset] = meshItem.normals[i];
			if (meshItem.tangents)
				combinedData[i*bufferCount+tangentOffset] = meshItem.tangents[i];
			if (meshItem.bitangents)
				combinedData[i*bufferCount+bitangentOffset] = meshItem.bitangents[i];
			if (meshItem.textureCoordinates)
				combinedData[i*bufferCount+textureCoordinateOffset] = meshItem.textureCoordinates[i];
		}


		// Upload the combined buffer.
		mesh->submeshes[i].glUpload.combinedBufferSize = singleBufferSize*bufferCount;
		mesh->submeshes[i].glUpload.combinedBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer, mesh->submeshes[i].glUpload.combinedBufferSize);
		VuoGlPool_retain(mesh->submeshes[i].glUpload.combinedBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->submeshes[i].glUpload.combinedBuffer);
		glBufferData(GL_ARRAY_BUFFER, singleBufferSize*bufferCount, combinedData, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ARRAY_BUFFER, 0, v.combinedBufferSize, combinedData);
		free(combinedData);


		// Upload the Element Buffer and add it to the Vertex Array Object
		mesh->submeshes[i].glUpload.elementBufferSize = sizeof(unsigned int)*meshItem.elementCount;
		mesh->submeshes[i].glUpload.elementBuffer = VuoGlPool_use(VuoGlPool_ElementArrayBuffer, mesh->submeshes[i].glUpload.elementBufferSize);
		VuoGlPool_retain(mesh->submeshes[i].glUpload.elementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->submeshes[i].glUpload.elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*meshItem.elementCount, meshItem.elements, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, v.elementBufferSize, meshItem.elements);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // handled by glDeleteVertexArrays()
//	glBindVertexArray(0); // handled by glDeleteVertexArrays()
	glDeleteVertexArrays(1, &vertexArray);

	// Prepare for this mesh to be used on a different OpenGL context.
	glFlushRenderAPPLE();

	VuoGlContext_disuse(cgl_ctx);
}

/**
 * Creates and registers a mesh with space for one submesh, and sets it to the given submesh.
 * Uploads the submesh to the GPU.
 */
VuoMesh VuoMesh_makeFromSingleSubmesh(VuoSubmesh submesh)
{
	VuoMesh m = VuoMesh_make(1);
	m->submeshes[0] = submesh;
	VuoMesh_upload(m);
	return m;
}

/**
 * Creates and registers a singleton mesh with space for one submesh, and sets it to the given submesh.
 * Uploads the submesh to the GPU.
 */
static VuoMesh VuoMesh_makeSingletonFromSingleSubmesh(VuoSubmesh submesh)
{
	VuoMesh m = (VuoMesh)malloc(sizeof(struct _VuoMesh));
	VuoRegisterSingleton(m);
	m->submeshCount = 1;
	m->submeshes = (VuoSubmesh *)calloc(1, sizeof(VuoSubmesh));
	m->submeshes[0] = submesh;
	VuoMesh_upload(m);
	return m;
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 */
static VuoMesh VuoMesh_makeQuadWithNormalsInternal(void)
{
	VuoSubmesh sm = VuoSubmesh_make(4, 6);

	// Positions
	{
		sm.positions[0] = VuoPoint4d_make(-.5,-.5,0,1);
		sm.positions[1] = VuoPoint4d_make( .5,-.5,0,1);
		sm.positions[2] = VuoPoint4d_make(-.5, .5,0,1);
		sm.positions[3] = VuoPoint4d_make( .5, .5,0,1);
	}

	// Normals
	{
		for (int i=0; i<sm.vertexCount; ++i)
			sm.normals[i] = VuoPoint4d_make(0,0,1,1);
	}

	// Tangents
	{
		for (int i=0; i<sm.vertexCount; ++i)
			sm.tangents[i] = VuoPoint4d_make(1,0,0,1);
	}

	// Bitangents
	{
		for (int i=0; i<sm.vertexCount; ++i)
			sm.bitangents[i] = VuoPoint4d_make(0,1,0,1);
	}

	// Texture Coordinates
	{
		sm.textureCoordinates[0] = VuoPoint4d_make(0,0,0,1);
		sm.textureCoordinates[1] = VuoPoint4d_make(1,0,0,1);
		sm.textureCoordinates[2] = VuoPoint4d_make(0,1,0,1);
		sm.textureCoordinates[3] = VuoPoint4d_make(1,1,0,1);
	}

	// Triangle elements
	sm.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	sm.elements[0] = 2;
	sm.elements[1] = 0;
	sm.elements[2] = 1;
	sm.elements[3] = 1;
	sm.elements[4] = 3;
	sm.elements[5] = 2;

	return VuoMesh_makeSingletonFromSingleSubmesh(sm);
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 *
 * The quad consists of only positions and texture coordinates (without normals, tangents, or bitangents).
 */
static VuoMesh VuoMesh_makeQuadWithoutNormalsInternal(void)
{
	VuoSubmesh sm;

	sm.vertexCount = 4;

	sm.positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	{
		sm.positions[0] = VuoPoint4d_make(-.5,-.5,0,1);
		sm.positions[1] = VuoPoint4d_make( .5,-.5,0,1);
		sm.positions[2] = VuoPoint4d_make(-.5, .5,0,1);
		sm.positions[3] = VuoPoint4d_make( .5, .5,0,1);
	}

	sm.normals = NULL;
	sm.tangents = NULL;
	sm.bitangents = NULL;

	sm.textureCoordinates = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	{
		sm.textureCoordinates[0] = VuoPoint4d_make(0,0,0,1);
		sm.textureCoordinates[1] = VuoPoint4d_make(1,0,0,1);
		sm.textureCoordinates[2] = VuoPoint4d_make(0,1,0,1);
		sm.textureCoordinates[3] = VuoPoint4d_make(1,1,0,1);
	}

	sm.elementCount = 6;
	sm.elements = (unsigned int *)malloc(sizeof(unsigned int)*sm.elementCount);
	sm.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	sm.faceCullingMode = GL_BACK;
	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	sm.elements[0] = 2;
	sm.elements[1] = 0;
	sm.elements[2] = 1;
	sm.elements[3] = 1;
	sm.elements[4] = 3;
	sm.elements[5] = 2;

	return VuoMesh_makeSingletonFromSingleSubmesh(sm);
}

/**
 * Returns an equilateral triangle with bottom edge length 1, pointing upward on the XY plane, centered at the origin.
 */
static VuoMesh VuoMesh_makeEquilateralTriangleInternal(void)
{
	VuoSubmesh sm = VuoSubmesh_make(3, 3);

	// Positions
	for (int i = 0; i < sm.vertexCount; ++i)
	{
		float angle = M_PI/2. + i * 2*M_PI/3.;
		sm.positions[i] = VuoPoint4d_make(cos(angle)/sqrt(3), sin(angle)/sqrt(3), 0, 1);
	}

	// Normals
	for (int i=0; i<sm.vertexCount; ++i)
		sm.normals[i] = VuoPoint4d_make(0,0,1,1);

	// Tangents
	for (int i=0; i<sm.vertexCount; ++i)
		sm.tangents[i] = VuoPoint4d_make(1,0,0,1);

	// Bitangents
	for (int i=0; i<sm.vertexCount; ++i)
		sm.bitangents[i] = VuoPoint4d_make(0,1,0,1);

	// Texture Coordinates
	sm.textureCoordinates[0] = VuoPoint4d_make(.5,1,0,1);
	sm.textureCoordinates[1] = VuoPoint4d_make(0,0,0,1);
	sm.textureCoordinates[2] = VuoPoint4d_make(1,0,0,1);

	// Triangle Strip elements
	sm.elementAssemblyMethod = VuoMesh_TriangleStrip;
	sm.elements[0] = 0;
	sm.elements[1] = 1;
	sm.elements[2] = 2;

	return VuoMesh_makeSingletonFromSingleSubmesh(sm);
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 *
 * This mesh is shared.  Don't modify its contents.
 */
VuoMesh VuoMesh_makeQuad(void)
{
	static VuoMesh sharedQuadWithNormals;
	static dispatch_once_t token = 0;
	dispatch_once(&token, ^{
					  sharedQuadWithNormals = VuoMesh_makeQuadWithNormalsInternal();
					  VuoRetain(sharedQuadWithNormals);
				  });
	return sharedQuadWithNormals;
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 *
 * The quad consists of only positions and texture coordinates (without normals, tangents, or bitangents).
 *
 * This mesh is shared.  Don't modify its contents.
 */
VuoMesh VuoMesh_makeQuadWithoutNormals(void)
{
	static VuoMesh sharedQuadWithoutNormals;
	static dispatch_once_t token = 0;
	dispatch_once(&token, ^{
					  sharedQuadWithoutNormals = VuoMesh_makeQuadWithoutNormalsInternal();
					  VuoRetain(sharedQuadWithoutNormals);
				  });
	return sharedQuadWithoutNormals;
}

/**
 * Returns an equilateral triangle with bottom edge length 1, pointing upward on the XY plane, centered at the origin.
 *
 * This mesh is shared.  Don't modify its contents.
 */
VuoMesh VuoMesh_makeEquilateralTriangle(void)
{
	static VuoMesh sharedEquilateralTriangle;
	static dispatch_once_t token = 0;
	dispatch_once(&token, ^{
					  sharedEquilateralTriangle = VuoMesh_makeEquilateralTriangleInternal();
					  VuoRetain(sharedEquilateralTriangle);
				  });
	return sharedEquilateralTriangle;
}

/**
 * Returns a VuoMesh consisting of the given positions and element assembly method.
 * Its normals, tangents, bitangents, and texture coordinates are all null.
 */
VuoMesh VuoMesh_make_VuoPoint2d(VuoList_VuoPoint2d positions, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize)
{
	unsigned long count = VuoListGetCount_VuoPoint2d(positions);
	VuoPoint2d *positionValues = VuoListGetData_VuoPoint2d(positions);
	VuoSubmesh sm;

	sm.vertexCount = count;
	sm.positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*count);

	sm.elementCount = count;
	sm.elements = (unsigned int *)malloc(sizeof(unsigned int)*count);

	for (unsigned long i = 0; i < count; ++i)
	{
		VuoPoint2d xy = positionValues[i];
		sm.positions[i] = (VuoPoint4d){xy.x, xy.y, 0, 1};
		sm.elements[i] = i;
	}

	sm.elementAssemblyMethod = elementAssemblyMethod;
	sm.primitiveSize = primitiveSize;
	sm.faceCullingMode = GL_BACK;

	sm.normals = NULL;
	sm.tangents = NULL;
	sm.bitangents = NULL;
	sm.textureCoordinates = NULL;

	return VuoMesh_makeFromSingleSubmesh(sm);
}

/**
 * Returns a VuoMesh consisting of the given positions and element assembly method.
 * Its normals, tangents, bitangents, and texture coordinates are all null.
 */
VuoMesh VuoMesh_make_VuoPoint3d(VuoList_VuoPoint3d positions, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(positions);
	VuoPoint3d *positionValues = VuoListGetData_VuoPoint3d(positions);
	VuoSubmesh sm;

	sm.vertexCount = count;
	sm.positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*count);

	sm.elementCount = count;
	sm.elements = (unsigned int *)malloc(sizeof(unsigned int)*count);

	for (unsigned long i = 0; i < count; ++i)
	{
		VuoPoint3d xyz = positionValues[i];
		sm.positions[i] = (VuoPoint4d){xyz.x, xyz.y, xyz.z, 1};
		sm.elements[i] = i;
	}

	sm.elementAssemblyMethod = elementAssemblyMethod;
	sm.primitiveSize = primitiveSize;
	sm.faceCullingMode = GL_BACK;

	sm.normals = NULL;
	sm.tangents = NULL;
	sm.bitangents = NULL;
	sm.textureCoordinates = NULL;

	return VuoMesh_makeFromSingleSubmesh(sm);
}

/**
 * Makes a copy of the mesh and its submeshes.
 * CPU mesh data is duplicated.
 */
VuoMesh VuoMesh_copy(const VuoMesh mesh)
{
	if (!mesh)
		return NULL;

	VuoMesh copiedMesh = VuoMesh_make(mesh->submeshCount);
	for (unsigned int i = 0; i < mesh->submeshCount; ++i)
	{
		VuoSubmesh submesh = mesh->submeshes[i];
		VuoSubmesh copiedSubmesh;

		copiedSubmesh.vertexCount = submesh.vertexCount;
		unsigned long attributeByteCount = sizeof(VuoPoint4d)*copiedSubmesh.vertexCount;

		if (submesh.positions)
		{
			copiedSubmesh.positions = (VuoPoint4d *)malloc(attributeByteCount);
			memcpy(copiedSubmesh.positions, submesh.positions, attributeByteCount);
		}
		else
			copiedSubmesh.positions = NULL;

		if (submesh.normals)
		{
			copiedSubmesh.normals = (VuoPoint4d *)malloc(attributeByteCount);
			memcpy(copiedSubmesh.normals, submesh.normals, attributeByteCount);
		}
		else
			copiedSubmesh.normals = NULL;

		if (submesh.tangents)
		{
			copiedSubmesh.tangents = (VuoPoint4d *)malloc(attributeByteCount);
			memcpy(copiedSubmesh.tangents, submesh.tangents, attributeByteCount);
		}
		else
			copiedSubmesh.tangents = NULL;

		if (submesh.bitangents)
		{
			copiedSubmesh.bitangents = (VuoPoint4d *)malloc(attributeByteCount);
			memcpy(copiedSubmesh.bitangents, submesh.bitangents, attributeByteCount);
		}
		else
			copiedSubmesh.bitangents = NULL;

		if (submesh.textureCoordinates)
		{
			copiedSubmesh.textureCoordinates = (VuoPoint4d *)malloc(attributeByteCount);
			memcpy(copiedSubmesh.textureCoordinates, submesh.textureCoordinates, attributeByteCount);
		}
		else
			copiedSubmesh.textureCoordinates = NULL;

		copiedSubmesh.elementCount = submesh.elementCount;
		if (submesh.elements)
		{
			unsigned long elementByteCount = sizeof(unsigned int)*submesh.elementCount;
			copiedSubmesh.elements = (unsigned int *)malloc(elementByteCount);
			memcpy(copiedSubmesh.elements, submesh.elements, elementByteCount);
		}
		else
			copiedSubmesh.elements = NULL;

		copiedSubmesh.elementAssemblyMethod = submesh.elementAssemblyMethod;
		copiedSubmesh.primitiveSize = submesh.primitiveSize;
		copiedSubmesh.faceCullingMode = submesh.faceCullingMode;

		memcpy(&copiedSubmesh.glUpload, &submesh.glUpload, sizeof(copiedSubmesh.glUpload));
		VuoGlPool_retain(copiedSubmesh.glUpload.combinedBuffer);
		VuoGlPool_retain(copiedSubmesh.glUpload.elementBuffer);

		copiedMesh->submeshes[i] = copiedSubmesh;
	}
	return copiedMesh;
}

/**
 * @ingroup VuoMesh
 * Returns the @c VuoMesh_ElementAssemblyMethod corresponding with the string @c elementAssemblyMethodString.  If none matches, returns VuoMesh_IndividualTriangles.
 */
VuoMesh_ElementAssemblyMethod VuoMesh_elementAssemblyMethodFromCString(const char * elementAssemblyMethodString)
{
	if (strcmp(elementAssemblyMethodString,"individualTriangles")==0)
		return VuoMesh_IndividualTriangles;
	else if (strcmp(elementAssemblyMethodString,"triangleStrip")==0)
		return VuoMesh_TriangleStrip;
	else if (strcmp(elementAssemblyMethodString,"triangleFan")==0)
		return VuoMesh_TriangleFan;
	else if (strcmp(elementAssemblyMethodString,"individualLines")==0)
		return VuoMesh_IndividualLines;
	else if (strcmp(elementAssemblyMethodString,"lineStrip")==0)
		return VuoMesh_LineStrip;
	else if (strcmp(elementAssemblyMethodString,"points")==0)
		return VuoMesh_Points;

	return VuoMesh_IndividualTriangles;
}

/**
 * Returns a string constant representing @c elementAssemblyMethod.
 *
 * Do not free the value returned from this function.
 */
const char * VuoMesh_cStringForElementAssemblyMethod(VuoMesh_ElementAssemblyMethod elementAssemblyMethod)
{
	switch (elementAssemblyMethod)
	{
		case VuoMesh_IndividualTriangles:
			return "individualTriangles";
		case VuoMesh_TriangleStrip:
			return "triangleStrip";
		case VuoMesh_TriangleFan:
			return "triangleFan";
		case VuoMesh_IndividualLines:
			return "individualLines";
		case VuoMesh_LineStrip:
			return "lineStrip";
		case VuoMesh_Points:
			return "points";
	}
	return "(unknown element assembly method)";
}

/**
 *	Pulls an element array from gl buffer.
 */
VuoPoint4d *extractElements(CGLContextObj cgl_ctx, VuoSubmesh *submesh, unsigned int vertexCount, unsigned int bufferCount, unsigned int bufferIndex)
{
	GLfloat *feedback = (GLfloat *)malloc(submesh->glUpload.combinedBufferSize);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, submesh->glUpload.combinedBufferSize, feedback);

	VuoPoint4d *elements = (VuoPoint4d*)malloc(sizeof(VuoPoint4d)*vertexCount);

	for (int vertex = 0; vertex < vertexCount; vertex++)
	{
		elements[vertex] = (VuoPoint4d)
			{
				feedback[vertex*4*bufferCount + 0 + (bufferIndex*4)],
				feedback[vertex*4*bufferCount + 1 + (bufferIndex*4)],
				feedback[vertex*4*bufferCount + 2 + (bufferIndex*4)],
				feedback[vertex*4*bufferCount + 3 + (bufferIndex*4)]
			};
	}
	free(feedback);

	return elements;
}

/**
 *	Copies element data from GPU back to the CPU.
 */
void VuoSubmeshMesh_download(VuoSubmesh *submesh)
{
	unsigned int vertexCount = submesh->vertexCount;
	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

	// Attach the input combinedBuffer for rendering.
	glBindBuffer(GL_ARRAY_BUFFER, submesh->glUpload.combinedBuffer);

	int bufferCount = 0;
	++bufferCount;
	if (submesh->glUpload.normalOffset)
		++bufferCount;
	if (submesh->glUpload.tangentOffset)
		++bufferCount;
	if (submesh->glUpload.bitangentOffset)
		++bufferCount;
	if (submesh->glUpload.textureCoordinateOffset)
		++bufferCount;

	glFlush();

	int bufferIndex = 0;

	submesh->positions = extractElements(cgl_ctx, submesh, vertexCount, bufferCount, bufferIndex++);
	VuoRegister(submesh->positions, free);


	if(submesh->glUpload.normalOffset)
	{
		submesh->normals = extractElements(cgl_ctx, submesh, vertexCount, bufferCount, bufferIndex++);
		VuoRegister(submesh->normals, free);
	}

	if(submesh->glUpload.tangentOffset)
	{
		submesh->tangents = extractElements(cgl_ctx, submesh, vertexCount, bufferCount, bufferIndex++);
		VuoRegister(submesh->tangents, free);
	}

	if(submesh->glUpload.bitangentOffset)
	{
		submesh->bitangents = extractElements(cgl_ctx, submesh, vertexCount, bufferCount, bufferIndex++);
		VuoRegister(submesh->bitangents, free);
	}

	if(submesh->glUpload.textureCoordinateOffset)
	{
		submesh->textureCoordinates = extractElements(cgl_ctx, submesh, vertexCount, bufferCount, bufferIndex++);
		VuoRegister(submesh->textureCoordinates, free);
	}

	VuoGlContext_disuse(cgl_ctx);
}

/**
 *	Iterates through mesh's vertices finding the center and extents.  Returns the axis aligned bounding box taking into account the passed transform.
 */
VuoBox VuoMesh_bounds(const VuoMesh v, float matrix[16])
{
	VuoPoint3d min, max;
	bool init = false;

	for(int i = 0; i < v->submeshCount; i++)
	{
		unsigned int vertexCount = v->submeshes[i].vertexCount;

		if(v->submeshes[i].positions == NULL)
			VuoSubmeshMesh_download(&v->submeshes[i]);

		if(vertexCount > 0 && !init)
		{
			min = max = VuoTransform_transformPoint((float*)matrix, VuoPoint3d_make(v->submeshes[i].positions[0].x, v->submeshes[i].positions[0].y, v->submeshes[i].positions[0].z));
			init = true;
		}

		for(int n = 0; n < vertexCount; n++)
		{
			VuoPoint3d p = VuoTransform_transformPoint((float*)matrix, VuoPoint3d_make(v->submeshes[i].positions[n].x, v->submeshes[i].positions[n].y, v->submeshes[i].positions[n].z));

			min.x = MIN(p.x, min.x);
			min.y = MIN(p.y, min.y);
			min.z = MIN(p.z, min.z);

			max.x = MAX(p.x, max.x);
			max.y = MAX(p.y, max.y);
			max.z = MAX(p.z, max.z);
		}
	}

	if(init)
		return VuoBox_make( VuoPoint3d_multiply(VuoPoint3d_add(min, max), 0.5), VuoPoint3d_subtract(max, min) );
	else
		return VuoBox_make( (VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0} );
}

/**
 * Returns true if the mesh has at least one submesh with a non-zero vertex count.
 */
bool VuoMesh_isPopulated(const VuoMesh mesh)
{
	if (!mesh)
		return false;

	for (unsigned long i = 0; i < mesh->submeshCount; ++i)
	{
		VuoSubmesh meshItem = mesh->submeshes[i];
		if (meshItem.vertexCount > 0)
			return true;
	}

	return false;
}

/**
 * @ingroup VuoMesh
 * Decodes the JSON object @c js to create a new value.
 *
 * @param js A JSON array of elements of the format parsed by VuoSubmesh_makeFromJson().
 */
VuoMesh VuoMesh_makeFromJson(json_object * js)
{
	if (!js)
		return NULL;

	return (VuoMesh)json_object_get_int64(js);
}

/**
 * @ingroup VuoMesh
 * Encodes @c value as a JSON object.
 */
json_object * VuoMesh_getJson(const VuoMesh value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * @ingroup VuoMesh
 * A brief summary of the contents of this submesh.
 *
 * @eg{4 vertices in a fan of 2 triangles
 * with first position (0,0,0,0)}
 * @eg{8 vertices, 12 triangles
 * with first position (-0.5,-0.5,-0.5,0)}
 */
char * VuoSubmesh_getSummary(const VuoSubmesh value)
{
	if (value.vertexCount == 0)
	{
		char * zero = strdup("0 vertices");
		return zero;
	}

	unsigned long objectCount = VuoSubmesh_getSplitPrimitiveCount(value);
	const char * objectString = "";
	const char * assemblyMethod = " (unknown element assembly method)";
	if (value.elementAssemblyMethod == VuoMesh_IndividualTriangles)
	{
		assemblyMethod = ", ";
		objectString = "triangle";
		/// @todo Report if value.elementCount isn't a multiple of 3.
	}
	else if (value.elementAssemblyMethod == VuoMesh_TriangleStrip)
	{
		assemblyMethod = " in a strip of ";
		objectString = "triangle";
	}
	else if (value.elementAssemblyMethod == VuoMesh_TriangleFan)
	{
		assemblyMethod = " in a fan of ";
		objectString = "triangle";
	}
	else if (value.elementAssemblyMethod == VuoMesh_IndividualLines)
	{
		assemblyMethod = ", ";
		objectString = "line";
	}
	else if (value.elementAssemblyMethod == VuoMesh_LineStrip)
	{
		assemblyMethod = " in a strip of ";
		objectString = "line";
	}
	else if (value.elementAssemblyMethod == VuoMesh_Points)
	{
		assemblyMethod = ", ";
		objectString = "point";
	}

	const char * vertexCountString = value.vertexCount==1 ? "vertex" : "vertices";
	const char * objectStringPlural = objectCount==1 ? "" : "s";
	VuoPoint4d p = value.positions[0];

	return VuoText_format("%u %s%s%lu %s%s<br>with first position (%g, %g, %g, %g)",
						  value.vertexCount, vertexCountString, assemblyMethod, objectCount, objectString, objectStringPlural,
						  p.x, p.y, p.z, p.w);
}

/**
 * @ingroup VuoMesh
 * A brief summary of the contents of this mesh, including a list of sub-meshes.
 */
char * VuoMesh_getSummary(const VuoMesh value)
{
	if (!value || value->submeshCount == 0)
		return strdup("(empty mesh)");

	char *prefix = "Mesh containing: <ul>";
	unsigned int prefixBytes = strlen(prefix);

	const int maxSubmeshCount = 20;
	unsigned int submeshCount = MIN(value->submeshCount, maxSubmeshCount);
	char **submeshes = (char **)malloc(submeshCount * sizeof(char *));
	unsigned int submeshBytes = 0;
	for (unsigned int i = 0; i < submeshCount; ++i)
	{
		char *s = VuoSubmesh_getSummary(value->submeshes[i]);
		submeshes[i] = (char *)malloc((strlen(s) + 4 + 5 + 1));
		submeshes[i][0] = 0;
		strncat(submeshes[i], "<li>", 4);
		strncat(submeshes[i], s, strlen(s));
		strncat(submeshes[i], "</li>", 5);
		submeshBytes += strlen(submeshes[i]);
		free(s);
	}

	char *suffix = (value->submeshCount > maxSubmeshCount ? "<li>...</li></ul>" : "</ul>");
	unsigned int suffixBytes = strlen(suffix);

	char *summary = (char *)malloc(prefixBytes + submeshBytes + suffixBytes + 1);
	summary[0] = 0;
	strncat(summary, prefix, prefixBytes);
	for (unsigned int i = 0; i < submeshCount; ++i)
		strncat(summary, submeshes[i], strlen(submeshes[i]));
	strncat(summary, suffix, suffixBytes);

	for (unsigned int i = 0; i < submeshCount; ++i)
		free(submeshes[i]);
	free(submeshes);

	return summary;
}
