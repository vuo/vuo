/**
 * @file
 * vuo.image.feedback node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoSceneRenderer.h"

VuoModuleMetadata({
					 "title" : "Blend Image with Feedback",
					 "keywords" : [ "combine", "mix", "fade", "merge", "layer", "composite", "feedback" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoLayer",
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "DisplayImageFeedback.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoSceneRenderer *sceneRenderer;
	VuoImage previousFeedbackImage;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->previousFeedbackImage = NULL;
	instance->glContext = VuoGlContext_use();
	instance->sceneRenderer = VuoSceneRenderer_make(instance->glContext, 1);
	VuoRetain(instance->sceneRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoTransform2d) imageTransform,
		VuoInputData(VuoTransform2d) feedbackTransform,
		VuoInputData(VuoReal, {"default":0.7,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.01}) feedbackOpacity,
		VuoInputData(VuoBoolean, {"default":false}) pixelAligned,
		VuoInputData(VuoBoolean, {"default":true, "name":"Image in Foreground"}) imageInForeground,
		VuoInputData(VuoInteger, {"default":1024, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) width,
		VuoInputData(VuoInteger, {"default":768, "suggestedMin":1, "suggestedMax":4096, "suggestedStep":256}) height,
		VuoInputData(VuoImageColorDepth, {"default":"8bpc"}) colorDepth,
		VuoOutputData(VuoImage) feedbackImage
)
{
	if (!image && !(*instance)->previousFeedbackImage)
	{
		*feedbackImage = NULL;
		return;
	}

	// Convert the new image into a layer.
	VuoImage newImage = (image ? image : VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,0,0,0),1,1));
	VuoLayer newImageLayer = VuoLayer_make(VuoText_make("New Image"),
												   newImage,
												   (VuoPoint2d){0,0},
												   0,
												   (2.*newImage->pixelsWide)/width,
												   1.);
	VuoList_VuoLayer newImageLayerList = VuoListCreate_VuoLayer();
	VuoListAppendValue_VuoLayer(newImageLayerList, newImageLayer);

	// Convert the previous feedback image into a layer.
	VuoImage previousFeedbackImage = ((*instance)->previousFeedbackImage ? (*instance)->previousFeedbackImage : VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,0,0,0),1,1));
	VuoLayer previousFeedbackLayer = VuoLayer_make(VuoText_make("Feedback Image"),
												   previousFeedbackImage,
												   (VuoPoint2d){0,0},
												   0,
												   (2.*previousFeedbackImage->pixelsWide)/width,
												   1. - pow(1. - feedbackOpacity, 3.));
	VuoList_VuoLayer previousFeedbackLayerList = VuoListCreate_VuoLayer();
	VuoListAppendValue_VuoLayer(previousFeedbackLayerList, previousFeedbackLayer);

	// Apply the requested transforms to the new layer and to the previous feedback layer,
	// respectively, then combine the two layers.
	VuoList_VuoLayer compositeLayerList = VuoListCreate_VuoLayer();

	VuoTransform2d feedbackTransform2 = pixelAligned
			? VuoTransform2d_make(
				  VuoPoint2d_snap(feedbackTransform.translation, VuoPoint2d_make(0,0), VuoPoint2d_make(2./width,2./width)),
				  VuoReal_snap(feedbackTransform.rotation, 0, M_PI/2),
				  VuoPoint2d_make(1,1))
			: feedbackTransform;

	if (imageInForeground)
	{
		VuoListAppendValue_VuoLayer(compositeLayerList, VuoLayer_makeGroup(previousFeedbackLayerList, feedbackTransform2));
		VuoListAppendValue_VuoLayer(compositeLayerList, VuoLayer_makeGroup(newImageLayerList, imageTransform));
	}
	else
	{
		VuoListAppendValue_VuoLayer(compositeLayerList, VuoLayer_makeGroup(newImageLayerList, imageTransform));
		VuoListAppendValue_VuoLayer(compositeLayerList, VuoLayer_makeGroup(previousFeedbackLayerList, feedbackTransform2));
	}

	VuoSceneObject rootSceneObject = VuoLayer_makeGroup(compositeLayerList, VuoTransform2d_makeIdentity()).sceneObject;
	VuoSceneRenderer_setRootSceneObject((*instance)->sceneRenderer, rootSceneObject);
	VuoSceneRenderer_regenerateProjectionMatrix((*instance)->sceneRenderer, width, height);

	// Render the composite scene.
	VuoSceneRenderer_renderToImage((*instance)->sceneRenderer, feedbackImage, colorDepth, VuoMultisample_Off, NULL);

	// Clean up.
	if ((*instance)->previousFeedbackImage)
		VuoRelease((*instance)->previousFeedbackImage);

	(*instance)->previousFeedbackImage = *feedbackImage;
	VuoRetain((*instance)->previousFeedbackImage);

	VuoRetain(newImageLayerList);
	VuoRelease(newImageLayerList);

	VuoRetain(previousFeedbackLayerList);
	VuoRelease(previousFeedbackLayerList);

	VuoRetain(compositeLayerList);
	VuoRelease(compositeLayerList);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->sceneRenderer);

	if ((*instance)->previousFeedbackImage)
		VuoRelease((*instance)->previousFeedbackImage);

	VuoGlContext_disuse((*instance)->glContext);
}
