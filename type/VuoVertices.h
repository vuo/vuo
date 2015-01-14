/**
 * @file
 * VuoVertices C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VuoVertices_H
#define VuoVertices_H

#include "VuoPoint4d.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoVertices VuoVertices
 * Vertices representing a 3D object.
 *
 * @{
 */

/**
 * The way in which an @c elements array should be interpreted during rasterization.
 */
typedef enum {
	VuoVertices_IndividualTriangles, ///< GL_TRIANGLES, requires `elementCount % 3 == 0`.
	VuoVertices_TriangleStrip, ///< GL_TRIANGLE_STRIP, requires `elementCount >= 3`.
	VuoVertices_TriangleFan ///< GL_TRIANGLE_FAN, requires `elementCount >= 3`.
} VuoVertices_ElementAssemblyMethod;

/**
 * A 3d mesh.
 */
typedef struct
{
	unsigned int vertexCount; ///< Number of vertices in @c positions, @c normals, @c tangents, @c bitangents, and @c textureCoordinates.
	/// @todo Change to `VuoDictionary_VuoPoint4D vertexAttributes;` once dictionaries exist: a string-keyed list of equal-size arrays of points (vertex attributes).  Typically contains at least 'positions', 'normals', and 'textureCoordinates'.
	VuoPoint4d * positions; ///< XYZW vertex positions
	VuoPoint4d * normals; ///< Vertex normals
	VuoPoint4d * tangents; ///< Vertex tangents
	VuoPoint4d * bitangents; ///< Vertex bitangents
	VuoPoint4d * textureCoordinates; ///< STRQ texture coordinates

	unsigned int elementCount; ///< Number of elements in @c elements.
	/**
	 * An array of size elementCount of integer elements (triangle indices) which are indexes into @c positions.
	 * (Requires conversion to i16 for OpenGL ES unless GL_OES_element_index_uint.)
	 */
	unsigned int * elements;

	/// The way in which the @c elements array should be interpreted during rasterization.
	VuoVertices_ElementAssemblyMethod elementAssemblyMethod;
} VuoVertices;

VuoVertices VuoVertices_alloc(unsigned int vertexCount, unsigned int elementCount);
VuoVertices VuoVertices_getQuad(void);
VuoVertices VuoVertices_getEquilateralTriangle(void);

VuoVertices VuoVertices_valueFromJson(struct json_object * js);
struct json_object * VuoVertices_jsonFromValue(const VuoVertices value);
char * VuoVertices_summaryFromValue(const VuoVertices value);

///@{
/**
 * Automatically generated function.
 */
VuoVertices VuoVertices_valueFromString(const char *str);
char * VuoVertices_stringFromValue(const VuoVertices value);
void VuoVertices_retain(const VuoVertices v);
void VuoVertices_release(const VuoVertices v);
///@}

/**
 * @}
 */

#endif
