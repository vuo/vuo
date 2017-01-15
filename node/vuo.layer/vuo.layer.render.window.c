/**
 * @file
 * vuo.layer.render.window node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"
#include "VuoLayer.h"
#include "VuoList_VuoLayer.h"
#include "VuoRenderedLayers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Layers to Window",
					 "keywords" : [ "draw", "graphics", "display", "view", "screen", "full screen", "fullscreen" ],
					 "version" : "2.3.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayImagesOnLayers.vuo", "RotateGears.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
	dispatch_queue_t sceneRendererQueue;
};

void vuo_layer_render_window_init(VuoGlContext glContext, float backingScaleFactor, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
	VuoRetain(context->sceneRenderer);
}

void vuo_layer_render_window_updateBacking(VuoGlContext glContext, void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	bool valid = false;
	VuoSceneObject so = VuoSceneRenderer_getRootSceneObject(context->sceneRenderer, &valid);
	if (valid)
		VuoSceneObject_retain(so);

	dispatch_sync(context->sceneRendererQueue, ^{
	VuoRelease(context->sceneRenderer);

	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
	VuoRetain(context->sceneRenderer);
				  });
	if (valid)
	{
		VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, so);
		VuoSceneObject_release(so);
	}
}

void vuo_layer_render_window_resize(VuoGlContext glContext, void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_layer_render_window_draw(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glClearColor(0,0,0,0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	VuoSceneRenderer_draw(context->sceneRenderer);
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = NULL;
	context->sceneRendererQueue = dispatch_queue_create("org.vuo.scene.render.window.sceneRenderer", NULL);

	context->window = VuoWindowOpenGl_make(
				false,
				vuo_layer_render_window_init,
				vuo_layer_render_window_updateBacking,
				vuo_layer_render_window_resize,
				vuo_layer_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

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
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoList_VuoWindowProperty) setWindowProperties,
		VuoInputEvent({"eventBlocking":"none","data":"setWindowProperties"}) setWindowPropertiesEvent,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal, {"eventThrottling":"drop"}),
		VuoOutputData(VuoRenderedLayers) renderedLayers
)
{
	if (setWindowPropertiesEvent)
		VuoWindowOpenGl_setProperties((*context)->window, setWindowProperties);

	VuoSceneObject rootSceneObject = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity()).sceneObject;

	dispatch_sync((*context)->sceneRendererQueue, ^{
	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
				  });

	// Schedule a redraw.
	VuoWindowOpenGl_redraw((*context)->window);

	VuoInteger width, height;
	float backingScaleFactor;
	VuoWindowReference_getContentSize(VuoWindowReference_make((*context)->window), &width, &height, &backingScaleFactor);
	*renderedLayers = VuoRenderedLayers_makeWithWindow(rootSceneObject, width, height, backingScaleFactor, VuoWindowReference_make((*context)->window));
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
	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoRelease((*context)->sceneRenderer);
											 });
	VuoRelease((*context)->window);
	dispatch_release((*context)->sceneRendererQueue);
}
