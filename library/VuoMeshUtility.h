/**
 * @file
 * VuoMeshUtility interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoMesh.h"
#include "VuoInteger.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoList_VuoPoint4d.h"
#include "VuoList_VuoInteger.h"
	
/**
 * Generate normals for this submesh.
 */
void VuoMeshUtility_calculateNormals(VuoSubmesh* submesh);
	
/**
 * Generate tangents and bitangents for this submesh.
 */
void VuoMeshUtility_calculateTangents(VuoSubmesh* submesh);

/**
 * Generate spherical UVs for this submesh.
 */
void VuoMeshUtility_calculateSphericalUVs(VuoSubmesh* submesh);

/**
 * Generate cubic UVs for this submesh.  Works best with IndividualTriangles meshes where no vertex is shared between triangles.
 */
void VuoMeshUtility_calculateCubicUVs(VuoSubmesh* submesh);

/**
 * Generate cubic UVs for this submesh, using the triangle normal to project UV instead of vertex normal.
 */
void VuoMeshUtility_calculateCubicUVsPerTriangle(VuoSubmesh* submesh);
/**
 * Inserts a seam along a vertical line on the left side of a mesh.
 */
void VuoMeshUtility_insertSeam(VuoSubmesh* submesh);

/**
 * Remove unused vertices in a mesh.  This should usually be called after a seam
 * is inserted.
 */
void VuoMeshUtility_removeUnusedVertices(VuoSubmesh* mesh);

/**
 * Calculates the face normal for position vertices @c a, @c b, and @c c.
 */
static inline VuoPoint4d VuoMeshUtility_faceNormal(VuoPoint4d a, VuoPoint4d b, VuoPoint4d c)
{
	return VuoPoint4d_normalize3d(VuoPoint4d_crossProduct(VuoPoint4d_subtract(b,a), VuoPoint4d_subtract(c,a)));
}

#ifdef __cplusplus
}
#endif
