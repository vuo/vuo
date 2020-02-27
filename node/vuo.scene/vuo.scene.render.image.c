/**
 * @file
 * vuo.scene.render.image node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneRenderer.h"
#include "VuoMultisample.h"

#include <stdio.h>
#include <string.h>
#include <math.h>


VuoModuleMetadata({
					 "title" : "Render Scene to Image",
					 "keywords" : [
						 "draw", "graphics",
						 "3D", "object", "opengl", "scenegraph",
						 "convert",
					 ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "RippleImageOfSphere.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoSceneRenderer *sceneRenderer;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));

	context->sceneRenderer = VuoSceneRenderer_make(1);
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
		VuoInputData(VuoMultisample, {"default":"4"}) multisampling,
		VuoInputData(VuoText) cameraName,
		VuoOutputData(VuoImage) image,
		VuoOutputData(VuoImage) depthImage
)
{
	VuoSceneObject rootSceneObject = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());

	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
	VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName, true);
	VuoSceneRenderer_regenerateProjectionMatrix((*context)->sceneRenderer, width, height);
	VuoSceneRenderer_renderToImage((*context)->sceneRenderer, image, colorDepth, multisampling, depthImage, false);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->sceneRenderer);
}
