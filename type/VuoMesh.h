/**
 * @file
 * VuoMesh C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMESH_H
#define VUOMESH_H

#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
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
 * One set of vertices, with associated normals and other per-vertex data, within a mesh.
 */
typedef struct
{
	unsigned int vertexCount; ///< Number of vertices in @c positions, @c normals, @c tangents, @c bitangents, and @c textureCoordinates.
	/// @todo Change to `VuoDictionary_VuoPoint4D vertexAttributes;` once dictionaries exist: a string-keyed list of equal-size arrays of points (vertex attributes).  Typically contains at least 'positions', 'normals', and 'textureCoordinates'.
	VuoPoint4d * positions; ///< XYZW vertex positions
	VuoPoint4d * normals; ///< Vertex normals.  May be @c NULL.
	VuoPoint4d * tangents; ///< Vertex tangents.  May be @c NULL.
	VuoPoint4d * bitangents; ///< Vertex bitangents.  May be @c NULL.
	VuoPoint4d * textureCoordinates; ///< STRQ texture coordinates.  May be @c NULL.

	unsigned int elementCount; ///< Number of elements in @c elements.
	/**
	 * An array of size elementCount of integer elements (triangle indices) which are indexes into @c positions.
	 * (Requires conversion to i16 for OpenGL ES unless GL_OES_element_index_uint.)
	 */
	unsigned int * elements;

	/// The way in which the @c elements array should be interpreted during rasterization.
	VuoMesh_ElementAssemblyMethod elementAssemblyMethod;

	/**
	 * For lines, the width (in scene units).
	 * For points, the width and height (in scene units).
	 */
	VuoReal primitiveSize;

	/// GL_BACK (default), GL_NONE, or GL_FRONT
	unsigned int faceCullingMode;

	/**
	 * References to mesh data uploaded to the GPU.
	 */
	struct
	{
		unsigned int combinedBuffer;
		unsigned int combinedBufferSize;

		void *normalOffset;
		void *tangentOffset;
		void *bitangentOffset;
		void *textureCoordinateOffset;

		unsigned int elementBuffer;
		unsigned int elementBufferSize;
	} glUpload;
} VuoSubmesh;

/**
 * A 3D mesh that contains one or more submeshes (allowing each submesh to have a different element assembly method).
 */
typedef struct _VuoMesh
{
	unsigned int submeshCount;  ///< Number of items in @c submeshes.
	VuoSubmesh *submeshes;  ///< The submeshes that together define the shape of the mesh.  All VuoSubmeshes are assumed to have the same primitive type (points, lines, triangles), but the assembly can vary (e.g., a single mesh can contain submeshes with VuoMesh_IndividualLines and VuoMesh_LineStrip).
} *VuoMesh;

VuoSubmesh VuoSubmesh_make(unsigned int vertexCount, unsigned int elementCount);
VuoSubmesh VuoSubmesh_makeGl(unsigned int vertexCount, unsigned int combinedBuffer, unsigned int combinedBufferSize, void *normalOffset, void *tangentOffset, void *bitangentOffset, void *textureCoordinateOffset, unsigned int elementCount, unsigned int elementBuffer, unsigned int elementBufferSize, VuoMesh_ElementAssemblyMethod elementAssemblyMethod);
unsigned long VuoSubmesh_getGlMode(VuoSubmesh submesh);
unsigned long VuoSubmesh_getSplitPrimitiveCount(VuoSubmesh submesh);
unsigned long VuoSubmesh_getSplitVertexCount(VuoSubmesh submesh);

VuoMesh VuoMesh_make(unsigned int itemCount);
void VuoMesh_upload(VuoMesh mesh);
VuoMesh VuoMesh_makeFromSingleSubmesh(VuoSubmesh submesh);
VuoMesh VuoMesh_makeQuad(void);
VuoMesh VuoMesh_makeQuadWithoutNormals(void);
VuoMesh VuoMesh_makeEquilateralTriangle(void);
VuoMesh VuoMesh_make_VuoPoint2d(VuoList_VuoPoint2d positions, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize);
VuoMesh VuoMesh_make_VuoPoint3d(VuoList_VuoPoint3d positions, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, VuoReal primitiveSize);

VuoMesh VuoMesh_copy(const VuoMesh mesh);
const char *VuoMesh_cStringForElementAssemblyMethod(VuoMesh_ElementAssemblyMethod elementAssemblyMethod);
VuoBox VuoMesh_bounds(const VuoMesh mesh, float matrix[16]);
bool VuoMesh_isPopulated(const VuoMesh mesh);

VuoMesh VuoMesh_makeFromJson(struct json_object * js);
struct json_object * VuoMesh_getJson(const VuoMesh value);
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
VuoMesh VuoMesh_makeFromString(const char *str);
char * VuoMesh_getString(const VuoMesh value);
void VuoMesh_retain(const VuoMesh v);
void VuoMesh_release(const VuoMesh v);
///@}

/**
 * @}
 */

#endif
