/**
 * @file
 * vuo.scene.render.window node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDisplayRefresh.h"
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
						 "VuoDisplayRefresh",
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
	VuoDisplayRefresh *displayRefresh;
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
};

void vuo_scene_render_window_init(VuoGlContext glContext, float backingScaleFactor, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(glContext, backingScaleFactor);
	VuoRetain(context->sceneRenderer);
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

	context->displayRefresh = VuoDisplayRefresh_make(context);
	VuoRetain(context->displayRefresh);

	context->window = VuoWindowOpenGl_make(
				true,
				vuo_scene_render_window_init,
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
	VuoWindowOpenGl_enableTriggers((*context)->window, showedWindow);
	VuoDisplayRefresh_enableTriggers((*context)->displayRefresh, requestedFrame, NULL);
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

	VuoWindowOpenGl_executeWithWindowContext((*context)->window, ^(VuoGlContext glContext){
												 VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
											 });

	VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName, true);

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
