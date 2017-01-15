/**
 * @file
 * vuo.scene.render.window node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneRenderer.h"
#include "VuoWindow.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Scene to Window",
					 "keywords" : [ "draw", "graphics", "display", "view", "object", "screen", "full screen", "fullscreen" ],
					 "version" : "2.2.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayScene.vuo", "DisplaySquare.vuo", "DisplaySphere.vuo", "MoveSpinningSphere.vuo", "SwitchCameras.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
	dispatch_queue_t sceneRendererQueue;
};

void vuo_scene_render_window_init(VuoGlContext glContext, float backingScaleFactor, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
	VuoRetain(context->sceneRenderer);
}

void vuo_scene_render_window_updateBacking(VuoGlContext glContext, void *ctx, float backingScaleFactor)
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
	if (valid)
	{
		VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, so);
		VuoSceneObject_release(so);
	}
				  });
}

void vuo_scene_render_window_resize(VuoGlContext glContext, void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_scene_render_window_draw(VuoGlContext glContext, void *ctx)
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
				true,
				vuo_scene_render_window_init,
				vuo_scene_render_window_updateBacking,
				vuo_scene_render_window_resize,
				vuo_scene_render_window_draw,
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
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoText) cameraName,
		VuoInputData(VuoList_VuoWindowProperty) setWindowProperties,
		VuoInputEvent({"eventBlocking":"none", "data":"setWindowProperties"}) setWindowPropertiesEvent
)
{
	if (setWindowPropertiesEvent)
		VuoWindowOpenGl_setProperties((*context)->window, setWindowProperties);

	VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), objects);

	dispatch_sync((*context)->sceneRendererQueue, ^{
	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);

	VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName, true);
				  });

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
	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoRelease((*context)->sceneRenderer);
											 });
	VuoRelease((*context)->window);
	dispatch_release((*context)->sceneRendererQueue);
}
