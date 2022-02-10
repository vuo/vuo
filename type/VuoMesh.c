/**
 * @file
 * VuoMesh implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "VuoMacOSSDKWorkaround.h"
#include <OpenGL/CGLMacro.h>
/// @{ Stub.
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Mesh",
					 "description" : "A 3D shape represented by a set of vertices with associated normals and other per-vertex data.",
					 "keywords" : [ "mesh", "vertex" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoColor",
						"VuoPoint2d",
						"VuoPoint3d",
						"VuoPoint4d",
						"VuoList_VuoColor",
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
 * @private VuoMesh fields.
 */
typedef struct
{
	unsigned int vertexCount;        ///< Number of vertices in `positions`, `normals`, and `textureCoordinates`.
	float *positions;                ///< XYZ vertex positions.
	float *normals;                  ///< XYZ vertex normals.  May be `NULL`.
	float *textureCoordinates;       ///< ST texture coordinates.  May be `NULL`.
	float *colors;                   ///< RGBA colors.  May be `NULL`.

	unsigned int elementCount;       ///< Number of elements in @c elements.
	/**
	 * An array of size elementCount of integer elements (triangle indices) which are indexes into `positions`.
	 */
	unsigned int *elements;

	/// The way in which the `elements` array should be interpreted during rasterization.
	VuoMesh_ElementAssemblyMethod elementAssemblyMethod;

	/**
	 * For lines, the width (in scene units).
	 * For points, the width and height (in scene units).
	 */
	VuoReal primitiveSize;

	VuoMesh_FaceCulling faceCulling;

	/**
	 * References to mesh data uploaded to the GPU.
	 */
	struct
	{
		unsigned int combinedBuffer;
		unsigned int combinedBufferSize;
		unsigned int combinedBufferStride;  ///< Number of bytes per buffer entry.  If 0, the stride is calculated automatically based on sizeof(VuoPoint4d) multiplied by 1 (position) + the number of non-NULL offsets below.

		void *normalOffset;
		void *textureCoordinateOffset;
		void *colorOffset;

		unsigned int elementBuffer;
		unsigned int elementBufferSize;
	} glUpload;
} VuoMesh_internal;

/**
 * Allocates:
 *
 *    - positions — 3 floats per vertex
 *    - normals — 3 floats per vertex
 *    - texture coordinates — 2 floats per vertex
 *    - colors — 4 floats per vertex
 *    - elements — 1 unsigned int per element
 *
 * @version200New
 */
void VuoMesh_allocateCPUBuffers(unsigned int vertexCount,
    float **positions, float **normals, float **textureCoordinates, float **colors,
    unsigned int elementCount, unsigned int **elements)
{
	if (positions)
		*positions          = (float *)malloc(sizeof(float) * 3 * vertexCount);
	if (normals)
		*normals            = (float *)malloc(sizeof(float) * 3 * vertexCount);
	if (textureCoordinates)
		*textureCoordinates = (float *)malloc(sizeof(float) * 2 * vertexCount);
	if (colors)
		*colors             = (float *)malloc(sizeof(float) * 4 * vertexCount);
	if (elements)
		*elements = elementCount ? (unsigned int *)malloc(sizeof(unsigned int) * elementCount) : NULL;
}

/**
 * Frees the mesh and each of its submeshes, including the vertex and element arrays within it.
 */
static void VuoMesh_free(void *value)
{
	VuoMesh_internal *m = (VuoMesh_internal *)value;

	free(m->positions);
	free(m->normals);
	free(m->textureCoordinates);
	free(m->colors);
	free(m->elements);

	VuoGlPool_release(VuoGlPool_ArrayBuffer, m->glUpload.combinedBufferSize, m->glUpload.combinedBuffer);
	VuoGlPool_release(VuoGlPool_ElementArrayBuffer, m->glUpload.elementBufferSize, m->glUpload.elementBuffer);

	free(m);
}

/**
 * @private Creates and registers a mesh.
 */
static VuoMesh_internal *VuoMesh_makeInternal(void)
{
	VuoMesh_internal *m = (VuoMesh_internal *)calloc(1, sizeof(VuoMesh_internal));
	VuoRegister(m, VuoMesh_free);
	m->faceCulling = VuoMesh_CullBackfaces;
	return m;
}

/**
 * @private Creates and registers a singleton mesh.
 */
static VuoMesh_internal *VuoMesh_makeSingletonInternal(void)
{
	VuoMesh_internal *m = (VuoMesh_internal *)calloc(1, sizeof(VuoMesh_internal));
	VuoRegisterSingleton(m);
	m->faceCulling = VuoMesh_CullBackfaces;
	return m;
}

static void VuoMesh_upload(VuoMesh_internal *m);

/**
 * Creates a mesh consisting of data that already exists in CPU RAM,
 * and uploads it to the GPU.
 *
 * The mesh object takes ownership of the arrays passed into it,
 * which it will eventually release using `free()`.
 *
 * @see VuoMesh_getCPUBuffers
 *
 * @version200Changed{positions and normals are now VuoPoint3d; textureCoordinates is now VuoPoint2d;
 * tangents and bitangents are no longer stored (they're now calculated when needed).}
 */
VuoMesh VuoMesh_makeFromCPUBuffers(unsigned int vertexCount,
    float *positions, float *normals, float *textureCoordinates, float *colors,
    unsigned int elementCount, unsigned int *elements, VuoMesh_ElementAssemblyMethod elementAssemblyMethod)
{
	VuoMesh_internal *m = VuoMesh_makeInternal();

	m->vertexCount = vertexCount;
	m->positions = positions;
	m->normals = normals;
	m->textureCoordinates = textureCoordinates;
	m->colors = colors;
	m->elementCount = elementCount;
	m->elements = elements;
	m->elementAssemblyMethod = elementAssemblyMethod;

	VuoMesh_upload(m);
	return (VuoMesh)m;
}

/**
 * Creates a @c VuoSubmesh consisting of data that's already been uploaded to the GPU.
 */
VuoMesh VuoMesh_makeFromGPUBuffers(unsigned int vertexCount, unsigned int combinedBuffer, unsigned int combinedBufferSize, void *normalOffset, void *textureCoordinateOffset, void *colorOffset, unsigned int elementCount, unsigned int elementBuffer, unsigned int elementBufferSize, VuoMesh_ElementAssemblyMethod elementAssemblyMethod)
{
	VuoMesh_internal *m = VuoMesh_makeInternal();

	m->vertexCount = vertexCount;
	m->elementCount = elementCount;
	m->elementAssemblyMethod = elementAssemblyMethod;
	m->glUpload.combinedBuffer = combinedBuffer;
	m->glUpload.combinedBufferSize = combinedBufferSize;
	m->glUpload.normalOffset = normalOffset;
	m->glUpload.textureCoordinateOffset = textureCoordinateOffset;
	m->glUpload.colorOffset = colorOffset;
	m->glUpload.elementBuffer = elementBuffer;
	m->glUpload.elementBufferSize = elementBufferSize;

	return (VuoMesh)m;
}

/**
 * Returns the GL mode (e.g., `GL_TRIANGLES`) that the mesh should be interpreted as.
 */
unsigned long VuoMesh_getGlMode(VuoMesh mesh)
{
	if (!mesh)
		return GL_NONE;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	if (m->elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return GL_TRIANGLES;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleStrip)
		return GL_TRIANGLE_STRIP;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleFan)
		return GL_TRIANGLE_FAN;
	else if (m->elementAssemblyMethod == VuoMesh_IndividualLines)
		return GL_LINES;
	else if (m->elementAssemblyMethod == VuoMesh_LineStrip)
		return GL_LINE_STRIP;
	else if (m->elementAssemblyMethod == VuoMesh_Points)
		return GL_POINTS;

	return GL_TRIANGLES;
}

/**
 * Returns the number of split primitives in the mesh.
 *
 * For example:
 *
 *    - If the mesh is VuoMesh_IndividualTriangles and has elementCount=6, it would render 2 triangles, so this function returns 2.
 *    - If the mesh is VuoMesh_TriangleStrip       and has elementCount=6, it would render 4 triangles, so this function returns 4.
 */
unsigned long VuoMesh_getSplitPrimitiveCount(VuoMesh mesh)
{
	if (!mesh)
		return 0;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	if (m->elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return m->elementCount ? m->elementCount/3 : m->vertexCount/3;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleStrip)
		return m->elementCount ? m->elementCount-2 : m->vertexCount-2;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleFan)
		return m->elementCount ? m->elementCount-2 : m->vertexCount-2;
	else if (m->elementAssemblyMethod == VuoMesh_IndividualLines)
		return m->elementCount ? m->elementCount/2 : m->vertexCount/2;
	else if (m->elementAssemblyMethod == VuoMesh_LineStrip)
		return m->elementCount ? m->elementCount-1 : m->vertexCount-1;
	else if (m->elementAssemblyMethod == VuoMesh_Points)
		return m->elementCount ? m->elementCount : m->vertexCount;

	return 0;
}

/**
 * Returns the number of split vertices in the mesh.
 *
 * For example:
 *
 *    - If the mesh is VuoMesh_IndividualTriangles and has elementCount=6, it would render 2 triangles, each of which has 3 vertices, so this function returns 6.
 *    - If the mesh is VuoMesh_TriangleStrip       and has elementCount=6, it would render 4 triangles, each of which has 3 vertices, so this function returns 12.
 */
unsigned long VuoMesh_getSplitVertexCount(VuoMesh mesh)
{
	if (!mesh)
		return 0;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	if (m->elementCount == 0 && m->vertexCount == 0)
		return 0;

	if (m->elementAssemblyMethod == VuoMesh_IndividualTriangles)
		return m->elementCount ? m->elementCount : m->vertexCount;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleStrip)
		return m->elementCount ? (m->elementCount-2)*3 : (m->vertexCount-2)*3;
	else if (m->elementAssemblyMethod == VuoMesh_TriangleFan)
		return m->elementCount ? (m->elementCount-2)*3 : (m->vertexCount-2)*3;
	else if (m->elementAssemblyMethod == VuoMesh_IndividualLines)
		return m->elementCount ? m->elementCount : m->vertexCount;
	else if (m->elementAssemblyMethod == VuoMesh_LineStrip)
		return m->elementCount ? (m->elementCount-1)*2 : (m->vertexCount-1)*2;
	else if (m->elementAssemblyMethod == VuoMesh_Points)
		return m->elementCount ? m->elementCount : m->vertexCount;

	return 0;
}

static unsigned long VuoMesh_getCompleteElementCountInternal(unsigned long elementCount, VuoMesh_ElementAssemblyMethod elementAssemblyMethod);

/**
 * Returns the number of complete elements in the mesh.
 *
 * If elementCount represents an incomplete element,
 * this function returns the rounded-down number of elements.
 *
 * For example, if elementCount = 5 and elementAssemblyMethod = VuoMesh_IndividualTriangles,
 * this function returns 3, discarding the last 2 elements that fail to form a complete triangle.
 */
unsigned long VuoMesh_getCompleteElementCount(const VuoMesh mesh)
{
	if (!mesh)
		return 0;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	unsigned long elementCount = m->elementCount ? m->elementCount : m->vertexCount;
	return VuoMesh_getCompleteElementCountInternal(elementCount, m->elementAssemblyMethod);
}

/**
 * Uploads @c mesh to the GPU.
 */
static void VuoMesh_upload(VuoMesh_internal *m)
{
	if (!m || m->glUpload.combinedBuffer)
		return;

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){

		// Create a temporary Vertex Array Object, so we can bind the buffers in order to upload them.
		GLuint vertexArray;
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		// Combine the vertex attribute buffers together, so we only have to upload a single one per mesh:

		m->glUpload.combinedBufferSize = sizeof(float) * 3 * m->vertexCount;  // positions

		uint64_t normalOffsetBytes = 0;
		if (m->normals)
		{
			normalOffsetBytes = m->glUpload.combinedBufferSize;
			m->glUpload.combinedBufferSize += sizeof(float) * 3 * m->vertexCount;
		}
		m->glUpload.normalOffset = (void *)normalOffsetBytes;

		uint64_t textureCoordinateOffsetBytes = 0;
		if (m->textureCoordinates)
		{
			textureCoordinateOffsetBytes = m->glUpload.combinedBufferSize;
			m->glUpload.combinedBufferSize += sizeof(float) * 2 * m->vertexCount;
		}
		m->glUpload.textureCoordinateOffset = (void *)textureCoordinateOffsetBytes;

		uint64_t colorOffsetBytes = 0;
		if (m->colors)
		{
			colorOffsetBytes = m->glUpload.combinedBufferSize;
			m->glUpload.combinedBufferSize += sizeof(float) * 4 * m->vertexCount;
		}
		m->glUpload.colorOffset = (void *)colorOffsetBytes;

		float *combinedData = (float *)malloc(m->glUpload.combinedBufferSize);
		for (int i =0; i < m->glUpload.combinedBufferSize/sizeof(float); ++i)
			combinedData[i]=-42;


		// Combine vertex attributes into the interleaved client-side buffer.
		for (unsigned long i = 0; i < m->vertexCount; ++i)
		{
			combinedData[i * 3    ] = m->positions[i * 3    ];
			combinedData[i * 3 + 1] = m->positions[i * 3 + 1];
			combinedData[i * 3 + 2] = m->positions[i * 3 + 2];
			if (m->normals)
			{
				combinedData[i * 3 + normalOffsetBytes / sizeof(float)    ] = m->normals[i * 3    ];
				combinedData[i * 3 + normalOffsetBytes / sizeof(float) + 1] = m->normals[i * 3 + 1];
				combinedData[i * 3 + normalOffsetBytes / sizeof(float) + 2] = m->normals[i * 3 + 2];
			}
			if (m->textureCoordinates)
			{
				combinedData[i * 2 + textureCoordinateOffsetBytes / sizeof(float)    ] = m->textureCoordinates[i * 2    ];
				combinedData[i * 2 + textureCoordinateOffsetBytes / sizeof(float) + 1] = m->textureCoordinates[i * 2 + 1];
			}
			if (m->colors)
			{
				combinedData[i * 4 + colorOffsetBytes / sizeof(float)    ] = m->colors[i * 4    ];
				combinedData[i * 4 + colorOffsetBytes / sizeof(float) + 1] = m->colors[i * 4 + 1];
				combinedData[i * 4 + colorOffsetBytes / sizeof(float) + 2] = m->colors[i * 4 + 2];
				combinedData[i * 4 + colorOffsetBytes / sizeof(float) + 3] = m->colors[i * 4 + 3];
			}
		}

//		for (int i =0; i < m->glUpload.combinedBufferSize/sizeof(float); ++i)
//			VLog("%3d  %f",i,combinedData[i]);


		// Upload the combined buffer.
//		VLog("vertexCount=%d  normals=%d  tc=%d  colors=%d  combinedBuffer=%u", m->vertexCount, m->normals?1:0, m->textureCoordinates?1:0, m->colors?1:0, m->glUpload.combinedBufferSize);
		m->glUpload.combinedBuffer = VuoGlPool_use(cgl_ctx, VuoGlPool_ArrayBuffer, m->glUpload.combinedBufferSize);
		VuoGlPool_retain(m->glUpload.combinedBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m->glUpload.combinedBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, m->glUpload.combinedBufferSize, combinedData);
		free(combinedData);


		// Upload the Element Buffer and add it to the Vertex Array Object
		m->glUpload.elementBufferSize = sizeof(unsigned int)*m->elementCount;
		m->glUpload.elementBuffer = VuoGlPool_use(cgl_ctx, VuoGlPool_ElementArrayBuffer, m->glUpload.elementBufferSize);
		VuoGlPool_retain(m->glUpload.elementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->glUpload.elementBuffer);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int) * m->elementCount, m->elements);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // handled by glDeleteVertexArrays()
//		glBindVertexArray(0); // handled by glDeleteVertexArrays()
		glDeleteVertexArrays(1, &vertexArray);

		// Prepare for this mesh to be used on a different OpenGL context.
		// @@@ is this still needed?
		glFlushRenderAPPLE();

	});
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 */
static VuoMesh VuoMesh_makeQuadWithNormalsInternal(void)
{
	VuoMesh_internal *m = VuoMesh_makeSingletonInternal();
	m->vertexCount = 4;
	m->elementCount = 6;
	VuoMesh_allocateCPUBuffers(m->vertexCount, &m->positions, &m->normals, &m->textureCoordinates, NULL, m->elementCount, &m->elements);

	// Positions
	VuoPoint3d_setArray(&m->positions[0 * 3], (VuoPoint3d){-.5, -.5, 0});
	VuoPoint3d_setArray(&m->positions[1 * 3], (VuoPoint3d){ .5, -.5, 0});
	VuoPoint3d_setArray(&m->positions[2 * 3], (VuoPoint3d){-.5,  .5, 0});
	VuoPoint3d_setArray(&m->positions[3 * 3], (VuoPoint3d){ .5,  .5, 0});

	// Normals
	for (int i = 0; i < m->vertexCount; ++i)
		VuoPoint3d_setArray(&m->normals[i * 3], (VuoPoint3d){0, 0, 1});

	// Texture Coordinates
	VuoPoint2d_setArray(&m->textureCoordinates[0 * 2], (VuoPoint2d){0, 0});
	VuoPoint2d_setArray(&m->textureCoordinates[1 * 2], (VuoPoint2d){1, 0});
	VuoPoint2d_setArray(&m->textureCoordinates[2 * 2], (VuoPoint2d){0, 1});
	VuoPoint2d_setArray(&m->textureCoordinates[3 * 2], (VuoPoint2d){1, 1});

	// Triangle elements
	m->elementAssemblyMethod = VuoMesh_IndividualTriangles;
	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	m->elements[0] = 2;
	m->elements[1] = 0;
	m->elements[2] = 1;
	m->elements[3] = 1;
	m->elements[4] = 3;
	m->elements[5] = 2;

	VuoMesh_upload(m);
	return (VuoMesh)m;
}

/**
 * Returns a quad with dimensions 1x1, on the XY plane, centered at the origin.
 *
 * The quad consists of only positions and texture coordinates (without normals, tangents, or bitangents).
 */
static VuoMesh VuoMesh_makeQuadWithoutNormalsInternal(void)
{
	VuoMesh_internal *m = VuoMesh_makeSingletonInternal();
	m->vertexCount = 4;
	m->elementCount = 6;
	VuoMesh_allocateCPUBuffers(m->vertexCount, &m->positions, NULL, &m->textureCoordinates, NULL, m->elementCount, &m->elements);

	// Positions
	m->positions[0]  = -.5;
	m->positions[1]  = -.5;
	m->positions[2]  = 0;

	m->positions[3]  =  .5;
	m->positions[4]  = -.5;
	m->positions[5]  = 0;

	m->positions[6]  = -.5;
	m->positions[7]  =  .5;
	m->positions[8]  = 0;

	m->positions[9]  =  .5;
	m->positions[10] =  .5;
	m->positions[11] = 0;

	// Texture Coordinates
	m->textureCoordinates[0] = 0;
	m->textureCoordinates[1] = 0;

	m->textureCoordinates[2] = 1;
	m->textureCoordinates[3] = 0;

	m->textureCoordinates[4] = 0;
	m->textureCoordinates[5] = 1;

	m->textureCoordinates[6] = 1;
	m->textureCoordinates[7] = 1;

	// Triangle elements
	m->elementAssemblyMethod = VuoMesh_IndividualTriangles;
	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	m->elements[0] = 2;
	m->elements[1] = 0;
	m->elements[2] = 1;
	m->elements[3] = 1;
	m->elements[4] = 3;
	m->elements[5] = 2;

	VuoMesh_upload(m);
	return (VuoMesh)m;
}

/**
 * Returns an equilateral triangle with bottom edge length 1, pointing upward on the XY plane, centered at the origin.
 */
static VuoMesh VuoMesh_makeEquilateralTriangleInternal(void)
{
	VuoMesh_internal *m = VuoMesh_makeSingletonInternal();
	m->vertexCount = 3;
	m->elementCount = 3;
	VuoMesh_allocateCPUBuffers(m->vertexCount, &m->positions, &m->normals, &m->textureCoordinates, NULL, m->elementCount, &m->elements);

	// Positions
	for (int i = 0; i < m->vertexCount; ++i)
	{
		float angle = M_PI/2. + i * 2*M_PI/3.;
		VuoPoint3d_setArray(&m->positions[i * 3], (VuoPoint3d){ cos(angle) / sqrt(3), sin(angle) / sqrt(3), 0 });
	}

	// Normals
	for (int i = 0; i < m->vertexCount; ++i)
		VuoPoint3d_setArray(&m->normals[i * 3], (VuoPoint3d){ 0, 0, 1 });

	// Texture Coordinates
	VuoPoint2d_setArray(&m->textureCoordinates[0 * 2], (VuoPoint2d){.5, 1});
	VuoPoint2d_setArray(&m->textureCoordinates[1 * 2], (VuoPoint2d){ 0, 0});
	VuoPoint2d_setArray(&m->textureCoordinates[2 * 2], (VuoPoint2d){ 1, 0});

	// Triangle Strip elements
	m->elementAssemblyMethod = VuoMesh_TriangleStrip;
	m->elements[0] = 0;
	m->elements[1] = 1;
	m->elements[2] = 2;

	VuoMesh_upload(m);
	return (VuoMesh)m;
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
 * Returns a cube of size 1x1.
 */
static VuoMesh VuoMesh_makeCubeInternal(void)
{
	// Separate vertices for each face, so each face can have its own sharp normals and texture coordinates.

	VuoPoint3d positions[] = (VuoPoint3d[]){
		// Front
		{-.5, -.5,  .5},
		{ .5, -.5,  .5},
		{-.5,  .5,  .5},
		{ .5,  .5,  .5},
		// Right
		{ .5, -.5,  .5},
		{ .5, -.5, -.5},
		{ .5,  .5,  .5},
		{ .5,  .5, -.5},
		// Bottom
		{-.5, -.5, -.5},
		{ .5, -.5, -.5},
		{-.5, -.5,  .5},
		{ .5, -.5,  .5},
		// Left
		{-.5, -.5, -.5},
		{-.5, -.5,  .5},
		{-.5,  .5, -.5},
		{-.5,  .5,  .5},
		// Top
		{-.5,  .5,  .5},
		{ .5,  .5,  .5},
		{-.5,  .5, -.5},
		{ .5,  .5, -.5},
		// Back
		{ .5, -.5, -.5},
		{-.5, -.5, -.5},
		{ .5,  .5, -.5},
		{-.5,  .5, -.5},
	};

	VuoPoint3d normals[] = (VuoPoint3d[]){
		// Front
		{0, 0, 1},
		{0, 0, 1},
		{0, 0, 1},
		{0, 0, 1},
		// Right
		{1, 0, 0},
		{1, 0, 0},
		{1, 0, 0},
		{1, 0, 0},
		// Bottom
		{0, -1, 0},
		{0, -1, 0},
		{0, -1, 0},
		{0, -1, 0},
		// Left
		{-1, 0, 0},
		{-1, 0, 0},
		{-1, 0, 0},
		{-1, 0, 0},
		// Top
		{0, 1, 0},
		{0, 1, 0},
		{0, 1, 0},
		{0, 1, 0},
		// Back
		{0, 0, -1},
		{0, 0, -1},
		{0, 0, -1},
		{0, 0, -1},
	};

	VuoPoint2d textureCoordinates[] = (VuoPoint2d[]){
		// Front
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		// Right
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		// Bottom
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		// Left
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		// Top
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
		// Back
		{0, 0},
		{1, 0},
		{0, 1},
		{1, 1},
	};

	unsigned int elements[] = (unsigned int[]){
			// Front
			 2,  0,  1,
			 1,  3,  2,
			// Right
			 6,  4,  5,
			 5,  7,  6,
			// Bottom
			10,  8,  9,
			 9, 11, 10,
			// Left
			14, 12, 13,
			13, 15, 14,
			// Top
			18, 16, 17,
			17, 19, 18,
			// Back
			22, 20, 21,
			21, 23, 22,
	};

	VuoMesh_internal *m = VuoMesh_makeSingletonInternal();
	m->vertexCount = 6 * 4;
	m->elementCount = 6 * 6;
	VuoMesh_allocateCPUBuffers(m->vertexCount, &m->positions, &m->normals, &m->textureCoordinates, NULL, m->elementCount, &m->elements);

	for (int i = 0; i < m->vertexCount; ++i)
	{
		VuoPoint3d_setArray(&m->positions[i * 3], positions[i]);
		VuoPoint3d_setArray(&m->normals[i * 3], normals[i]);
		VuoPoint2d_setArray(&m->textureCoordinates[i * 2], textureCoordinates[i]);
	}
	memcpy(m->elements, elements, sizeof(elements));

	VuoMesh_upload(m);
	return (VuoMesh)m;
}

/**
 * Returns a cube of size 1x1.
 *
 * This mesh is shared.  Don't modify its contents.
 */
VuoMesh VuoMesh_makeCube(void)
{
	static VuoMesh sharedCube;
	static dispatch_once_t token = 0;
	dispatch_once(&token, ^{
					  sharedCube = VuoMesh_makeCubeInternal();
					  VuoRetain(sharedCube);
				  });
	return sharedCube;
}

/**
 * Creates a flat mesh subdivided into rows and columns.
 *
 * @version200New
 */
VuoMesh VuoMesh_makePlane(VuoInteger columns, VuoInteger rows)
{
	unsigned int vertexCount = rows * columns;
	unsigned int triangleCount = (rows-1) * (columns-1) * 6;

	float *positions, *normals, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, &normals, &textureCoordinates, NULL, triangleCount, &elements);

	unsigned int index = 0, t_index = 0;

	for(unsigned int i = 0; i < rows; i++)
	{
		float y = (i/(float)(rows-1)) - .5;

		for(unsigned int n = 0; n < columns; n++)
		{
			float x = (n/(float)(columns-1)) - .5;

			VuoPoint3d_setArray(&positions         [index * 3], (VuoPoint3d){x, y, 0});
			VuoPoint3d_setArray(&normals           [index * 3], (VuoPoint3d){0, 0, 1});
			VuoPoint2d_setArray(&textureCoordinates[index * 2], (VuoPoint2d){x + .5, y + .5});

			if(n < columns-1 && i < rows-1)
			{
				elements[t_index++] = index + columns;
				elements[t_index++] = index;
				elements[t_index++] = index + 1;

				elements[t_index++] = index + 1;
				elements[t_index++] = index + columns + 1;
				elements[t_index++] = index + columns;
			}

			index++;
		}
	}

	return VuoMesh_makeFromCPUBuffers(vertexCount,
									  positions, normals, textureCoordinates, NULL,
									  triangleCount, elements, VuoMesh_IndividualTriangles);
}

/**
 * @private Helper for VuoMesh_make_VuoPoint* and VuoMesh_getCompleteElementCount.
 */
static unsigned long VuoMesh_getCompleteElementCountInternal(unsigned long elementCount, VuoMesh_ElementAssemblyMethod elementAssemblyMethod)
{
	if (elementAssemblyMethod == VuoMesh_IndividualTriangles)
		// Round down to a multiple of 3, since each triangle requires 3 vertices.
		return (elementCount / 3) * 3;

	else if (elementAssemblyMethod == VuoMesh_TriangleStrip
		  || elementAssemblyMethod == VuoMesh_TriangleFan)
		// Triangle strips and fans must have at least 1 complete triangle.
		return elementCount < 3 ? 0 : elementCount;

	else if (elementAssemblyMethod == VuoMesh_IndividualLines)
		// Round down to an even number of vertices, since each line requires a pair of vertices.
		return (elementCount / 2) * 2;

	else if (elementAssemblyMethod == VuoMesh_LineStrip)
		// Line strips must have at least 1 complete line.
		return elementCount < 2 ? 0 : elementCount;

	else if (elementAssemblyMethod == VuoMesh_Points)
		// Since all points are independent, any number of points is fine.
		return elementCount;

	else
	{
		VUserLog("Error: Unknown submesh element assembly method: %d", elementAssemblyMethod);
		return 0;
	}
}

/**
 * Returns a VuoMesh consisting of the given positions and element assembly method.
 * Its normals, tangents, bitangents, and texture coordinates are all null.
 */
VuoMesh VuoMesh_make_VuoPoint2d(VuoList_VuoPoint2d positions, VuoList_VuoColor colors, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize)
{
	unsigned long positionCount = VuoListGetCount_VuoPoint2d(positions);
	VuoPoint2d *positionValues = VuoListGetData_VuoPoint2d(positions);

	positionCount = VuoMesh_getCompleteElementCountInternal(positionCount, elementAssemblyMethod);
	if (!positionCount)
		return NULL;

	unsigned long colorCount = VuoListGetCount_VuoColor(colors);
	VuoColor *colorValues = VuoListGetData_VuoColor(colors);

	float *positionsFloat, *normals, *colorsFloat = NULL;
	VuoMesh_allocateCPUBuffers(positionCount, &positionsFloat, &normals, NULL, colorCount ? &colorsFloat : NULL, 0, NULL);
	for (unsigned long i = 0; i < positionCount; ++i)
	{
		VuoPoint2d xy = positionValues[i];
		positionsFloat[i * 3    ] = xy.x;
		positionsFloat[i * 3 + 1] = xy.y;
		positionsFloat[i * 3 + 2] = 0;

		normals[i * 3    ] = 0;
		normals[i * 3 + 1] = 0;
		normals[i * 3 + 2] = 1;

		if (colorCount)
		{
			float progress = (float)i / MAX(1, positionCount - 1);
			unsigned long colorIndex = round(progress * (colorCount - 1));
			VuoColor c = colorValues[colorIndex];
			colorsFloat[i * 4    ] = c.r * c.a;
			colorsFloat[i * 4 + 1] = c.g * c.a;
			colorsFloat[i * 4 + 2] = c.b * c.a;
			colorsFloat[i * 4 + 3] = c.a;
		}
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(positionCount,
		positionsFloat, normals, NULL, colorsFloat,
		0, NULL, elementAssemblyMethod);
	VuoMesh_setPrimitiveSize(mesh, primitiveSize);
	return mesh;
}

/**
 * Returns a VuoMesh consisting of the given positions and element assembly method.
 * Its normals, tangents, bitangents, and texture coordinates are all null.
 */
VuoMesh VuoMesh_make_VuoPoint3d(VuoList_VuoPoint3d positions, VuoList_VuoColor colors, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize)
{
	unsigned long positionCount  = VuoListGetCount_VuoPoint3d(positions);
	VuoPoint3d *positionValues = VuoListGetData_VuoPoint3d(positions);

	positionCount = VuoMesh_getCompleteElementCountInternal(positionCount, elementAssemblyMethod);
	if (!positionCount)
		return NULL;

	unsigned long colorCount = VuoListGetCount_VuoColor(colors);
	VuoColor *colorValues = VuoListGetData_VuoColor(colors);

	float *positionsFloat, *normals, *colorsFloat = NULL;
	VuoMesh_allocateCPUBuffers(positionCount, &positionsFloat, &normals, NULL, colorCount ? &colorsFloat : NULL, 0, NULL);
	for (unsigned long i = 0; i < positionCount; ++i)
	{
		VuoPoint3d_setArray(&positionsFloat[i * 3], positionValues[i]);

		normals[i * 3    ] = 0;
		normals[i * 3 + 1] = 0;
		normals[i * 3 + 2] = 1;

		if (colorCount)
		{
			float progress = (float)i / MAX(1, positionCount - 1);
			unsigned long colorIndex = round(progress * (colorCount - 1));
			VuoColor c = colorValues[colorIndex];
			colorsFloat[i * 4    ] = c.r * c.a;
			colorsFloat[i * 4 + 1] = c.g * c.a;
			colorsFloat[i * 4 + 2] = c.b * c.a;
			colorsFloat[i * 4 + 3] = c.a;
		}
	}

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(positionCount,
		positionsFloat, normals, NULL, colorsFloat,
		0, NULL, elementAssemblyMethod);
	VuoMesh_setPrimitiveSize(mesh, primitiveSize);
	return mesh;
}

/**
 * Duplicates the mesh's CPU data, and retains the mesh's GPU data.
 */
VuoMesh VuoMesh_copy(const VuoMesh mesh)
{
	if (!mesh)
		return NULL;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	VuoMesh_internal *copiedMesh = VuoMesh_makeInternal();
	copiedMesh->vertexCount = m->vertexCount;
	copiedMesh->elementCount = m->elementCount;

	if (m->positions)
	{
		unsigned long size = sizeof(float) * 3 * copiedMesh->vertexCount;
		copiedMesh->positions = (float *)malloc(size);
		memcpy(copiedMesh->positions, m->positions, size);
	}
	else
		copiedMesh->positions = NULL;

	if (m->normals)
	{
		unsigned long size = sizeof(float) * 3 * copiedMesh->vertexCount;
		copiedMesh->normals = (float *)malloc(size);
		memcpy(copiedMesh->normals, m->normals, size);
	}
	else
		copiedMesh->normals = NULL;

	if (m->textureCoordinates)
	{
		unsigned long size = sizeof(float) * 3 * copiedMesh->vertexCount;
		copiedMesh->textureCoordinates = (float *)malloc(size);
		memcpy(copiedMesh->textureCoordinates, m->textureCoordinates, size);
	}
	else
		copiedMesh->textureCoordinates = NULL;

	if (m->colors)
	{
		unsigned long size = sizeof(float) * 3 * copiedMesh->vertexCount;
		copiedMesh->colors = (float *)malloc(size);
		memcpy(copiedMesh->colors, m->colors, size);
	}
	else
		copiedMesh->colors = NULL;

	copiedMesh->elementCount = m->elementCount;
	if (m->elements)
	{
		unsigned long elementByteCount = sizeof(unsigned int)*m->elementCount;
		copiedMesh->elements = (unsigned int *)malloc(elementByteCount);
		memcpy(copiedMesh->elements, m->elements, elementByteCount);
	}
	else
		copiedMesh->elements = NULL;

	copiedMesh->elementAssemblyMethod = m->elementAssemblyMethod;
	copiedMesh->primitiveSize = m->primitiveSize;
	copiedMesh->faceCulling = m->faceCulling;

	memcpy(&copiedMesh->glUpload, &m->glUpload, sizeof(copiedMesh->glUpload));
	VuoGlPool_retain(copiedMesh->glUpload.combinedBuffer);
	VuoGlPool_retain(copiedMesh->glUpload.elementBuffer);

	return (VuoMesh)copiedMesh;
}

/**
 * Makes a shallow copy of the mesh and its submeshes.
 * CPU mesh data is not present in the copy.
 * GPU mesh data is retained.
 *
 * @version200New
 *
 * @todo Maybe someday redesign VuoMesh so CPU mesh data is reference-counted,
 * to avoid having to make the deep/shallow distinction?
 */
VuoMesh VuoMesh_copyShallow(const VuoMesh mesh)
{
	if (!mesh)
		return NULL;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	VuoMesh_internal *copiedMesh = VuoMesh_makeInternal();

	copiedMesh->vertexCount = m->vertexCount;
	copiedMesh->elementCount = m->elementCount;

	copiedMesh->elementAssemblyMethod = m->elementAssemblyMethod;
	copiedMesh->primitiveSize = m->primitiveSize;
	copiedMesh->faceCulling = m->faceCulling;

	memcpy(&copiedMesh->glUpload, &m->glUpload, sizeof(copiedMesh->glUpload));
	VuoGlPool_retain(copiedMesh->glUpload.combinedBuffer);
	VuoGlPool_retain(copiedMesh->glUpload.elementBuffer);

	return (VuoMesh)copiedMesh;
}

/**
 * Deallocates and clears the texture coordinate arrays associated with this mesh.
 *
 * @version200New
 */
void VuoMesh_removeTextureCoordinates(VuoMesh mesh)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	if (m->textureCoordinates)
	{
		free(m->textureCoordinates);
		m->textureCoordinates = NULL;
	}

	if (m->glUpload.combinedBuffer && m->glUpload.textureCoordinateOffset)
	{
		m->glUpload.textureCoordinateOffset = NULL;
		// The data is still allocated in GPU RAM as part of combinedBuffer;
		// just ignore that (rather than wasting time repacking the buffer).
	}
}

static void VuoMesh_download(VuoMesh_internal *m);

/**
 * Outputs vertex and element information, downloading the data from the GPU if needed.
 *
 * You may pass NULL to any of the output variables.
 *
 * Do not modify or free the output arrays; the mesh continues to own them.
 *
 * @version200New
 */
void VuoMesh_getCPUBuffers(const VuoMesh mesh, unsigned int *vertexCount,
    float **positions, float **normals, float **textureCoordinates, float **colors,
    unsigned int *elementCount, unsigned int **elements)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	VuoMesh_download(m);
	if (vertexCount)
		*vertexCount = m->vertexCount;
	if (positions)
		*positions = m->positions;
	if (normals)
		*normals = m->normals;
	if (textureCoordinates)
		*textureCoordinates = m->textureCoordinates;
	if (colors)
		*colors = m->colors;
	if (elementCount)
		*elementCount = m->elementCount;
	if (elements)
		*elements = m->elements;
}

/**
 * Outputs OpenGL vertex and element buffer information, uploading the data to the GPU if needed.
 *
 * You may pass NULL to any of the output variables.
 *
 * @version200New
 */
void VuoMesh_getGPUBuffers(const VuoMesh mesh, unsigned int *vertexCount,
    unsigned int *combinedBuffer,
    void **normalOffset, void **textureCoordinateOffset, void **colorOffset,
    unsigned int *elementCount, unsigned int *elementBuffer)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	VuoMesh_upload(m);
	if (vertexCount)
		*vertexCount = m->vertexCount;
	if (combinedBuffer)
		*combinedBuffer = m->glUpload.combinedBuffer;
	if (normalOffset)
		*normalOffset = m->glUpload.normalOffset;
	if (textureCoordinateOffset)
		*textureCoordinateOffset = m->glUpload.textureCoordinateOffset;
	if (colorOffset)
		*colorOffset = m->glUpload.colorOffset;
	if (elementCount)
		*elementCount = m->elementCount;
	if (elementBuffer)
		*elementBuffer = m->glUpload.elementBuffer;
}

/**
 * Returns the mesh's element assembly method.
 *
 * @version200New
 */
VuoMesh_ElementAssemblyMethod VuoMesh_getElementAssemblyMethod(const VuoMesh mesh)
{
	if (!mesh)
		return VuoMesh_IndividualTriangles;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	return m->elementAssemblyMethod;
}

/**
 * Returns the mesh's face culling mode.
 *
 * @version200New
 */
VuoMesh_FaceCulling VuoMesh_getFaceCulling(const VuoMesh mesh)
{
	if (!mesh)
		return VuoMesh_CullNone;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	return m->faceCulling;
}

/**
 * Returns the OpenGL enum value for the mesh's face culling mode.
 *
 * @version200New
 */
unsigned int VuoMesh_getFaceCullingGL(const VuoMesh mesh)
{
	if (!mesh)
		return GL_NONE;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	if (m->faceCulling == VuoMesh_CullBackfaces)
		return GL_BACK;
	else if (m->faceCulling == VuoMesh_CullFrontfaces)
		return GL_FRONT;
	else // if (m->faceCulling == VuoMesh_CullNone)
		return GL_NONE;
}

/**
 * Returns the size, in Vuo Coordinates, at which lines and points should be rendered.
 *
 * @version200New
 */
VuoReal VuoMesh_getPrimitiveSize(const VuoMesh mesh)
{
	if (!mesh)
		return 0;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	return m->primitiveSize;
}

/**
 * Returns the size, in bytes, of the GPU element buffer.
 *
 * @version200New
 */
unsigned int VuoMesh_getElementBufferSize(const VuoMesh mesh)
{
	if (!mesh)
		return 0;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	return m->glUpload.elementBufferSize;
}

/**
 * Changes the mesh's CPU buffers, and uploads them to the GPU.
 *
 * The mesh takes ownership of the buffers, and will `free()` them when finished.
 *
 * @see VuoMesh_makeFromCPUBuffers
 * @version200New
 */
void VuoMesh_setCPUBuffers(VuoMesh mesh, unsigned int vertexCount,
    float *positions, float *normals, float *textureCoordinates, float *colors,
    unsigned int elementCount, unsigned int *elements)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	m->vertexCount = vertexCount;

	if (m->positions != positions)
	{
		free(m->positions);
		m->positions = positions;
	}

	if (m->normals != normals)
	{
		free(m->normals);
		m->normals = normals;
	}

	if (m->textureCoordinates != textureCoordinates)
	{
		free(m->textureCoordinates);
		m->textureCoordinates = textureCoordinates;
	}

	if (m->colors != colors)
	{
		free(m->colors);
		m->colors = colors;
	}

	m->elementCount = elementCount;

	if (m->elements != elements)
	{
		free(m->elements);
		m->elements = elements;
	}

	VuoGlPool_release(VuoGlPool_ArrayBuffer, m->glUpload.combinedBufferSize, m->glUpload.combinedBuffer);
	VuoGlPool_release(VuoGlPool_ElementArrayBuffer, m->glUpload.elementBufferSize, m->glUpload.elementBuffer);
	m->glUpload.combinedBufferSize = 0;
	m->glUpload.combinedBuffer = 0;
	m->glUpload.elementBufferSize = 0;
	m->glUpload.elementBuffer = 0;

	VuoMesh_upload(m);
}

/**
 * Changes the mesh's face culling mode.
 *
 * @version200New
 */
void VuoMesh_setFaceCulling(VuoMesh mesh, VuoMesh_FaceCulling faceCulling)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	m->faceCulling = faceCulling;
}

/**
 * Changes the size, in Vuo Coordinates, at which lines and points should be rendered.
 *
 * @version200New
 */
void VuoMesh_setPrimitiveSize(VuoMesh mesh, VuoReal primitiveSize)
{
	if (!mesh)
		return;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	m->primitiveSize = primitiveSize;
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
}

/**
 * Copies element data from the GPU back to the CPU.
 */
static void VuoMesh_download(VuoMesh_internal *m)
{
	if (!m->glUpload.combinedBuffer
	 || (m->positions
			&& (m->normals || !m->glUpload.normalOffset)
			&& (m->textureCoordinates || !m->glUpload.textureCoordinateOffset)
			&& (m->colors || !m->glUpload.colorOffset)
			&& (m->elements || !m->glUpload.elementBuffer)))
		return;

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		glBindBuffer(GL_ARRAY_BUFFER, m->glUpload.combinedBuffer);

		// @@@ is this necessary?
//		glFlush();

		float *vertexData = (float *)malloc(m->glUpload.combinedBufferSize);
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, m->glUpload.combinedBufferSize, vertexData);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (!m->positions)
		{
			m->positions = (float *)malloc(sizeof(float) * 3 * m->vertexCount);
			memcpy(m->positions, vertexData, sizeof(float) * 3 * m->vertexCount);
		}

		if (!m->normals && m->glUpload.normalOffset)
		{
			m->normals = (float *)malloc(sizeof(float) * 3 * m->vertexCount);
			memcpy(m->normals, (char *)vertexData + (int)m->glUpload.normalOffset, sizeof(float) * 3 * m->vertexCount);
		}

		if (!m->textureCoordinates && m->glUpload.textureCoordinateOffset)
		{
			m->textureCoordinates = (float *)malloc(sizeof(float) * 2 * m->vertexCount);
			memcpy(m->textureCoordinates, (char *)vertexData + (int)m->glUpload.textureCoordinateOffset, sizeof(float) * 2 * m->vertexCount);
		}

		if (!m->colors && m->glUpload.colorOffset)
		{
			m->colors = (float *)malloc(sizeof(float) * 4 * m->vertexCount);
			memcpy(m->colors, (char *)vertexData + (int)m->glUpload.colorOffset, sizeof(float) * 4 * m->vertexCount);
		}

		free(vertexData);

		if (!m->elements && m->glUpload.elementBuffer)
		{
			m->elements = malloc(m->glUpload.elementBufferSize);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->glUpload.elementBuffer);
			glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m->glUpload.elementBufferSize, m->elements);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	});
}

/**
 * Finds the mesh's center and axis-aligned extents, taking into account the passed transform.
 */
VuoBox VuoMesh_bounds(const VuoMesh mesh, float matrix[16])
{
	if (!mesh)
		return VuoBox_make((VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0});

	VuoPoint3d min, max;
	bool init = false;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	unsigned int vertexCount = m->vertexCount;

	VuoMesh_download(m);

		if(vertexCount > 0 && !init)
		{
			min = max = VuoTransform_transformPoint((float*)matrix, VuoPoint3d_makeFromArray(&m->positions[0]));
			init = true;
		}

		for(int n = 0; n < vertexCount; n++)
		{
			VuoPoint3d p = VuoTransform_transformPoint((float*)matrix, VuoPoint3d_makeFromArray(&m->positions[n * 3]));

			min.x = MIN(p.x, min.x);
			min.y = MIN(p.y, min.y);
			min.z = MIN(p.z, min.z);

			max.x = MAX(p.x, max.x);
			max.y = MAX(p.y, max.y);
			max.z = MAX(p.z, max.z);
		}

//		VLog("%fx%fx%f @ %f,%f,%f",bounds->size.x,bounds->size.y,bounds->size.z,bounds->center.x,bounds->center.y,bounds->center.z);


	if(init)
		return VuoBox_make((min + max) / (VuoPoint3d)(2.), max - min);
	else
		return VuoBox_make( (VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0} );
}

/**
 * Returns true if the mesh has any vertices.
 */
bool VuoMesh_isPopulated(const VuoMesh mesh)
{
	if (!mesh)
		return false;

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;
	return m->vertexCount > 0;
}

/**
 * Callback to pass to `json_object_set_userdata()`.
 */
static void VuoMesh_releaseCallback(struct json_object *js, void *mesh)
{
	VuoRelease(mesh);
}

/**
 * @ingroup VuoMesh
 * Decodes the JSON object @c js to create a new value.
 *
 * @param js A JSON array of elements of the format parsed by VuoSubmesh_makeFromJson().
 */
VuoMesh VuoMesh_makeFromJson(json_object * js)
{
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "pointer", &o))
	{
		VuoMesh mesh = (VuoMesh)json_object_get_int64(o);
		json_object_set_userdata(js, (void *)mesh, VuoMesh_releaseCallback);
		return mesh;
	}

	return NULL;
}

/**
 * @ingroup VuoMesh
 * Encodes @c value as a JSON object.
 */
json_object * VuoMesh_getJson(const VuoMesh value)
{
	if (!value)
		return NULL;

	VuoRetain(value);

	json_object *js = json_object_new_object();
	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value));
	return js;
}

/**
 * @ingroup VuoMesh
 * Calls VuoMesh_getJson(). Interprocess support is not yet implemented.
 */
json_object * VuoMesh_getInterprocessJson(const VuoMesh value)
{
	return VuoMesh_getJson(value);
}

/**
 * @ingroup VuoMesh
 * Returns a brief summary of the contents of this mesh.
 */
char *VuoMesh_getSummary(const VuoMesh mesh)
{
	if (!mesh)
		return strdup("Empty mesh");

	VuoMesh_internal *m = (VuoMesh_internal *)mesh;

	if (!m->vertexCount)
		return strdup("Mesh without any vertices");

	unsigned long objectCount = VuoMesh_getSplitPrimitiveCount(mesh);
	const char * objectString = "";
	const char * assemblyMethod = " (unknown element assembly method)";
	if (m->elementAssemblyMethod == VuoMesh_IndividualTriangles)
	{
		assemblyMethod = ", ";
		objectString = "triangle";
		/// @todo Report if value.elementCount isn't a multiple of 3.
	}
	else if (m->elementAssemblyMethod == VuoMesh_TriangleStrip)
	{
		assemblyMethod = " in a strip of ";
		objectString = "triangle";
	}
	else if (m->elementAssemblyMethod == VuoMesh_TriangleFan)
	{
		assemblyMethod = " in a fan of ";
		objectString = "triangle";
	}
	else if (m->elementAssemblyMethod == VuoMesh_IndividualLines)
	{
		assemblyMethod = ", ";
		objectString = "line";
	}
	else if (m->elementAssemblyMethod == VuoMesh_LineStrip)
	{
		assemblyMethod = " in a strip of ";
		objectString = "line";
	}
	else if (m->elementAssemblyMethod == VuoMesh_Points)
	{
		assemblyMethod = ", ";
		objectString = "point";
	}

	const char * vertexCountString = m->vertexCount==1 ? "vertex" : "vertices";
	const char * objectStringPlural = objectCount==1 ? "" : "s";

	char *firstPosition = NULL;
	if (m->positions)
		firstPosition = VuoText_format("<div>with first position (%s)</div>", VuoPoint3d_getSummary(VuoPoint3d_makeFromArray(&m->positions[0])));

	char *summary = VuoText_format("<div>%u %s%s%lu %s%s</div>%s<div>%s positions</div><div>%s normals</div><div>%s texture coordinates</div><div>%s vertex colors</div>",
						  m->vertexCount, vertexCountString, assemblyMethod, objectCount, objectString, objectStringPlural,
						  firstPosition ? firstPosition : "",
						  (m->positions          || m->glUpload.combinedBuffer         ) ? "✓" : "◻",
						  (m->normals            || m->glUpload.normalOffset           ) ? "✓" : "◻",
						  (m->textureCoordinates || m->glUpload.textureCoordinateOffset) ? "✓" : "◻",
						  (m->colors             || m->glUpload.colorOffset            ) ? "✓" : "◻");

	free(firstPosition);
	return summary;
}
