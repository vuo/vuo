/**
 * @file
 * vuo.scene.render.window node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>

#define DEBUG 0


VuoModuleMetadata({
					 "title" : "Render Scene to Window",
					 "description" :
						 "<p>Displays a window containing a 3D scene.</p> \
						 <p>When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.</p> \
						 <p>The window provides a menu option to toggle between windowed and full-screen mode.</p> \
						 <p>Mouse positions are in Vuo coordinates. (0,0,0) is the center of the scene. \
						 The scene has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. \
						 The scene's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. \
						 The scene's camera is at (0,0,1), with the z-coordinate increasing from back to front.</p> \
						 <p><ul> \
						 <li>`objects` — The 3D objects to place in the scene.</li> \
						 <li>`frameRequested` — When the display is ready for the next frame, fires an event with information about the frame.</li> \
						 <li>`movedMouseTo` — When the mouse is moved while this is the active (frontmost) window, fires an event with the current position of the pointer.</li> \
						 <li>`scrolledMouse` — When the mouse is scrolled, fires an event with the distance scrolled. This is the change in \
						 position since the previous `scrolled` event. The vertical distance (y-coordinate) respects the operating \
						 system's \"natural scrolling\" setting.</li> \
						 <li>`usedMouseButton` — When a mouse button is pressed or released, fires an event with information about how \
						 the mouse button was used.</li> \
						 </ul></p>",
					 "keywords" : [ "frame", "draw", "opengl", "scenegraph", "graphics", "display", "view", "object",
						 "mouse", "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer"],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
};

void vuo_scene_render_window_init(void *ctx)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_prepareContext(context->sceneRenderer);
}

void vuo_scene_render_window_resize(void *ctx, unsigned int width, unsigned int height)
{
	//fprintf(stderr, "vuo_scene_render_window_resize(%d,%d)\n", width, height);
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_scene_render_window_draw(void *ctx)
{
	//fprintf(stderr, "vuo_scene_render_window_draw\n");
	struct nodeInstanceData *context = ctx;

	glClearColor(0,0,0,0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	VuoSceneRenderer_draw(context->sceneRenderer);

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
	VuoWindowOpenGl_enableTriggers((*context)->window, requestedFrame, movedMouseTo, scrolledMouse, usedMouseButton);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoSceneObject) objects
)
{
	VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), objects);
	VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);

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
	VuoRelease((*context)->window);
	VuoRelease((*context)->sceneRenderer);
}
