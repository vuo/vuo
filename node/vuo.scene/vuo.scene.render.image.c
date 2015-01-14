/**
 * @file
 * vuo.scene.render.image node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"
#include "VuoSceneRenderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>


VuoModuleMetadata({
					 "title" : "Render Scene to Image",
					 "keywords" : [ "draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoGlPool",
						 "VuoSceneRenderer"
					 ],
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "RippleImageOfSphere.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoSceneRenderer *sceneRenderer;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));

	context->sceneRenderer = VuoSceneRenderer_make();
	VuoRetain(context->sceneRenderer);

	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoInteger, {"default":1024}) width,
		VuoInputData(VuoInteger, {"default":768}) height,
		VuoInputData(VuoText) cameraName,
		VuoOutputData(VuoImage) image,
		VuoOutputData(VuoImage) depthImage
)
{
	VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), objects);

	GLuint outputTexture;
	GLuint outputDepthTexture;
	{
		VuoGlContext glContext = VuoGlContext_use();
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, glContext, rootSceneObject);

		VuoSceneRenderer_setCameraName((*context)->sceneRenderer, cameraName);

		VuoSceneRenderer_prepareContext((*context)->sceneRenderer, glContext);

		VuoSceneRenderer_regenerateProjectionMatrix((*context)->sceneRenderer, width, height);
		glViewport(0, 0, width, height);

		// Create a new GL Texture Object and Framebuffer Object.
		outputTexture = VuoGlPool_use(VuoGlPool_Texture);
		glBindTexture(GL_TEXTURE_2D, outputTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		outputDepthTexture = VuoGlPool_use(VuoGlPool_Texture);
		glBindTexture(GL_TEXTURE_2D, outputDepthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		GLuint outputFramebuffer;
		glGenFramebuffers(1, &outputFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outputDepthTexture, 0);

		// Execute the shader every frame
		{
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			VuoSceneRenderer_draw((*context)->sceneRenderer, glContext);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &outputFramebuffer);

		glFlushRenderAPPLE();
		VuoGlContext_disuse(glContext);
	}

	*image = VuoImage_make(outputTexture, width, height);
	*depthImage = VuoImage_make(outputDepthTexture, width, height);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->sceneRenderer);
}
