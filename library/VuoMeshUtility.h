/**
 * @file
 * VuoMeshUtility interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
 * Generate normals for this mesh.
 */
void VuoMeshUtility_calculateNormals(VuoMesh mesh);
	
/**
 * Generate spherical UVs for this mesh.
 */
void VuoMeshUtility_calculateSphericalUVs(VuoMesh mesh);

/**
 * Generate cubic UVs for this mesh.  Works best with IndividualTriangles meshes where no vertex is shared between triangles.
 */
void VuoMeshUtility_calculateCubicUVs(VuoMesh mesh);

/**
 * Generate cubic UVs for this mesh, using the triangle normal to project UV instead of vertex normal.
 */
void VuoMeshUtility_calculateCubicUVsPerTriangle(VuoMesh mesh);

/**
 * Inserts a seam along a vertical line on the left side of a mesh.
 */
void VuoMeshUtility_insertSeam(VuoMesh mesh);

/**
 * Remove unused vertices in a mesh.  This should usually be called after a seam
 * is inserted.
 */
void VuoMeshUtility_removeUnusedVertices(VuoMesh mesh);

/**
 * Calculates the face normal for position vertices @c a, @c b, and @c c.
 */
static inline VuoPoint3d VuoMeshUtility_faceNormal(VuoPoint3d a, VuoPoint3d b, VuoPoint3d c)
{
	return VuoPoint3d_normalize(VuoPoint3d_crossProduct(b - a, c - a));
}

#ifdef __cplusplus
}
#endif
