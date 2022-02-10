/**
 * @file
 * VuoImageRenderer implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"


#include <IOSurface/IOSurface.h>

#include <OpenGL/CGLMacro.h>
/// @{ Stub.
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#include "module.h"
extern "C"
{
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
	GLuint quadDataBuffer;
};
/**
 * Internal state data for a VuoImageRenderer instance.
 */
static VuoImageRendererInternal VuoImageRendererGlobal;

/**
 * Positions and texture coordinates for a quad.
 */
static const GLfloat quadData[] = {
	// Positions
	-1, -1, 0, 1,
	 1, -1, 0, 1,
	-1,  1, 0, 1,
	 1,  1, 0, 1,

	// Texture Coordinates
	0, 0, 0, 0,
	1, 0, 0, 0,
	0, 1, 0, 0,
	1, 1, 0, 0
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

/**
 * True if `VuoImageRenderer_init()` has initialized graphics data.
 */
static bool VuoImageRenderer_initialized = false;

/**
 * Initializes global state for rendering a @ref VuoImage.
 */
static void VuoImageRenderer_init(void)
{
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoImageRenderer_initialized = true;

		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glGenVertexArrays(1, &VuoImageRendererGlobal.vertexArray);
			glBindVertexArray(VuoImageRendererGlobal.vertexArray);
			{
				VuoImageRendererGlobal.quadDataBuffer = VuoGlPool_use(cgl_ctx, VuoGlPool_ArrayBuffer, sizeof(quadData));
				VuoGlPool_retain(VuoImageRendererGlobal.quadDataBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, VuoImageRendererGlobal.quadDataBuffer);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadData), quadData);
			}
			glBindVertexArray(0);

			glFlushRenderAPPLE();

			glGenFramebuffers(1, &VuoImageRendererGlobal.outputFramebuffer);
		});
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

	GLuint internalFormat;
	GLuint outputTexture = VuoImageRenderer_draw_internal(shader, pixelsWide, pixelsHigh, imageColorDepth, false, false, 0, &internalFormat);
	if (!outputTexture)
		return NULL;

	VuoImage outputImage = VuoImage_make(outputTexture, internalFormat, pixelsWide, pixelsHigh);

	VuoImage firstInputImage = VuoShader_getFirstImage(shader);
	if (firstInputImage)
		outputImage->scaleFactor = firstInputImage->scaleFactor;

	return outputImage;
}

/**
 * Helper for VuoImageRenderer_render().
 *
 * If @c outputToGlTextureRectangle is true, the caller is responsible for deleting the texture (it should not be thrown into the GL texture pool).
 */
unsigned long int VuoImageRenderer_draw_internal(VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth, bool outputToIOSurface, bool outputToGlTextureRectangle, unsigned int outputToSpecificTexture, GLuint *outputInternalFormat)
{
	VuoImageRenderer_init();

	__block GLuint outputTexture = 0;
	__block IOSurfaceID surfID = 0;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		glViewport(0, 0, pixelsWide, pixelsHigh);

		// Create a new GL Texture Object.
		bool shaderOpaque = VuoShader_isOpaque(shader);
		GLenum textureFormat = shaderOpaque ? GL_BGR : GL_BGRA;
		GLuint textureTarget = (outputToIOSurface || outputToGlTextureRectangle) ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
		GLuint textureTargetInternalFormat = VuoImageColorDepth_getGlInternalFormat(textureFormat, imageColorDepth);
		if (outputInternalFormat)
			*outputInternalFormat = textureTargetInternalFormat;

		VuoIoSurface ioSurface = NULL;
		if (outputToIOSurface)
			/// @todo only generate an IOSurface with an alpha channel if the alpha channel is really needed (take textureFormat into account)
			ioSurface = VuoIoSurfacePool_use(cgl_ctx, pixelsWide, pixelsHigh, &outputTexture);
		else if (outputToSpecificTexture)
			outputTexture = outputToSpecificTexture;
		else
			outputTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, outputToGlTextureRectangle ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D, textureTargetInternalFormat, pixelsWide, pixelsHigh, textureFormat, NULL);

		if (!outputTexture)
			return;

		glBindFramebuffer(GL_FRAMEBUFFER, VuoImageRendererGlobal.outputFramebuffer);
//		VLog("glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, %s, %d, 0);", VuoGl_stringForConstant(textureTarget), outputTexture);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, outputTexture, 0);

		if (!shaderOpaque)
		{
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// Execute the shader.
		{
			GLint positionAttribute;
			GLint textureCoordinateAttribute;
			bool ret = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &positionAttribute, NULL, &textureCoordinateAttribute, NULL);
			if (!ret)
			{
				VDebugLog("Error: Failed to get attribute locations.");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				if (outputToIOSurface)
					VuoIoSurfacePool_disuse(ioSurface, false);  // It was never used, so no need to quarantine it.
				else if (outputToSpecificTexture)
				{}
				else
				{
					VuoGlTexture_retain(outputTexture, NULL, NULL);
					VuoGlTexture_release(VuoGlTexturePool_Allocate, outputToGlTextureRectangle ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D, textureTargetInternalFormat, pixelsWide, pixelsHigh, outputTexture);
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

				glBindBuffer(GL_ARRAY_BUFFER, VuoImageRendererGlobal.quadDataBuffer);

				glEnableVertexAttribArray(positionAttribute);
				glVertexAttribPointer(positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);

				if (textureCoordinateAttribute != -1)
				{
					glEnableVertexAttribArray(textureCoordinateAttribute);
					glVertexAttribPointer(textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)(sizeof(GLfloat)*16));
				}

#ifdef VUO_PROFILE
	GLuint timeElapsedQuery;
	glGenQueries(1, &timeElapsedQuery);
	glBeginQuery(GL_TIME_ELAPSED_EXT, timeElapsedQuery);
#endif

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef VUO_PROFILE
	double seconds;
	glEndQuery(GL_TIME_ELAPSED_EXT);
	GLuint nanoseconds;
	glGetQueryObjectuiv(timeElapsedQuery, GL_QUERY_RESULT, &nanoseconds);
	seconds = ((double)nanoseconds) / NSEC_PER_SEC;
	glDeleteQueries(1, &timeElapsedQuery);

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
			VuoIoSurfacePool_disuse(ioSurface, true);  // We may be sending it to another process, so we need to quarantine it.
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
extern "C" void VuoImageRenderer_fini(void)
{
	if (! VuoImageRenderer_initialized)
		return;

	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			VuoGlPool_release(VuoGlPool_ArrayBuffer, sizeof(quadData), VuoImageRendererGlobal.quadDataBuffer);

			glDeleteVertexArrays(1, &VuoImageRendererGlobal.vertexArray);
			glDeleteFramebuffers(1, &VuoImageRendererGlobal.outputFramebuffer);
		});
	});
}
