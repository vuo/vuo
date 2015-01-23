/**
 * @file
 * vuo.layer.render.window node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDisplayRefresh.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"
#include "VuoLayer.h"
#include "VuoList_VuoLayer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Layers to Window",
					 "keywords" : [ "frame", "draw", "opengl", "graphics", "display", "view", "object",
						 "mouse", "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer"],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoDisplayRefresh",
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [
							 "DisplayImagesOnLayers.vuo",
							 "RotateGears.vuo"
						 ]
					 }
				 });


struct nodeInstanceData
{
	VuoDisplayRefresh *displayRefresh;
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
};

void vuo_layer_render_window_init(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	if (!context->sceneRenderer)
	{
		context->sceneRenderer = VuoSceneRenderer_make(glContext);
		VuoRetain(context->sceneRenderer);
	}

	VuoSceneRenderer_prepareContext(context->sceneRenderer);
}

void vuo_layer_render_window_resize(VuoGlContext glContext, void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_layer_render_window_switchContext(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *ctx)
{
//	VLog("old=%p  new=%p",oldGlContext,newGlContext);
	struct nodeInstanceData *context = ctx;
	VuoSceneRenderer_switchContext(context->sceneRenderer, newGlContext);
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

	context->displayRefresh = VuoDisplayRefresh_make(context);
	VuoRetain(context->displayRefresh);

	context->window = VuoWindowOpenGl_make(
				false,
				vuo_layer_render_window_init,
				vuo_layer_render_window_resize,
				vuo_layer_render_window_switchContext,
				vuo_layer_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(requestedFrame, VuoFrameRequest),
		VuoOutputTrigger(movedMouseTo, VuoPoint2d),
		VuoOutputTrigger(scrolledMouse, VuoPoint2d),
		VuoOutputTrigger(usedMouseButton, VuoMouseButtonAction)
)
{
	VuoWindowOpenGl_enableTriggers((*context)->window, movedMouseTo, scrolledMouse, usedMouseButton);
	VuoDisplayRefresh_enableTriggers((*context)->displayRefresh, requestedFrame, NULL);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoLayer) layers
)
{
	VuoSceneObject rootSceneObject = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity()).sceneObject;

	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
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
	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoRelease((*context)->sceneRenderer);
											 });
	VuoRelease((*context)->window);
	VuoRelease((*context)->displayRefresh);
}
