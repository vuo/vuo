/**
 * @file
 * vuo.image.render.window node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"
#include "VuoRenderedLayers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <dispatch/dispatch.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Image to Window",
					 "keywords" : [ "draw", "graphics", "display", "view", "screen", "full screen", "fullscreen" ],
					 "version" : "4.0.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
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

static void vuo_image_render_window_init(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
	VuoRetain(context->sceneRenderer);

	// Since we're speciying VuoShader_makeImageShader() which doesn't use normals, we don't need to generate them.
	VuoMesh mesh = VuoMesh_makeQuadWithoutNormals();
	context->rootSceneObject = VuoSceneObject_makeMesh(
				mesh,
				VuoShader_makeUnlitImageShader(NULL, 1),
				VuoTransform_makeIdentity());
	VuoSceneObject_retain(context->rootSceneObject);

	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);
}

static void vuo_image_render_window_updateBacking(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoRelease(context->sceneRenderer);
	context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
	VuoRetain(context->sceneRenderer);
	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
}

static void vuo_image_render_window_resize(void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;
	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
}

static VuoIoSurface vuo_image_render_window_draw(void *ctx)
{
	struct nodeInstanceData *context = ctx;
	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoIoSurface vis = VuoSceneRenderer_renderToIOSurface(context->sceneRenderer, VuoImageColorDepth_8, VuoMultisample_Off, false);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
	return vis;
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = NULL;

	context->scenegraphSemaphore = dispatch_semaphore_create(1);

	context->window = VuoWindowOpenGl_make(
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
		VuoOutputTrigger(updatedWindow, VuoRenderedLayers)
)
{
	VuoWindowOpenGl_enableTriggers((*context)->window, updatedWindow);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoWindowDescription) setWindowDescription,
		VuoInputEvent({"eventBlocking":"none","data":"setWindowDescription"}) setWindowDescriptionEvent,
		VuoOutputTrigger(updatedWindow, VuoRenderedLayers)
)
{
	if (setWindowDescriptionEvent)
	{
		VuoList_VuoWindowProperty windowProperties = VuoWindowDescription_getWindowProperties(setWindowDescription);
		unsigned int windowPropertyCount = VuoListGetCount_VuoWindowProperty(windowProperties);
		for (unsigned int i = 0; i < windowPropertyCount; ++i)
		{
			VuoWindowPropertyType type = VuoListGetValue_VuoWindowProperty(windowProperties, i).type;
			if (type == VuoWindowProperty_AspectRatio
			 || type == VuoWindowProperty_Size)
				(*context)->aspectRatioOverridden = true;
			else if (type == VuoWindowProperty_AspectRatioReset)
				(*context)->aspectRatioOverridden = false;
		}

		VuoWindowOpenGl_setProperties((*context)->window, windowProperties);
	}

	dispatch_semaphore_wait((*context)->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		VuoShader_setUniform_VuoImage(VuoSceneObject_getShader((*context)->rootSceneObject), "texture", image);

		if (image && image->pixelsWide && image->pixelsHigh)
		{
			VuoSceneObject_setScale((*context)->rootSceneObject, (VuoPoint3d){2, 2. * (float)image->pixelsHigh/(float)image->pixelsWide, 1.});
			if (!(*context)->aspectRatioOverridden)
				VuoWindowOpenGl_setAspectRatio((*context)->window, image->pixelsWide / image->scaleFactor, image->pixelsHigh / image->scaleFactor);
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
	struct nodeInstanceData *c = *context;

	// Ensure the context isn't deallocated until the window has finished closing
	// (which might be a while if macOS is doing its exit-fullscreen animation).
	VuoRetain(c);

	VuoWindowOpenGl_close(c->window, ^{
		VuoRelease(c->sceneRenderer);
		VuoSceneObject_release(c->rootSceneObject);
		dispatch_release(c->scenegraphSemaphore);
		VuoRelease(c);
	});

	VuoRelease(c->window);
}
