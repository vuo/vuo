/**
 * @file
 * VuoSceneObjectRenderer interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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

VuoSceneObjectRenderer VuoSceneObjectRenderer_make(VuoGlContext glContext, VuoShader shader);
VuoSceneObject VuoSceneObjectRenderer_draw(VuoSceneObjectRenderer sceneObjectRenderer, VuoSceneObject object);

#ifdef __cplusplus
}
#endif
