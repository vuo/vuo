/**
 * @file
 * VuoImageRenderer implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceAPI.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include <OpenGL/CGLIOSurface.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageRenderer",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ]
				 });
#endif
}

/**
 * Internal state data for a VuoImageRenderer instance.
 */
struct VuoImageRendererInternal
{
	GLuint outputFramebuffer;

	GLuint quadPositionBuffer;
	GLuint quadTextureCoordinateBuffer;
	GLuint quadElementBuffer;
};

static const GLfloat quadPositions[] = {
	-1, -1, 0, 1,
	 1, -1, 0, 1,
	-1,  1, 0, 1,
	 1,  1, 0, 1
};
static const GLfloat quadTextureCoordinates[] = {
	0, 0, 0, 0,
	1, 0, 0, 0,
	0, 1, 0, 0,
	1, 1, 0, 0
};
static const GLushort quadElements[] = { 0, 1, 2, 3 };
static const GLfloat unityMatrix[16] = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1
};

void VuoImageRenderer_destroy(VuoImageRenderer ir);

/**
 * Creates a reference-counted object for rendering a @ref VuoImage.
 *
 * @threadAny
 */
VuoImageRenderer VuoImageRenderer_make(void)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)malloc(sizeof(struct VuoImageRendererInternal));
	VuoRegister(imageRenderer, VuoImageRenderer_destroy);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		imageRenderer->quadPositionBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadPositionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STREAM_DRAW);

		imageRenderer->quadTextureCoordinateBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadTextureCoordinateBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadTextureCoordinates), quadTextureCoordinates, GL_STREAM_DRAW);

		{
			GLuint vertexArray;
			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);

			imageRenderer->quadElementBuffer = VuoGlPool_use(VuoGlPool_ElementArrayBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageRenderer->quadElementBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STREAM_DRAW);

			glBindVertexArray(0);
			glDeleteVertexArrays(1, &vertexArray);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		VuoGlContext_disuse(cgl_ctx);
	}

	return (VuoImageRenderer)imageRenderer;
}

/**
 * Produces a new @c VuoImage by rendering @c shader.
 *
 * @threadAnyGL
 * (Additionally, the caller is responsible for ensuring that the same @c VuoImageRenderer is not used simultaneously on multiple threads.)
 */
VuoImage VuoImageRenderer_draw(VuoImageRenderer ir, VuoGlContext glContext, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	return VuoImage_make(VuoImageRenderer_draw_internal(ir,glContext,shader,pixelsWide,pixelsHigh,false), pixelsWide, pixelsHigh);
}

/**
 * Helper for VuoImageRenderer_draw().
 */
unsigned long int VuoImageRenderer_draw_internal(VuoImageRenderer ir, VuoGlContext glContext, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, bool outputToIOSurface)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;

	GLuint outputTexture;
	IOSurfaceID surfID;
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		// Allocate a single vertex array for all drawing during this pass (since VAOs can't be shared between contexts).
		GLuint vertexArray;
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		glViewport(0, 0, pixelsWide, pixelsHigh);

		// Create a new GL Texture Object.
		GLuint textureTarget = outputToIOSurface ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
		outputTexture = VuoGlPool_use(VuoGlPool_Texture);
		if (outputToIOSurface)
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(textureTarget, outputTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (outputToIOSurface)
		{
			CFMutableDictionaryRef properties = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
			CFDictionaryAddValue(properties, kIOSurfaceIsGlobal, kCFBooleanTrue);
			long long pixelsWideLL = pixelsWide;
			CFDictionaryAddValue(properties, kIOSurfaceWidth, CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsWideLL));
			long long pixelsHighLL = pixelsHigh;
			CFDictionaryAddValue(properties, kIOSurfaceHeight, CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsHighLL));
			long long bytesPerElement = 4;
			CFDictionaryAddValue(properties, kIOSurfaceBytesPerElement, CFNumberCreate(NULL, kCFNumberLongLongType, &bytesPerElement));

			IOSurfaceRef surf = IOSurfaceCreate(properties);
			CGLError err = CGLTexImageIOSurface2D(cgl_ctx, textureTarget, GL_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
			surfID = IOSurfaceGetID(surf);
			// IOSurfaceDecrementUseCount(surf); ?
			CFRelease(surf);
			if(err != kCGLNoError)
			{
				fprintf(stderr,"VuoImageRenderer_draw_internal() Error in CGLTexImageIOSurface2D(): %s\n", CGLErrorString(err));
				return 0;
			}
		}
		else
			glTexImage2D(textureTarget, 0, GL_RGBA, pixelsWide, pixelsHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(textureTarget, 0);

		// Create a new GL Framebuffer Object, backed by the above GL Texture Object.
		GLuint outputFramebuffer;
		glGenFramebuffers(1, &outputFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, outputTexture, 0);

		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Execute the shader.
		{
			glUseProgram(shader->glProgramName);
			{
				VuoShader_activateTextures(shader, cgl_ctx);
				{
					GLint projectionMatrixUniform = glGetUniformLocation(shader->glProgramName, "projectionMatrix");
					glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, unityMatrix);

					GLint modelviewMatrixUniform = glGetUniformLocation(shader->glProgramName, "modelviewMatrix");
					glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, unityMatrix);

					GLuint positionAttribute = glGetAttribLocation(shader->glProgramName, "position");
					glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadPositionBuffer);
					glVertexAttribPointer(positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);
					glEnableVertexAttribArray(positionAttribute);

					GLint textureCoordinateAttribute = glGetAttribLocation(shader->glProgramName, "textureCoordinate");
					glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadTextureCoordinateBuffer);
					glVertexAttribPointer(textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);
					glEnableVertexAttribArray(textureCoordinateAttribute);

					{
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageRenderer->quadElementBuffer);
						glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
					}

					glDisableVertexAttribArray(textureCoordinateAttribute);
					glDisableVertexAttribArray(positionAttribute);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				VuoShader_deactivateTextures(shader, cgl_ctx);
			}
			glUseProgram(0);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, 0, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &outputFramebuffer);

		if (outputToIOSurface)
			VuoGlPool_disuse(VuoGlPool_Texture, outputTexture);

		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vertexArray);

		glFlushRenderAPPLE();
	}

	if (outputToIOSurface)
		return surfID;
	else
		return outputTexture;
}

/**
 * Destroys and deallocates the image renderer.
 *
 * @threadAny
 */
void VuoImageRenderer_destroy(VuoImageRenderer ir)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		/// @todo https://b33p.net/kosada/node/6752 — Why does this leak if we recycle it?
		glDeleteBuffers(1,&imageRenderer->quadElementBuffer);
//		VuoGlPool_disuse(VuoGlPool_ElementArrayBuffer, imageRenderer->quadElementBuffer);

		VuoGlContext_disuse(cgl_ctx);
	}

	VuoGlPool_disuse(VuoGlPool_ArrayBuffer, imageRenderer->quadTextureCoordinateBuffer);
	VuoGlPool_disuse(VuoGlPool_ArrayBuffer, imageRenderer->quadPositionBuffer);

	free(imageRenderer);
}
