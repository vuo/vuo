/**
 * @file
 * vuo.scene.render.window node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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

#include <OpenGL/CGLMacro.h>

#define DEBUG 0


VuoModuleMetadata({
					 "title" : "Render Scene to Window",
					 "keywords" : [ "frame", "draw", "opengl", "scenegraph", "graphics", "display", "view", "object",
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
							 "DisplayScene.vuo",
							 "DisplaySquare.vuo",
							 "SpinSphere.vuo",
							 "MoveSpinningSphere.vuo",
							 "SwitchCameras.vuo"
						]
					 }
				 });


struct nodeInstanceData
{
	VuoDisplayRefresh *displayRefresh;
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
};

void vuo_scene_render_window_init(VuoGlContext glContext, void *ctx)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_prepareContext(context->sceneRenderer, glContext);
}

void vuo_scene_render_window_resize(VuoGlContext glContext, void *ctx, unsigned int width, unsigned int height)
{
	//fprintf(stderr, "vuo_scene_render_window_resize(%d,%d)\n", width, height);
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_scene_render_window_draw(VuoGlContext glContext, void *ctx)
{
	//fprintf(stderr, "vuo_scene_render_window_draw\n");
	struct nodeInstanceData *context = ctx;
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glClearColor(0,0,0,0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	VuoSceneRenderer_draw(context->sceneRenderer, glContext);

#if DEBUG
	VuoSceneRenderer_drawElement(context->sceneRenderer, 0, .08f);	// Normals
	VuoSceneRenderer_drawElement(context->sceneRenderer, 1, .08f);	// Tangents
	VuoSceneRenderer_drawElement(context->sceneRenderer, 2, .08f);	// Bitangents
#endif
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->displayRefresh = VuoDisplayRefresh_make(context);
	VuoRetain(context->displayRefresh);

	context->sceneRenderer = VuoSceneRenderer_make();
	VuoRetain(context->sceneRenderer);

	context->window = VuoWindowOpenGl_make(
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
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoText) cameraName
)
{
	VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), objects);
	{
		VuoGlContext glContext = VuoGlContext_use();

		VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, glContext, rootSceneObject);

		VuoGlContext_disuse(glContext);
	}

	VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName);

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
	VuoRelease((*context)->displayRefresh);
}
