/**
 * @file
 * vuo.image.render.window node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
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
					 "version" : "3.2.1",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "SimulateFilmProjector.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer sceneRenderer;
	bool aspectRatioOverridden;

	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to @c rootSceneObject.
	VuoSceneObject rootSceneObject;
};

void vuo_image_render_window_init(VuoGlContext glContext, float backingScaleFactor, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
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
	VuoSceneObject_retain(context->rootSceneObject);

	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);
}

void vuo_image_render_window_updateBacking(VuoGlContext glContext, void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoRelease(context->sceneRenderer);
	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
	VuoRetain(context->sceneRenderer);
	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
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

	context->scenegraphSemaphore = dispatch_semaphore_create(1);

	context->window = VuoWindowOpenGl_make(
				false,
				vuo_image_render_window_init,
				vuo_image_render_window_updateBacking,
				vuo_image_render_window_resize,
				vuo_image_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	context->aspectRatioOverridden = false;

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal, {"eventThrottling":"drop"})
)
{
	VuoWindowOpenGl_enableTriggers((*context)->window, showedWindow, requestedFrame);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoList_VuoWindowProperty) setWindowProperties,
		VuoInputEvent({"eventBlocking":"none","data":"setWindowProperties"}) setWindowPropertiesEvent
)
{
	if (setWindowPropertiesEvent)
	{
		unsigned int windowPropertyCount = VuoListGetCount_VuoWindowProperty(setWindowProperties);
		for (unsigned int i = 0; i < windowPropertyCount; ++i)
		{
			VuoWindowPropertyType type = VuoListGetValue_VuoWindowProperty(setWindowProperties, i).type;
			if (type == VuoWindowProperty_AspectRatio
			 || type == VuoWindowProperty_Size)
				(*context)->aspectRatioOverridden = true;
			else if (type == VuoWindowProperty_AspectRatioReset)
				(*context)->aspectRatioOverridden = false;
		}

		VuoWindowOpenGl_setProperties((*context)->window, setWindowProperties);
	}

	dispatch_semaphore_wait((*context)->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		if (image && image->pixelsWide && image->pixelsHigh)
		{
			VuoShader_setUniform_VuoImage((*context)->rootSceneObject.shader, "texture", image);
			(*context)->rootSceneObject.transform.scale.y = 2. * (float)image->pixelsHigh/(float)image->pixelsWide;
			if (!(*context)->aspectRatioOverridden)
				VuoWindowOpenGl_setAspectRatio((*context)->window, image->pixelsWide, image->pixelsHigh);
		}

		VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, (*context)->rootSceneObject);
	}
	dispatch_semaphore_signal((*context)->scenegraphSemaphore);

	// Schedule a redraw.
	VuoWindowOpenGl_redraw((*context)->window);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowOpenGl_disableTriggers((*context)->window);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowOpenGl_close((*context)->window);
	VuoRelease((*context)->window);
	VuoRelease((*context)->sceneRenderer);
	VuoSceneObject_release((*context)->rootSceneObject);
	dispatch_release((*context)->scenegraphSemaphore);
}
