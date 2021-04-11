/**
 * @file
 * vuo.scene.render.window node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneRenderer.h"
#include "VuoWindow.h"
#include "VuoEventLoop.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Scene to Window",
					 "keywords" : [
						 "draw", "graphics",
						 "3D", "object", "opengl", "scenegraph",
						 "display", "view", "screen", "full screen", "fullscreen",
					 ],
					 "version" : "2.3.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "DisplayScene.vuo", "DisplaySphere.vuo", "MoveSpinningSphere.vuo", "SwitchCameras.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
	VuoMultisample multisampling;
	dispatch_queue_t sceneRendererQueue;
};

static void vuo_scene_render_window_init(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
	VuoRetain(context->sceneRenderer);
}

static void vuo_scene_render_window_updateBacking(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	bool valid = false;
	VuoSceneObject so = VuoSceneRenderer_getRootSceneObject(context->sceneRenderer, &valid);
	if (valid)
		VuoSceneObject_retain(so);

	dispatch_sync(context->sceneRendererQueue, ^{
	VuoRelease(context->sceneRenderer);

	context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
	VuoRetain(context->sceneRenderer);
	if (valid)
	{
		VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, so);
		VuoSceneObject_release(so);
	}
				  });
}

static void vuo_scene_render_window_resize(void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;
	dispatch_sync(context->sceneRendererQueue, ^{
		VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
	});
}

static VuoIoSurface vuo_scene_render_window_draw(void *ctx)
{
	struct nodeInstanceData *context = ctx;
	__block VuoIoSurface vis;
	dispatch_sync(context->sceneRendererQueue, ^{
		vis = VuoSceneRenderer_renderToIOSurface(context->sceneRenderer, VuoImageColorDepth_8, context->multisampling, true);
	});
	return vis;
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = NULL;
	context->sceneRendererQueue = dispatch_queue_create("org.vuo.scene.render.window.sceneRenderer", VuoEventLoop_getDispatchInteractiveAttribute());

	context->window = VuoWindowOpenGl_make(
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
		VuoOutputTrigger(requestedFrame, VuoReal, {"name":"Refreshed at Time", "eventThrottling":"drop"})
)
{
	VuoWindowOpenGl_enableTriggers_deprecated((*context)->window, showedWindow, requestedFrame);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoText) cameraName,
		VuoInputData(VuoMultisample, {"default":"4"}) multisampling,
		VuoInputData(VuoList_VuoWindowProperty) setWindowProperties,
		VuoInputEvent({"eventBlocking":"none", "data":"setWindowProperties"}) setWindowPropertiesEvent
)
{
	(*context)->multisampling = multisampling;

	if (setWindowPropertiesEvent)
		VuoWindowOpenGl_setProperties((*context)->window, setWindowProperties);

	VuoSceneObject rootSceneObject = VuoSceneObject_makeGroup(objects, VuoTransform_makeIdentity());

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
	struct nodeInstanceData *c = *context;

	// Ensure the context isn't deallocated until the window has finished closing
	// (which might be a while if macOS is doing its exit-fullscreen animation).
	VuoRetain(c);

	VuoWindowOpenGl_close(c->window, ^{
		VuoRelease(c->sceneRenderer);
		dispatch_release(c->sceneRendererQueue);
		VuoRelease(c);
	});

	VuoRelease(c->window);
}
