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
#include "VuoRenderedLayers.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Layers to Window",
					 "keywords" : [ "draw", "graphics", "display", "view", "screen", "full screen", "fullscreen" ],
					 "version" : "2.2.0",
					 "dependencies" : [
						 "VuoDisplayRefresh",
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
	VuoDisplayRefresh *displayRefresh;
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
	bool hasShown;
};

void vuo_layer_render_window_init(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext);
	VuoRetain(context->sceneRenderer);
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

	context->displayRefresh = VuoDisplayRefresh_make(context);
	VuoRetain(context->displayRefresh);

	context->window = VuoWindowOpenGl_make(
				false,
				vuo_layer_render_window_init,
				vuo_layer_render_window_resize,
				vuo_layer_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	context->hasShown = false;

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal, {"eventThrottling":"drop"})
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

	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
											 });

	// Schedule a redraw.
	VuoWindowOpenGl_redraw((*context)->window);

	VuoInteger width, height;
	VuoWindowReference_getContentSize(VuoWindowReference_make((*context)->window), &width, &height);
	*renderedLayers = VuoRenderedLayers_makeWithWindow(rootSceneObject, width, height, VuoWindowReference_make((*context)->window));
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
