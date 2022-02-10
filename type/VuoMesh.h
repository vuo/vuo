/**
 * @file
 * VuoMesh C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoColor.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoList_VuoColor.h"
#include "VuoList_VuoPoint2d.h"
#include "VuoList_VuoPoint3d.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoMesh VuoMesh
 * Vertices representing a 3D object.
 *
 * @{
 */

/**
 * The way in which an @c elements array should be interpreted during rasterization.
 */
typedef enum {
	VuoMesh_IndividualTriangles,	///< GL_TRIANGLES, requires `elementCount % 3 == 0`.
	VuoMesh_TriangleStrip,			///< GL_TRIANGLE_STRIP, requires `elementCount >= 3`.
	VuoMesh_TriangleFan,			///< GL_TRIANGLE_FAN, requires `elementCount >= 3`.
	VuoMesh_IndividualLines,		///< GL_LINES, requires `elementCount % 2 == 0`.
	VuoMesh_LineStrip,				///< GL_LINE_STRIP, requires `elementCount >= 2`.
	VuoMesh_Points					///< GL_POINTS
} VuoMesh_ElementAssemblyMethod;

/**
 * Which triangles to render.
 */
typedef enum {
	VuoMesh_CullNone,        ///< Render all triangles.
	VuoMesh_CullBackfaces,   ///< Cull backward-facing triangles.
	VuoMesh_CullFrontfaces,  ///< Cull forward-facing triangles.
} VuoMesh_FaceCulling;

/**
 * A 3D mesh, consisting of CPU and/or GPU data
 * describing a list of elements (positions; and optionally normals, texture coordinates, and colors)
 * which represent points or are assembled into lines or triangles.
 *
 * @version200Changed{VuoSceneObject is now an opaque, heap-allocated type.
 * Please use the get/set methods instead of directly accessing the structure.}
 */
typedef const struct { void *l; } * VuoMesh;

// Constructors
VuoMesh VuoMesh_makeQuad(void);
VuoMesh VuoMesh_makeQuadWithoutNormals(void);
VuoMesh VuoMesh_makeEquilateralTriangle(void);
VuoMesh VuoMesh_makeCube(void);
VuoMesh VuoMesh_makePlane(VuoInteger columns, VuoInteger rows);
VuoMesh VuoMesh_make_VuoPoint2d(VuoList_VuoPoint2d positions, VuoList_VuoColor colors, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize);
VuoMesh VuoMesh_make_VuoPoint3d(VuoList_VuoPoint3d positions, VuoList_VuoColor colors, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize);

void VuoMesh_allocateCPUBuffers(unsigned int vertexCount,
	float **positions, float **normals, float **textureCoordinates, float **colors,
	unsigned int elementCount, unsigned int **elements);

VuoMesh VuoMesh_makeFromCPUBuffers(unsigned int vertexCount,
    float *positions, float *normals, float *textureCoordinates, float *colors,
    unsigned int elementCount, unsigned int *elements, VuoMesh_ElementAssemblyMethod elementAssemblyMethod);
VuoMesh VuoMesh_makeFromGPUBuffers(unsigned int vertexCount,
    unsigned int combinedBuffer, unsigned int combinedBufferSize,
    void *normalOffset, void *textureCoordinateOffset, void *colorOffset,
    unsigned int elementCount, unsigned int elementBuffer, unsigned int elementBufferSize, VuoMesh_ElementAssemblyMethod elementAssemblyMethod);

VuoMesh VuoMesh_copy(const VuoMesh mesh);
VuoMesh VuoMesh_copyShallow(const VuoMesh mesh);

// Mutators
void VuoMesh_removeTextureCoordinates(VuoMesh mesh);

// Getters
void VuoMesh_getCPUBuffers(const VuoMesh mesh, unsigned int *vertexCount,
	float **positions, float **normals, float **textureCoordinates, float **colors,
	unsigned int *elementCount, unsigned int **elements);
void VuoMesh_getGPUBuffers(const VuoMesh mesh, unsigned int *vertexCount,
	unsigned int *combinedBuffer,
	void **normalOffset, void **textureCoordinateOffset, void **colorOffset,
	unsigned int *elementCount, unsigned int *elementBuffer);

VuoMesh_ElementAssemblyMethod VuoMesh_getElementAssemblyMethod(const VuoMesh mesh);
VuoMesh_FaceCulling VuoMesh_getFaceCulling(const VuoMesh mesh);
unsigned int VuoMesh_getFaceCullingGL(const VuoMesh mesh);
VuoReal VuoMesh_getPrimitiveSize(const VuoMesh mesh);

unsigned long VuoMesh_getGlMode(const VuoMesh mesh);
unsigned long VuoMesh_getSplitPrimitiveCount(const VuoMesh mesh);
unsigned long VuoMesh_getSplitVertexCount(const VuoMesh mesh);
unsigned long VuoMesh_getCompleteElementCount(const VuoMesh mesh);
unsigned int VuoMesh_getElementBufferSize(const VuoMesh mesh);

// Setters
void VuoMesh_setCPUBuffers(VuoMesh mesh, unsigned int vertexCount,
    float *positions, float *normals, float *textureCoordinates, float *colors,
    unsigned int elementCount, unsigned int *elements);
void VuoMesh_setFaceCulling(VuoMesh mesh, VuoMesh_FaceCulling faceCulling);
void VuoMesh_setPrimitiveSize(VuoMesh mesh, VuoReal primitiveSize);

const char *VuoMesh_cStringForElementAssemblyMethod(VuoMesh_ElementAssemblyMethod elementAssemblyMethod);
VuoBox VuoMesh_bounds(const VuoMesh mesh, float matrix[16]);
bool VuoMesh_isPopulated(const VuoMesh mesh);

VuoMesh VuoMesh_makeFromJson(struct json_object * js);
struct json_object * VuoMesh_getJson(const VuoMesh value);

/// This type has a _getInterprocessJson() function.
#define VuoMesh_REQUIRES_INTERPROCESS_JSON
struct json_object * VuoMesh_getInterprocessJson(const VuoMesh value);

char * VuoMesh_getSummary(const VuoMesh value);

/**
 * For a given element assembly method, returns the equivalent assembly method for individual elements (i.e., after they've been expanded from strips/fans).
 */
static inline VuoMesh_ElementAssemblyMethod VuoMesh_getExpandedPrimitiveMode(const VuoMesh_ElementAssemblyMethod inputMode)
{
	if (inputMode == VuoMesh_IndividualTriangles
	 || inputMode == VuoMesh_TriangleFan
	 || inputMode == VuoMesh_TriangleStrip)
		return VuoMesh_IndividualTriangles;
	else if (inputMode == VuoMesh_IndividualLines
		  || inputMode == VuoMesh_LineStrip)
		return VuoMesh_IndividualLines;
	else
		return VuoMesh_Points;
}

///@{
/**
 * Automatically generated function.
 */
char * VuoMesh_getString(const VuoMesh value);
void VuoMesh_retain(const VuoMesh v);
void VuoMesh_release(const VuoMesh v);
///@}

/**
 * @}
 */
