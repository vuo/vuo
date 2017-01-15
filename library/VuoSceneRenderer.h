/**
 * @file
 * VuoSceneRenderer interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoGlContext.h"
#include "VuoMultisample.h"

/**
 * An object for rendering a scenegraph.
 */
typedef void * VuoSceneRenderer;

VuoSceneRenderer VuoSceneRenderer_make(VuoGlContext glContext, float backingScaleFactor);
void VuoSceneRenderer_regenerateProjectionMatrix(VuoSceneRenderer sceneRenderer, unsigned int width, unsigned int height);

void VuoSceneRenderer_draw(VuoSceneRenderer sceneRenderer);
void VuoSceneRenderer_renderToImage(VuoSceneRenderer sceneRenderer, VuoImage *image, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, VuoImage *depthImage);

void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sceneRenderer, VuoSceneObject rootSceneObject);
VuoSceneObject VuoSceneRenderer_getRootSceneObject(VuoSceneRenderer sceneRenderer, bool *isValid);
void VuoSceneRenderer_setCameraName(VuoSceneRenderer sceneRenderer, VuoText cameraName, VuoBoolean useLeftCamera);

extern dispatch_semaphore_t VuoSceneRenderer_vertexArraySemaphore;

#ifdef __cplusplus
}
#endif
