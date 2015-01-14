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

#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurfaceAPI.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}
#include <OpenGL/CGLIOSurface.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageRenderer",
					 "dependencies" : [
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
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
VuoImageRenderer VuoImageRenderer_make(void)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)malloc(sizeof(struct VuoImageRendererInternal));
	VuoRegister(imageRenderer, VuoImageRenderer_destroy);

	VuoGlContext_use();
	{
		glGenBuffers(1, &imageRenderer->quadPositionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadPositionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

		glGenBuffers(1, &imageRenderer->quadTextureCoordinateBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadTextureCoordinateBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadTextureCoordinates), quadTextureCoordinates, GL_STATIC_DRAW);

		glGenBuffers(1, &imageRenderer->quadElementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageRenderer->quadElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STATIC_DRAW);
	}
	VuoGlContext_disuse();

	return (VuoImageRenderer)imageRenderer;
}

/**
 * Produces a new @c VuoImage by rendering @c shader.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 * However, it's not safe to use the same @c VuoImageRenderer instance from multiple threads simultaneously.
 */
VuoImage VuoImageRenderer_draw(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	return VuoImage_make(VuoImageRenderer_draw_internal(ir,shader,pixelsWide,pixelsHigh,false), pixelsWide, pixelsHigh);
}

/**
 * Helper for VuoImageRenderer_draw().
 */
unsigned long int VuoImageRenderer_draw_internal(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, bool outputToIOSurface)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;

	GLuint outputTexture;
	IOSurfaceID surfID;
	CGLContextObj ctx = (CGLContextObj)VuoGlContext_use();
	{
		glViewport(0, 0, pixelsWide, pixelsHigh);

		// Create a new GL Texture Object.
		GLuint textureTarget = outputToIOSurface ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
		glGenTextures(1, &outputTexture);
		if (outputToIOSurface)
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(textureTarget, outputTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
			CGLError err = CGLTexImageIOSurface2D(ctx, textureTarget, GL_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
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

		// Create a new GL Framebuffer Object, backed by the above GL Texture Object.
		GLuint outputFramebuffer;
		glGenFramebuffers(1, &outputFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, outputTexture, 0);

		// Execute the shader.
		glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
		{
			glUseProgram(shader->glProgramName);
			{
				VuoShader_activateTextures(shader);
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
					}

					glDisableVertexAttribArray(textureCoordinateAttribute);
					glDisableVertexAttribArray(positionAttribute);
				}
				VuoShader_deactivateTextures(shader);
			}
			glUseProgram(0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &outputFramebuffer);

		if (outputToIOSurface)
			glDeleteTextures(1, &outputTexture);
	}
	VuoGlContext_disuse();

	if (outputToIOSurface)
		return surfID;
	else
		return outputTexture;
}

/**
 * Destroys and deallocates the image renderer.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
void VuoImageRenderer_destroy(VuoImageRenderer ir)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;

	VuoGlContext_use();
	{
		glDeleteBuffers(1, &imageRenderer->quadElementBuffer);
		glDeleteBuffers(1, &imageRenderer->quadPositionBuffer);
	}
	VuoGlContext_disuse();

	free(imageRenderer);
}
