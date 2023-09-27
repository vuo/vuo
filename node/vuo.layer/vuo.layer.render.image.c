/**
 * @file
 * vuo.layer.render.image node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneRenderer.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>


VuoModuleMetadata({
					 "title" : "Render Layers to Image",
					 "keywords" : [
						 "draw", "graphics",
						 "2D",
						 "convert"
					 ],
					 "version" : "1.2.0",
					 "dependencies" : [
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "DrawRainbowTrail.vuo" ]
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
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoInteger, {"default":1024, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) width,
		VuoInputData(VuoInteger, {"default":768, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) height,
		VuoInputData(VuoImageColorDepth, {"default":"8bpc"}) colorDepth,
		VuoInputData(VuoMultisample, {"default":"off"}) multisampling,
		VuoOutputData(VuoImage) image,
		VuoOutputData(VuoRenderedLayers) renderedLayers
)
{
	VuoSceneObject rootSceneObject = (VuoSceneObject)VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());

	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
	VuoSceneRenderer_regenerateProjectionMatrix((*context)->sceneRenderer, width, height);
	VuoSceneRenderer_renderToImage((*context)->sceneRenderer, image, colorDepth, multisampling, NULL, false);

	*renderedLayers = VuoRenderedLayers_make(rootSceneObject, width, height, 1, NULL);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->sceneRenderer);
}
