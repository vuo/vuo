/**
 * @file
 * VuoSceneRenderer interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoGlContext.h"

/**
 * An object for rendering a scenegraph.
 */
typedef void * VuoSceneRenderer;

VuoSceneRenderer VuoSceneRenderer_make(void);
void VuoSceneRenderer_prepareContext(VuoSceneRenderer sceneRenderer, VuoGlContext glContext);
void VuoSceneRenderer_regenerateProjectionMatrix(VuoSceneRenderer sceneRenderer, unsigned int width, unsigned int height);
void VuoSceneRenderer_draw(VuoSceneRenderer sceneRenderer, VuoGlContext glContext);
void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sceneRenderer, VuoGlContext glContext, VuoSceneObject rootSceneObject);
void VuoSceneRenderer_setCameraName(VuoSceneRenderer sceneRenderer, VuoText cameraName);
void VuoSceneRenderer_drawElement(VuoSceneRenderer sceneRenderer, VuoGlContext glContext, int element, float length);

#ifdef __cplusplus
}
#endif
