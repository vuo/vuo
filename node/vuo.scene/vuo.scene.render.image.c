/**
 * @file
 * vuo.scene.render.image node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoSceneRenderer.h"
#include "VuoMultisample.h"

#include <stdio.h>
#include <string.h>
#include <math.h>


VuoModuleMetadata({
					 "title" : "Render Scene to Image",
					 "keywords" : [ "draw", "object", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "RippleImageOfSphere.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoSceneRenderer *sceneRenderer;
	VuoGlContext glContext;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));

	context->glContext = VuoGlContext_use();

	context->sceneRenderer = VuoSceneRenderer_make(context->glContext, 1);
	VuoRetain(context->sceneRenderer);

	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoInteger, {"default":1024, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) width,
		VuoInputData(VuoInteger, {"default":768, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) height,
		VuoInputData(VuoImageColorDepth, {"default":"8bpc"}) colorDepth,
		VuoInputData(VuoMultisample, {"default":"off"}) multisampling,
		VuoInputData(VuoText) cameraName,
		VuoOutputData(VuoImage) image,
		VuoOutputData(VuoImage) depthImage
)
{
	VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), objects);

	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
	VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName, true);
	VuoSceneRenderer_regenerateProjectionMatrix((*context)->sceneRenderer, width, height);
	VuoSceneRenderer_renderToImage((*context)->sceneRenderer, image, colorDepth, multisampling, depthImage);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->sceneRenderer);
	VuoGlContext_disuse((*context)->glContext);
}
