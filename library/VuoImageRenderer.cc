/**
 * @file
 * VuoImageRenderer implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <stdlib.h>

#include <IOSurface/IOSurface.h>
#include <CoreServices/CoreServices.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
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
						 "VuoImage",
						 "VuoImageColorDepth",
						 "VuoShader",
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

	GLuint vertexArray;
	GLuint triDataBuffer;
};
/**
 * Internal state data for a VuoImageRenderer instance.
 */
static VuoImageRendererInternal VuoImageRendererGlobal;

/**
 * Positions and texture coordinates for a full-screen triangle.
 */
static const GLfloat triData[] = {
	// Positions
	-1, -1, 0, 1,
	 3, -1, 0, 1,
	-1,  3, 0, 1,

	// Texture Coordinates
	0, 0, 0, 0,
	2, 0, 0, 0,
	0, 2, 0, 0,
};
/**
 * An identity matrix.
 */
static const GLfloat unityMatrix[16] = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1
};

static void VuoImageRenderer_fini(void);

/**
 * Initializes global state for rendering a @ref VuoImage.
 */
static void VuoImageRenderer_init(void)
{
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glGenVertexArrays(1, &VuoImageRendererGlobal.vertexArray);
			glBindVertexArray(VuoImageRendererGlobal.vertexArray);
			{
				VuoImageRendererGlobal.triDataBuffer = VuoGlPool_use(cgl_ctx, VuoGlPool_ArrayBuffer, sizeof(triData));
				VuoGlPool_retain(VuoImageRendererGlobal.triDataBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, VuoImageRendererGlobal.triDataBuffer);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(triData), triData);
			}
			glBindVertexArray(0);

			glFlushRenderAPPLE();

			glGenFramebuffers(1, &VuoImageRendererGlobal.outputFramebuffer);
		});

		VuoAddCompositionFiniCallback(VuoImageRenderer_fini);
	});
}

/**
 * Deprecated.  This shim exists to enable old 3rd-party nodes to continue working.
 */
extern "C" VuoImage VuoImageRenderer_draw(void *ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth)
{
	return VuoImageRenderer_render(shader, pixelsWide, pixelsHigh, imageColorDepth);
}

/**
 * Deprecated. This shim exists to enable old 3rd-party nodes to continue working.
 */
extern "C" void *VuoImageRenderer_make(void *)
{
	return NULL;
}

/**
 * Produces a new @c VuoImage by rendering @c shader.
 *
 * @threadAny
 */
VuoImage VuoImageRenderer_render(VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth)
{
	if (pixelsWide < 1 || pixelsHigh < 1)
		return NULL;

	GLuint outputTexture = VuoImageRenderer_draw_internal(shader,pixelsWide,pixelsHigh,imageColorDepth,false,false,0);
	if (!outputTexture)
		return NULL;

	return VuoImage_make(outputTexture, VuoImageColorDepth_getGlInternalFormat(GL_BGRA, imageColorDepth), pixelsWide, pixelsHigh);
}

/**
 * Helper for VuoImageRenderer_render().
 *
 * If @c outputToGlTextureRectangle is true, the caller is responsible for deleting the texture (it should not be thrown into the GL texture pool).
 */
unsigned long int VuoImageRenderer_draw_internal(VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth, bool outputToIOSurface, bool outputToGlTextureRectangle, unsigned int outputToSpecificTexture)
{
	VuoImageRenderer_init();

	__block GLuint outputTexture = 0;
	__block IOSurfaceID surfID = 0;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		glViewport(0, 0, pixelsWide, pixelsHigh);

		// Create a new GL Texture Object.
		GLuint textureTarget = (outputToIOSurface || outputToGlTextureRectangle) ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
		GLuint textureTargetInternalFormat = VuoImageColorDepth_getGlInternalFormat(GL_BGRA, imageColorDepth);

		VuoIoSurface ioSurface = NULL;
		if (outputToIOSurface)
			ioSurface = VuoIoSurfacePool_use(cgl_ctx, pixelsWide, pixelsHigh, &outputTexture);
		else if (outputToSpecificTexture)
			outputTexture = outputToSpecificTexture;
		else
		{
			if (outputToGlTextureRectangle)
			{
				glGenTextures(1, &outputTexture);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, outputTexture);
//				VLog("glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, %s, %d, %d, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);", VuoGl_stringForConstant(textureTargetInternalFormat), pixelsWide, pixelsHigh);
				glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, textureTargetInternalFormat, pixelsWide, pixelsHigh, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			}
			else
				outputTexture = VuoGlTexturePool_use(cgl_ctx, textureTargetInternalFormat, pixelsWide, pixelsHigh, GL_BGRA);
		}

		if (!outputTexture)
			return;

		glBindFramebuffer(GL_FRAMEBUFFER, VuoImageRendererGlobal.outputFramebuffer);
//		VLog("glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, %s, %d, 0);", VuoGl_stringForConstant(textureTarget), outputTexture);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, outputTexture, 0);

		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Execute the shader.
		{
			GLint positionAttribute;
			GLint textureCoordinateAttribute;
			bool ret = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &positionAttribute, NULL, NULL, NULL, &textureCoordinateAttribute);
			if (!ret)
			{
				VUserLog("Error: Failed to get attribute locations.");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				if (outputToIOSurface)
					VuoIoSurfacePool_disuse(ioSurface);
				else if (outputToSpecificTexture)
				{}
				else
				{
					if (outputToGlTextureRectangle)
						glDeleteTextures(1, &outputTexture);
					else
					{
						VuoGlTexture_retain(outputTexture, NULL, NULL);
						VuoGlTexture_release(textureTargetInternalFormat, pixelsWide, pixelsHigh, outputTexture, outputToGlTextureRectangle ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D);
					}
				}
				outputTexture = 0;
				surfID = 0;
				return;
			}

			VuoGlProgram program;
			if (!VuoShader_activate(shader, VuoMesh_IndividualTriangles, cgl_ctx, &program))
			{
				VUserLog("Shader activation failed.");
				outputTexture = 0;
				surfID = 0;
				return;
			}

			{
				glBindVertexArray(VuoImageRendererGlobal.vertexArray);

				GLint projectionMatrixUniform = VuoGlProgram_getUniformLocation(program, "projectionMatrix");
				glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, unityMatrix);

				GLint cameraMatrixInverseUniform = VuoGlProgram_getUniformLocation(program, "cameraMatrixInverse");
				glUniformMatrix4fv(cameraMatrixInverseUniform, 1, GL_FALSE, unityMatrix);

				GLint useFisheyeProjectionUniform = VuoGlProgram_getUniformLocation(program, "useFisheyeProjection");
				glUniform1i(useFisheyeProjectionUniform, false);

				GLint modelviewMatrixUniform = VuoGlProgram_getUniformLocation(program, "modelviewMatrix");
				glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, unityMatrix);

				GLint aspectRatioUniform = VuoGlProgram_getUniformLocation(program, "aspectRatio");
				if (aspectRatioUniform != -1)
					glUniform1f(aspectRatioUniform, (float)pixelsWide/(float)pixelsHigh);

				GLint viewportSizeUniform = VuoGlProgram_getUniformLocation(program, "viewportSize");
				if (viewportSizeUniform != -1)
					glUniform2f(viewportSizeUniform, (float)pixelsWide, (float)pixelsHigh);

				glBindBuffer(GL_ARRAY_BUFFER, VuoImageRendererGlobal.triDataBuffer);

				glEnableVertexAttribArray(positionAttribute);
				glVertexAttribPointer(positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);

				if (textureCoordinateAttribute != -1)
				{
					glEnableVertexAttribArray(textureCoordinateAttribute);
					glVertexAttribPointer(textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)(sizeof(GLfloat)*12));
				}

#ifdef PROFILE
	GLuint timeElapsedQuery;
	double timeStart = 0;
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
	if (macMinorVersion < 9)
	{
		// Prior to OS X v10.9, glGetQueryObjectuiv() isn't likely to work.
		// (On NVIDIA GeForce 9400M on OS X v10.8, it hangs for 6 seconds then returns bogus data.)
		// https://www.mail-archive.com/mac-opengl@lists.apple.com/msg00003.html
		// https://b33p.net/kosada/node/10677
		glFinish();
		timeStart = VuoLogGetTime();
	}
	else
	{
		glGenQueries(1, &timeElapsedQuery);
		glBeginQuery(GL_TIME_ELAPSED_EXT, timeElapsedQuery);
	}
#endif

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

#ifdef PROFILE
	double seconds;
	if (macMinorVersion < 9)
	{
		glFinish();
		seconds = VuoLogGetTime() - timeStart;
	}
	else
	{
		glEndQuery(GL_TIME_ELAPSED_EXT);
		GLuint nanoseconds;
		glGetQueryObjectuiv(timeElapsedQuery, GL_QUERY_RESULT, &nanoseconds);
		seconds = ((double)nanoseconds) / NSEC_PER_SEC;
		glDeleteQueries(1, &timeElapsedQuery);
	}

	double objectPercent = seconds / (1./60.) * 100.;
	VLog("%6.2f %% of 60 Hz frame	%s", objectPercent, shader->name);
#endif

				if (textureCoordinateAttribute != -1)
				glDisableVertexAttribArray(textureCoordinateAttribute);

				glDisableVertexAttribArray(positionAttribute);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);
			}
			VuoShader_deactivate(shader, VuoMesh_IndividualTriangles, cgl_ctx);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, 0, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (outputToIOSurface)
		{
			surfID = VuoIoSurfacePool_getId(ioSurface);
			VuoIoSurfacePool_disuse(ioSurface);
		}

		glFlushRenderAPPLE();
	});

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
static void VuoImageRenderer_fini(void)
{
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		VuoGlPool_release(cgl_ctx, VuoGlPool_ArrayBuffer, sizeof(triData), VuoImageRendererGlobal.triDataBuffer);

		glDeleteVertexArrays(1, &VuoImageRendererGlobal.vertexArray);
		glDeleteFramebuffers(1, &VuoImageRendererGlobal.outputFramebuffer);
	});
}
