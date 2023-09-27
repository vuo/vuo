/**
 * @file
 * VuoSceneObjectRenderer interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoSceneObject.h"
#include "VuoShader.h"

/**
 * Context data for applying a shader to a @ref VuoSceneObject.
 */
typedef void *VuoSceneObjectRenderer;

VuoSceneObjectRenderer VuoSceneObjectRenderer_make(VuoShader shader);

/**
 * The maximum number of vertices a @ref VuoSceneObjectRenderer_CPUGeometryOperator function can output.
 *
 * Just large enough to support `vuo.scene.spike`.
 *
 * @version200New
 */
#define VuoSceneObjectRenderer_maxOutputVertices 9

/**
 * A function that modifies a primitive (triangle, line, or point).
 * Roughly equivalent to an OpenGL Vertex Shader + Geometry Shader.
 *
 * All parameters serve as both input and output.
 * It may decrease (down to 0) or increase (up to @ref VuoSceneObjectRenderer_maxOutputVertices) the number of vertices it outputs.
 *
 * @version200New
 */
typedef void (^VuoSceneObjectRenderer_CPUGeometryOperator)(float *modelMatrix,
														   float *modelMatrixInverse,
														   int *vertexCount,
														   float *positions,
														   float *normals,
														   float *textureCoordinates,
														   float *colors);

/**
 * A function that changes the position of a single vertex.
 * (A simplified version of @ref VuoSceneObjectRenderer_CPUGeometryOperator.)
 *
 * @version200New
 */
typedef VuoPoint3d (^VuoSceneObjectRenderer_Deformer)(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate);

VuoSceneObjectRenderer_CPUGeometryOperator VuoSceneObjectRenderer_makeDeformer(VuoSceneObjectRenderer_Deformer deformer);

bool VuoSceneObjectRenderer_usingGPU(void);

VuoSceneObject VuoSceneObjectRenderer_draw(VuoSceneObjectRenderer sceneObjectRenderer, VuoSceneObject object, VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator);

#ifdef __cplusplus
}
#endif
