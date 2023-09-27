/**
 * @file
 * VuoSceneRenderer interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoGlPool.h"
#include "VuoImage.h"
#include "VuoMultisample.h"
#include "VuoSceneObject.h"

/**
 * An object for rendering a scenegraph.
 */
typedef void * VuoSceneRenderer;

VuoSceneRenderer VuoSceneRenderer_make(float backingScaleFactor);
void VuoSceneRenderer_regenerateProjectionMatrix(VuoSceneRenderer sceneRenderer, unsigned int width, unsigned int height);

void VuoSceneRenderer_renderToImage(VuoSceneRenderer sceneRenderer, VuoImage *image, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, VuoImage *depthImage, bool invertDepthImage);
VuoIoSurface VuoSceneRenderer_renderToIOSurface(VuoSceneRenderer sceneRenderer, VuoImageColorDepth imageColorDepth, VuoMultisample multisample, bool includeDepthBuffer);

void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sceneRenderer, VuoSceneObject rootSceneObject);
VuoSceneObject VuoSceneRenderer_getRootSceneObject(VuoSceneRenderer sceneRenderer, bool *isValid);
void VuoSceneRenderer_setCameraName(VuoSceneRenderer sceneRenderer, VuoText cameraName, VuoBoolean useLeftCamera);

#ifdef __cplusplus
}
#endif
