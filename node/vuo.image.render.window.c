/**
 * @file
 * vuo.image.render.window node implementation.
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

#include <dispatch/dispatch.h>

#include <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>

VuoModuleMetadata({
					 "title" : "Render Image to Window",
					 "description" :
						"<p>Displays a window containing an image.</p> \
						<p>When the composition starts or this node is added to a running composition, it pops up a window that contains a graphics area.</p> \
						<p>When this node receives an event, it places the image in the graphics area. The window is resized to fit the image.</p> \
						<p>When the user resizes the window, the window's aspect ratio is kept the same as the image's.</p> \
						<p>The window provides a menu option to toggle between windowed and full-screen mode.</p>",
					 "keywords" : [ "draw", "graphics", "display", "view", "full screen", "fullscreen" ],
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
	VuoSceneRenderer sceneRenderer;

	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to @c rootSceneObject.
	VuoSceneObject rootSceneObject;
};

void vuo_image_render_window_init(void *ctx)
{
	struct nodeInstanceData *context = ctx;

	VuoSceneRenderer_prepareContext(context->sceneRenderer);
}

void vuo_image_render_window_resize(void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;
	VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
}

void vuo_image_render_window_draw(void *ctx)
{
	struct nodeInstanceData *context = ctx;

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	dispatch_semaphore_wait(context->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoSceneRenderer_draw(context->sceneRenderer);
	dispatch_semaphore_signal(context->scenegraphSemaphore);
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = VuoSceneRenderer_make();
	VuoRetain(context->sceneRenderer);

	VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
	VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
	context->rootSceneObject = VuoSceneObject_make(
				verticesList,
				VuoShader_makeImageShader(),
				VuoTransform_makeIdentity(),
				VuoListCreate_VuoSceneObject()
			);
	context->rootSceneObject.transform.scale.x = 2;
	VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, context->rootSceneObject);

	context->scenegraphSemaphore = dispatch_semaphore_create(1);

	context->window = VuoWindowOpenGl_make(
				vuo_image_render_window_init,
				vuo_image_render_window_resize,
				vuo_image_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowOpenGl_enableTriggers((*context)->window, NULL, NULL, NULL, NULL);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoImage) image
)
{
	dispatch_semaphore_wait((*context)->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	{
		VuoShader_resetTextures((*context)->rootSceneObject.shader);
		if (image)
		{
			VuoShader_addTexture((*context)->rootSceneObject.shader, image, "texture");
			(*context)->rootSceneObject.transform.scale.y = 2. * (float)image->pixelsHigh/(float)image->pixelsWide;
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
	VuoRelease((*context)->window);
	VuoRelease((*context)->sceneRenderer);
	dispatch_release((*context)->scenegraphSemaphore);
}
