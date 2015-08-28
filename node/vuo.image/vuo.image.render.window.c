/**
 * @file
 * vuo.image.render.window node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDisplayRefresh.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dispatch/dispatch.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Image to Window",
					 "keywords" : [ "draw", "graphics", "display", "view", "screen", "full screen", "fullscreen" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoDisplayRefresh",
						 "VuoGlContext",
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayImage.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoDisplayRefresh *displayRefresh;
	VuoWindowOpenGl *window;
	VuoSceneRenderer sceneRenderer;
	bool hasShown;
	bool aspectRatioOverridden;

	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to @c rootSceneObject.
	VuoSceneObject rootSceneObject;
};

void vuo_image_render_window_init(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext);
	VuoRetain(context->sceneRenderer);

	// Since we're speciying VuoShader_makeImageShader() which doesn't use normals, we don't need to generate them.
	VuoMesh mesh = VuoMesh_makeQuadWithoutNormals();
	context->rootSceneObject = VuoSceneObject_make(
				mesh,
				VuoShader_makeUnlitImageShader(NULL, 1),
				VuoTransform_makeIdentity(),
				VuoListCreate_VuoSceneObject()
			);
	context->rootSceneObject.transform.scale.x = 2;

	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);
}

void vuo_image_render_window_resize(VuoGlContext glContext, void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;
	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_image_render_window_draw(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	if (VuoShader_getUniform_VuoImage(context->rootSceneObject.shader, "texture"))
		VuoSceneRenderer_draw(context->sceneRenderer);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = NULL;

	context->displayRefresh = VuoDisplayRefresh_make(context);
	VuoRetain(context->displayRefresh);

	context->scenegraphSemaphore = dispatch_semaphore_create(1);

	context->window = VuoWindowOpenGl_make(
				false,
				vuo_image_render_window_init,
				vuo_image_render_window_resize,
				vuo_image_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	context->hasShown = false;
	context->aspectRatioOverridden = false;

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal, VuoPortEventThrottling_Drop)
)
{
	VuoWindowOpenGl_enableTriggers((*context)->window);
	VuoDisplayRefresh_enableTriggers((*context)->displayRefresh, requestedFrame, NULL);

	if (! (*context)->hasShown)
	{
		showedWindow( VuoWindowReference_make((*context)->window) );
		(*context)->hasShown = true;
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoList_VuoWindowProperty) windowProperties
)
{
	unsigned int windowPropertyCount = VuoListGetCount_VuoWindowProperty(windowProperties);
	for (unsigned int i = 0; i < windowPropertyCount; ++i)
	{
		VuoWindowPropertyType type = VuoListGetValueAtIndex_VuoWindowProperty(windowProperties, i).type;
		if (type == VuoWindowProperty_AspectRatio)
			(*context)->aspectRatioOverridden = true;
		else if (type == VuoWindowProperty_AspectRatioReset)
			(*context)->aspectRatioOverridden = false;
	}

	VuoWindowOpenGl_setProperties((*context)->window, windowProperties);

	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 dispatch_semaphore_wait((*context)->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

												 if (image && image->pixelsWide && image->pixelsHigh)
												 {
													 VuoShader_setUniform_VuoImage((*context)->rootSceneObject.shader, "texture", image);
													 (*context)->rootSceneObject.transform.scale.y = 2. * (float)image->pixelsHigh/(float)image->pixelsWide;
													 if (!(*context)->aspectRatioOverridden)
														 VuoWindowOpenGl_setAspectRatio((*context)->window, image->pixelsWide, image->pixelsHigh);
												 }

												 VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, (*context)->rootSceneObject);

												 dispatch_semaphore_signal((*context)->scenegraphSemaphore);
											 });

	// Schedule a redraw.
	VuoWindowOpenGl_redraw((*context)->window);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoDisplayRefresh_disableTriggers((*context)->displayRefresh);
	VuoWindowOpenGl_disableTriggers((*context)->window);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->window);
	VuoRelease((*context)->sceneRenderer);
	dispatch_release((*context)->scenegraphSemaphore);
	VuoRelease((*context)->displayRefresh);
}
