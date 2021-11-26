/**
 * @file
 * VuoImage implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "node.h"
#include "VuoImage.h"
#include "VuoImageRenderer.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <CoreFoundation/CoreFoundation.h>

#include <IOSurface/IOSurfaceAPI.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
/// @{ Stub.
void glBindVertexArray(GLuint array);
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
void glGenVertexArrays(GLsizei n, GLuint *arrays);
/// @}


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Image",
					 "description" : "An image residing in GPU memory (GL Texture Object).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoBoolean",
						 "VuoColor",
						 "VuoImageColorDepth",
						 "VuoImageWrapMode",
						 "VuoPoint2d",
						 "VuoGlContext",
						 "VuoGlPool",
						 "VuoImageRenderer",
						 "VuoImageBlur",
						 "VuoImageMapColors",
						 "CoreFoundation.framework",
						 "IOSurface.framework"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoImage
 * Decrements the retain count of the OpenGL Texture Object associated with the specified @c VuoImage,
 * and frees the @c texture VuoImage struct.
 *
 * @threadAny
 */
void VuoImage_free(void *texture)
{
	VuoImage t = (VuoImage)texture;
//	VLog("Freeing image %p %s",t,VuoImage_getSummary(t));

	// Detach the CPU memory from the GL texture before recycling.
	if (t->cpuQueueInitialized && json_object_object_length(t->cpuData))
	{
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		glBindTexture(t->glTextureTarget, t->glTextureName);
		GLenum format = GL_BGRA;
		if (t->glInternalFormat == GL_DEPTH_COMPONENT)
			format = GL_DEPTH_COMPONENT;
		GLenum type = VuoGlTexture_getType(format);
//		VLog("glTexImage2D(%s, 0, %s, %ld, %ld, 0, %s, %s, NULL);", VuoGl_stringForConstant(t->glTextureTarget), VuoGl_stringForConstant(t->glInternalFormat), t->pixelsWide, t->pixelsHigh, VuoGl_stringForConstant(format), VuoGl_stringForConstant(type));
		glTexImage2D(t->glTextureTarget, 0, t->glInternalFormat, t->pixelsWide, t->pixelsHigh, 0, format, type, NULL);
		glBindTexture(t->glTextureTarget, 0);
		});
	}

	VuoGlTexture_release(VuoGlTexturePool_Allocate, t->glTextureTarget, t->glInternalFormat, t->pixelsWide, t->pixelsHigh, t->glTextureName);

	if (t->cpuQueueInitialized)
	{
		dispatch_release(t->cpuQueue);

		json_object_object_foreach(t->cpuData, key, value)
		{
//			VLog("%p: freeing %s",t,key);
			json_object *o;
			json_object_object_get_ex(value, "buffer", &o);
			void *buffer = (void *)json_object_get_int64(o);

			json_object_object_get_ex(value, "freeCallback", &o);
			void (^freeCallback)(void *) = (void (^)(void *))json_object_get_int64(o);

			freeCallback(buffer);

			Block_release(freeCallback);
		}

		json_object_put(t->cpuData);
	}

	free(t);
}

/**
 * Helper for @c VuoImage_make and @c VuoImage_makeClientOwned.
 */
static VuoImage VuoImage_make_internal(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext)
{
	VuoImage t = (VuoImage)malloc(sizeof(struct _VuoImage));
	VuoRegister(t, VuoImage_free);

	t->glTextureName = glTextureName;
	t->glTextureTarget = GL_TEXTURE_2D;
	t->glInternalFormat = glInternalFormat;
	t->pixelsWide = pixelsWide;
	t->pixelsHigh = pixelsHigh;
	t->scaleFactor = 1;

	t->freeCallbackContext = freeCallbackContext;

	VuoGlTexture_retain(glTextureName, freeCallback, freeCallbackContext);

	t->cpuQueueInitialized = 0;

//	VLog("Made image %p %s",t,VuoImage_getSummary(t));
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_2D.
 *    - use wrap mode GL_CLAMP_TO_EDGE on the S and T axes
 *    - use minifying and magnification filter GL_LINEAR
 *
 * The @ref VuoImage takes ownership of @c glTextureName,
 * and will call @c glDeleteTextures() on it when it's no longer needed.
 *
 * @ref VuoImage is intended to be immutable — do not modify
 * the contents of the @c glTextureName after passing it to this function.
 *
 * @threadAny
 */
VuoImage VuoImage_make(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh)
{
	if (!glTextureName || !pixelsWide || !pixelsHigh)
		return NULL;

	VuoImage t = VuoImage_make_internal(glTextureName, glInternalFormat, pixelsWide, pixelsHigh, NULL, NULL);
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_2D
 *    - use wrap mode GL_CLAMP_TO_EDGE on the S and T axes
 *    - use minifying and magnification filter GL_LINEAR
 *
 * When the VuoImage is no longer needed, @c freeCallback is called.
 * The @c freeCallback may then activate a GL context and delete the texture, or send it back to a texture pool.
 * `freeCallback` must not be NULL.
 *
 * @c freeCallbackContext is optional data passed to @c freeCallback via the @ref VuoImage struct.
 * Use @c NULL if you don't need to provide additional data to the callback.
 *
 * @ref VuoImage is intended to be immutable — do not modify the contents of the @c glTextureName
 * between passing it to this function and when @c freeCallback is called.
 *
 * @threadAny
 */
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext)
{
	if (!glTextureName || !pixelsWide || !pixelsHigh)
		return NULL;

	return VuoImage_make_internal(glTextureName, glInternalFormat, pixelsWide, pixelsHigh, freeCallback, freeCallbackContext);
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_RECTANGLE_ARB
 *    - use wrap mode GL_CLAMP_TO_EDGE on the S and T axes
 *    - use minifying and magnification filter GL_LINEAR
 *
 * When the VuoImage is no longer needed, @c freeCallback is called.
 * The @c freeCallback may then activate a GL context and delete the texture, or send it back to a texture pool.
 * `freeCallback` must not be NULL.
 *
 * @c freeCallbackContext is optional data passed to @c freeCallback via the @ref VuoImage struct.
 * Use @c NULL if you don't need to provide additional data to the callback.
 *
 * @ref VuoImage is intended to be immutable — do not modify the contents of the @c glTextureName
 * between passing it to this function and when @c freeCallback is called.
 *
 * @note Images returned by this function may only be passed to functions that explicitly mention support for @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
VuoImage VuoImage_makeClientOwnedGlTextureRectangle(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext)
{
	VuoImage t = VuoImage_makeClientOwned(glTextureName, glInternalFormat, pixelsWide, pixelsHigh, freeCallback, freeCallbackContext);
	if (!t)
		return NULL;

	t->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;

	return t;
}

/**
 * @ingroup VuoImage
 * Uploads the specified pixel data to the GPU and returns a new @ref VuoImage referencing it.
 *
 * @threadAny
 *
 * @param pixels Pointer to a buffer of pixel data.  Row-major, starting at the bottom (flipped).  Zero stride.  Premultiplied alpha.
 *               Vuo takes ownership of this data, and will invoke `freeCallback` when it's done using it.
 * @param format An OpenGL format constant.  Supported formats include:
 *    - @c GL_RGB
 *    - @c GL_YCBCR_422_APPLE
 *    - @c GL_RGBA
 *    - @c GL_BGRA
 *    - @c GL_LUMINANCE
 *    - @c GL_LUMINANCE_ALPHA
 * @param pixelsWide Width in pixels
 * @param pixelsHigh Height in pixels
 * @param colorDepth The number of bits per channel.
 *                   When 8bpc, `pixels` should be `unsigned char *`;
 *                   when 16bpc, `pixels` should be `__fp16 *`;
 *                   when 32bpc, `pixels` should be `float *`.
 * @param freeCallback
 */
VuoImage VuoImage_makeFromBuffer(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels))
{
	return VuoImage_makeFromBufferWithStride(pixels, format, pixelsWide, pixelsHigh, 0, colorDepth, freeCallback);
}

/**
 * Same as @ref VuoImage_makeFromBuffer, but accepts a nonstandard stride (`bytesPerRow`).
 */
VuoImage VuoImage_makeFromBufferWithStride(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, unsigned int bytesPerRow, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels))
{
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	__block GLenum internalformat;
	__block GLuint glTextureName;
	__block int alignment = 1;
	__block bool customRowLength = false;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){

	internalformat = VuoImageColorDepth_getGlInternalFormat(format, colorDepth);
//	VLog("Using format=%s -> internalformat=%s", VuoGl_stringForConstant(format), VuoGl_stringForConstant(internalformat));
	glTextureName = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, GL_TEXTURE_2D, internalformat, pixelsWide, pixelsHigh, format, NULL);
	if (!glTextureName)
		return;

	int bytesPerPixel = VuoGlTexture_getChannelCount(format);
	GLuint glType;
	if (colorDepth == VuoImageColorDepth_8)
		glType = VuoGlTexture_getType(format);
	else if (colorDepth == VuoImageColorDepth_16)
	{
		glType = GL_HALF_FLOAT_ARB;
		bytesPerPixel *= 2;
	}
	else // if (colorDepth == VuoImageColorDepth_32)
	{
		glType = GL_FLOAT;
		bytesPerPixel *= 4;
	}

	if (!bytesPerRow
	 || bytesPerRow == bytesPerPixel * pixelsWide)
		// Tightly-packed.
		alignment = 1;
	else
	{
		if (bytesPerRow % 4 == 0)
			alignment = 4;
		else if (bytesPerRow % 8 == 0)
			alignment = 8;
		else if (bytesPerRow % 2 == 0)
			alignment = 2;
		else if (bytesPerRow % bytesPerPixel == 0)
		{
			GLuint rowPixels = bytesPerRow / bytesPerPixel;
			glPixelStorei(GL_UNPACK_ROW_LENGTH, rowPixels);
			customRowLength = true;
		}
		else
		{
			VUserLog("Not sure how to handle this stride:");
			VUserLog("	%dx%d",pixelsWide,pixelsHigh);
			VUserLog("	bytesPerRow   = %d",bytesPerRow);
			VUserLog("	bytesPerPixel = %d",bytesPerPixel);
			GLint leftoverBytes = bytesPerRow - bytesPerPixel*pixelsWide;
			VUserLog("	leftoverBytes = %d",leftoverBytes);
			return;
		}
	}
	if (alignment != 4)
		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

	glBindTexture(GL_TEXTURE_2D, glTextureName);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//	VLog("glTexImage2D(GL_TEXTURE_2D, 0, %s, %d, %d, 0, %s, %s, %p);", VuoGl_stringForConstant(internalformat), pixelsWide, pixelsHigh, VuoGl_stringForConstant(format), VuoGl_stringForConstant(glType), pixels);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, pixelsWide, pixelsHigh, 0, format, glType, (GLvoid *)pixels);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (alignment != 4)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (customRowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	// Since returning from this function implies that the texture is ready to use,
	// flush before returning, to ensure that the enqueued commands to create the texture actually get executed
	// before it gets used on a different context.
	glFlushRenderAPPLE();
//	glFinish();

	});
	if (!glTextureName)
		return NULL;

	VuoImage image = VuoImage_make(glTextureName, internalformat, pixelsWide, pixelsHigh);

	dispatch_once(&image->cpuQueueInitialized, ^{});
	image->cpuQueue = dispatch_queue_create("org.vuo.image.cpu", NULL);
	image->cpuData = json_object_new_object();
	char *key = VuoGl_stringForConstant(format);
	if (alignment != 1)
	{
		char *keyWithAlignment;
		asprintf(&keyWithAlignment, "%s alignment=%d", key, alignment);
		free(key);
		key = keyWithAlignment;
	}
	if (customRowLength)
	{
		char *keyWithRowLength;
		asprintf(&keyWithRowLength, "%s rowLength=%d", key, bytesPerRow);
		free(key);
		key = keyWithRowLength;
	}
	json_object *cpuEntry = json_object_new_object();
	json_object_object_add(cpuEntry, "buffer", json_object_new_int64((long long)pixels));
	json_object_object_add(cpuEntry, "freeCallback", json_object_new_int64((long long)Block_copy(freeCallback)));
	json_object_object_add(image->cpuData, key, cpuEntry);
//	VLog("%p: populated %s",image,key);
	free(key);

	return image;
}

/**
 * Downloads the specified image's pixel data from the GPU to the CPU (or returns it from a cache),
 * and returns the CPU memory buffer.
 *
 * The return value is a pointer to a buffer of pixel data.  Row-major, starting at the bottom (flipped).  No row padding.  Premultiplied alpha.
 *
 * The returned buffer is owned by the VuoImage instance; the caller should not modify or `free()` it.
 *
 * @threadAny
 *
 * @param image The image to download.
 * @param requestedFormat An OpenGL format constant.  Supported formats include:
 *    - @c GL_RGB (24 bits per pixel)
 *    - @c GL_BGR (24 bits per pixel)
 *    - @c GL_RGBA (32 bits per pixel)
 *    - @c GL_BGRA (32 bits per pixel)
 *    - @c GL_RGBA16I_EXT (64 bits per pixel)
 *    - @c GL_RGBA16F_ARB (64 bits per pixel)
 *    - @c GL_RGBA32F_ARB (128 bits per pixel)
 *    - @c GL_R16 (16 bits per pixel)
 *    - @c GL_LUMINANCE (8 bits per pixel)
 *    - @c GL_LUMINANCE_ALPHA (16 bits per pixel)
 *    - @c GL_DEPTH_COMPONENT16 (16 bits per pixel)
 */
const unsigned char *VuoImage_getBuffer(VuoImage image, unsigned int requestedFormat)
{
	if (!image)
		return NULL;

	if (image->glInternalFormat == GL_DEPTH_COMPONENT && requestedFormat != GL_DEPTH_COMPONENT16)
	{
		VUserLog("Error: Image has format GL_DEPTH_COMPONENT, which must be fetched as GL_DEPTH_COMPONENT16.");
		return NULL;
	}

	dispatch_once(&image->cpuQueueInitialized, ^{
		image->cpuQueue = dispatch_queue_create("org.vuo.image.cpu", NULL);
		image->cpuData = json_object_new_object();
	});

	__block unsigned char *pixels = NULL;
	dispatch_sync(image->cpuQueue, ^{
		char *key = VuoGl_stringForConstant(requestedFormat);
		struct json_object *value;
		if (json_object_object_get_ex(image->cpuData, key, &value))
		{
			json_object *o;
			json_object_object_get_ex(value, "buffer", &o);
			pixels = (unsigned char *)json_object_get_int64(o);
		}
		else
		{
			unsigned int channels;
			if (requestedFormat == GL_LUMINANCE
			 || requestedFormat == GL_R16
			 || requestedFormat == GL_DEPTH_COMPONENT16)
				channels = 1;
			else if (requestedFormat == GL_LUMINANCE_ALPHA)
				channels = 2;
			else if (requestedFormat == GL_RGB
				  || requestedFormat == GL_BGR)
				channels = 3;
			else if (requestedFormat == GL_RGBA
				  || requestedFormat == GL_BGRA
				  || requestedFormat == GL_RGBA16I_EXT
				  || requestedFormat == GL_RGBA16F_ARB
				  || requestedFormat == GL_RGBA32F_ARB)
				channels = 4;
			else
			{
				VUserLog("Error: Unknown format %s.", VuoGl_stringForConstant(requestedFormat));
				return;
			}

			unsigned int bytesPerChannel = 1;
			GLuint type = GL_UNSIGNED_BYTE;
			if (requestedFormat == GL_RGBA16I_EXT
			 || requestedFormat == GL_R16
			 || requestedFormat == GL_DEPTH_COMPONENT16)
			{
				bytesPerChannel = 2;
				type = GL_UNSIGNED_SHORT;
			}
			else if (requestedFormat == GL_RGBA16F_ARB)
			{
				bytesPerChannel = 2;
				type = GL_HALF_FLOAT_ARB;
			}
			else if (requestedFormat == GL_RGBA32F_ARB)
			{
				bytesPerChannel = 4;
				type = GL_FLOAT;
			}

			GLuint actualFormat = requestedFormat;
			if (requestedFormat == GL_RGBA16I_EXT
			 || requestedFormat == GL_RGBA16F_ARB
			 || requestedFormat == GL_RGBA32F_ARB)
				actualFormat = GL_BGRA;
			else if (requestedFormat == GL_DEPTH_COMPONENT16)
				actualFormat = GL_DEPTH_COMPONENT;
			else if (requestedFormat == GL_R16)
				actualFormat = GL_RED;

			size_t pixelBufferSize = image->pixelsWide * image->pixelsHigh * channels * bytesPerChannel;
			pixels = (unsigned char *)malloc(pixelBufferSize);

			// In the seal, use zeroes for the alpha channel,
			// to lessen the chance that we collide with valid image data.
			const char *tamperEvidentSeal = "Vuo\0Ima\0ge_\0get\0Buf\0fer\0()\0";
			int tamperEvidentSealLength = strlen(tamperEvidentSeal);
			if (pixelBufferSize > tamperEvidentSealLength)
				strlcpy((char *)pixels, tamperEvidentSeal, pixelBufferSize);

			VuoGlContext_perform(^(CGLContextObj cgl_ctx){

				// If each row is quad-aligned, OpenGL doesn't add any padding (good).
				bool openGLAddsPadding = ((image->pixelsWide * channels * bytesPerChannel) % 4 != 0);
				if (openGLAddsPadding)
					// Remove the padding, since VuoImage_getBuffer promises tightly-packed buffers.
					glPixelStorei(GL_PACK_ALIGNMENT, 1);

			glBindTexture(image->glTextureTarget, image->glTextureName);
//			VLog("glGetTexImage(%s, 0, %s, %s, …); on texture internalformat %s", VuoGl_stringForConstant(image->glTextureTarget), VuoGl_stringForConstant(actualFormat), VuoGl_stringForConstant(type), VuoGl_stringForConstant(image->glInternalFormat));
			glGetTexImage(image->glTextureTarget, 0, actualFormat, type, (GLvoid *)pixels);
			glBindTexture(image->glTextureTarget, 0);

				if (openGLAddsPadding)
					// Restore the default.
					glPixelStorei(GL_PACK_ALIGNMENT, 4);

			if (pixelBufferSize > tamperEvidentSealLength && strncmp((char *)pixels, tamperEvidentSeal, strlen(tamperEvidentSeal)) == 0)
			{
				GLenum error = glGetError();
				if (error == GL_NO_ERROR)
					// But as of macOS 10.14.4, calling glGetTexImage on an IOSurface
					// now fills the buffer with garbage (instead of leaving the buffer unmodified),
					// and still doesn't return an error, so we no longer have a way to detect this situation.
					VUserLog("Warning: glGetTexImage() says it was successful, but it didn't actually copy any data.  This might happen if the input texture has an IOSurface bound to it.");
				else
					VUserLog("OpenGL Error: %d", error);
				free(pixels);
				pixels = NULL;
				return;
			}

			});
			if (!pixels)
				return;

			json_object *cpuEntry = json_object_new_object();
			json_object_object_add(cpuEntry, "buffer", json_object_new_int64((long long)pixels));
			json_object_object_add(cpuEntry, "freeCallback", json_object_new_int64((long long)Block_copy( ^(void *buffer){ free(buffer); } )));
			json_object_object_add(image->cpuData, key, cpuEntry);
//			VLog("%p: populated %s",image,key);
		}
		free(key);
	});

	return pixels;
}

/**
 * Returns the image's wrap mode.
 */
VuoImageWrapMode VuoImage_getWrapMode(VuoImage image)
{
	__block GLint wrapModeGL;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){

	glBindTexture(image->glTextureTarget, image->glTextureName);

	glGetTexParameteriv(image->glTextureTarget, GL_TEXTURE_WRAP_S, &wrapModeGL);
	// Ignore GL_TEXTURE_WRAP_T since Vuo assumes it's the same as _S.

	glBindTexture(image->glTextureTarget, 0);

	});

	if (wrapModeGL == GL_CLAMP_TO_EDGE)
		return VuoImageWrapMode_ClampEdge;
	else if (wrapModeGL == GL_REPEAT)
		return VuoImageWrapMode_Repeat;
	else if (wrapModeGL == GL_MIRRORED_REPEAT)
		return VuoImageWrapMode_MirroredRepeat;

	return VuoImageWrapMode_None;
}

/**
 * Changes the image's wrap mode.
 */
void VuoImage_setWrapMode(VuoImage image, VuoImageWrapMode wrapMode)
{
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){

	glBindTexture(image->glTextureTarget, image->glTextureName);

	GLint wrapModeGL = GL_CLAMP_TO_BORDER;
	if (wrapMode == VuoImageWrapMode_ClampEdge)
		wrapModeGL = GL_CLAMP_TO_EDGE;
	else if (wrapMode == VuoImageWrapMode_Repeat)
		wrapModeGL = GL_REPEAT;
	else if (wrapMode == VuoImageWrapMode_MirroredRepeat)
		wrapModeGL = GL_MIRRORED_REPEAT;

	glTexParameteri(image->glTextureTarget, GL_TEXTURE_WRAP_S, wrapModeGL);
	glTexParameteri(image->glTextureTarget, GL_TEXTURE_WRAP_T, wrapModeGL);

	glBindTexture(image->glTextureTarget, 0);

	// Ensure the command queue gets executed before we return,
	// since the VuoShader might immediately be used on another context.
	glFlushRenderAPPLE();

	});
}

/**
 * Makes a solid-color image.
 */
VuoImage VuoImage_makeColorImage(VuoColor color, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	VuoShader shader = VuoShader_makeUnlitColorShader(color);
	VuoRetain(shader);
	VuoImage image = VuoImageRenderer_render(shader, pixelsWide, pixelsHigh, VuoImageColorDepth_8);
	VuoRelease(shader);
	return image;
}

/**
 * Returns a new texture copy of the passed image.
 *
 * @param image Nay be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 * @param flip If true, the copied image is flipped upside-down relative to the source image.
 * @param forcePixelsWide If 0, the output image has the same width as the input image.  Otherwise stretches the image.
 * @param forcePixelsHigh If 0, the output image has the same height as the input image.  Otherwise stretches the image.
 * @param forceAlpha If true, the copied image will have an alpha channel, even if the source image doesn't.
 *
 * @version200Changed{Added `forcePixelsWide`, `forcePixelsHigh`, and `forceAlpha` parameters.}
 */
VuoImage VuoImage_makeCopy(VuoImage image, bool flip, unsigned int forcePixelsWide, unsigned int forcePixelsHigh, bool forceAlpha)
{
	VuoShader shader = NULL;
	if (image->glTextureTarget == GL_TEXTURE_2D)
		shader = VuoShader_makeUnlitAlphaPassthruImageShader(image, flip);
	else if (image->glTextureTarget == GL_TEXTURE_RECTANGLE_ARB)
		shader = VuoShader_makeGlTextureRectangleAlphaPassthruShader(image, flip);
	else
	{
		VUserLog("Error: Unknown glTextureTarget %s", VuoGl_stringForConstant(image->glTextureTarget));
		return NULL;
	}
	VuoRetain(shader);

	if (forceAlpha)
		shader->isTransparent = true;

	VuoImage img = VuoImageRenderer_render(shader,
										   forcePixelsWide ? forcePixelsWide : image->pixelsWide,
										   forcePixelsHigh ? forcePixelsHigh : image->pixelsHigh,
										   VuoImage_getColorDepth(image));

	VuoRelease(shader);

	return img;
}

/**
 * Copies @c image to a new @c GL_TEXTURE_RECTANGLE_ARB texture.
 *
 * @note This is intended for interfacing with host apps; Vuo should always use @c GL_TEXTURE_2D internally.
 */
VuoImage VuoImage_makeGlTextureRectangleCopy(VuoImage image)
{
	VuoShader frag = VuoShader_makeUnlitImageShader(image, 1);
	VuoRetain(frag);

	GLuint textureName = VuoImageRenderer_draw_internal(frag, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image), false, true, 0, NULL);

	VuoImage img = VuoImage_make_internal(textureName, image->glInternalFormat, image->pixelsWide, image->pixelsHigh, NULL, NULL);
	img->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;

	VuoRelease(frag);

	return img;
}

/**
 * Like @ref VuoImage_areEqual(), but permits color channel values to differ by up to `tolerance`.
 */
bool VuoImage_areEqualWithinTolerance(const VuoImage a, const VuoImage b, const unsigned char tolerance)
{
	if (!a && !b)
		return true;
	if (!a || !b)
		return false;

	if (a->pixelsWide != b->pixelsWide
	 || a->pixelsHigh != b->pixelsHigh)
		return false;

	if (a->glTextureName == b->glTextureName)
		return true;

	const unsigned char *aPixels = VuoImage_getBuffer(a, GL_BGRA);
	const unsigned char *bPixels = VuoImage_getBuffer(b, GL_BGRA);

	unsigned char aChannels = VuoGlTexture_getChannelCount(a->glInternalFormat);
	unsigned char bChannels = VuoGlTexture_getChannelCount(b->glInternalFormat);
	if (aChannels == 4 && bChannels == 1)
	{
		// Treat 1-channel red images as equal to opaque greyscale BGRA images.
		for (unsigned int i = 0; i < a->pixelsWide * a->pixelsHigh; ++i)
			if (abs(aPixels[i*4+0] - bPixels[i*4+2]) > tolerance
			 || abs(aPixels[i*4+1] - bPixels[i*4+2]) > tolerance
			 || abs(aPixels[i*4+2] - bPixels[i*4+2]) > tolerance
			 || abs(aPixels[i*4+3] - bPixels[i*4+3]) > tolerance)
			{
				VDebugLog("Difference found at pixel coordinate (%ld,%ld): RGBA %d,%d,%d,%d vs %d,%d,%d,%d",
					i%a->pixelsWide, i/a->pixelsWide,
					aPixels[i*4+2],aPixels[i*4+1],aPixels[i*4+0],aPixels[i*4+3],
					bPixels[i*4+2],bPixels[i*4+2],bPixels[i*4+2],bPixels[i*4+3]);
				return false;
			}
		return true;
	}
	else if (aChannels == 1 && bChannels == 4)
	{
		// Treat 1-channel red images as equal to opaque greyscale BGRA images.
		for (unsigned int i = 0; i < a->pixelsWide * a->pixelsHigh; ++i)
			if (abs(aPixels[i*4+2] - bPixels[i*4+0]) > tolerance
			 || abs(aPixels[i*4+2] - bPixels[i*4+1]) > tolerance
			 || abs(aPixels[i*4+2] - bPixels[i*4+2]) > tolerance
			 || abs(aPixels[i*4+3] - bPixels[i*4+3]) > tolerance)
			{
				VDebugLog("Difference found at pixel coordinate (%ld,%ld): RGBA %d,%d,%d,%d vs %d,%d,%d,%d",
					i%a->pixelsWide, i/a->pixelsWide,
					aPixels[i*4+2],aPixels[i*4+2],aPixels[i*4+2],aPixels[i*4+3],
					bPixels[i*4+2],bPixels[i*4+1],bPixels[i*4+0],bPixels[i*4+3]);
				return false;
			}
		return true;
	}

	for (unsigned int i = 0; i < a->pixelsWide * a->pixelsHigh * 4; ++i)
		if (abs(aPixels[i] - bPixels[i]) > tolerance)
		{
			unsigned int p = (i/4)*4; // Round down to the start of this 32bit pixel.
			VDebugLog("Difference found at pixel coordinate (%ld,%ld): abs(%d - %d) > %d (RGBA %d,%d,%d,%d vs %d,%d,%d,%d)",
				i%a->pixelsWide, i/a->pixelsWide,
				aPixels[i], bPixels[i], tolerance,
				aPixels[p+2],aPixels[p+1],aPixels[p+0],aPixels[p+3],
				bPixels[p+2],bPixels[p+1],bPixels[p+0],bPixels[p+3]);
			return false;
		}

	return true;
}

/**
 * Returns true if both images have the same dimensions and the same RGBA pixel data.
 *
 * Other attributes (pixel format, target) are ignored.
 *
 * Two NULL images are considered equal.
 * A NULL image is never equal to a non-NULL image.
 *
 * This operation is fairly expensive — it requires downloading both images from the GPU to the CPU.
 * It's intended primarily for use in automated tests.
 */
bool VuoImage_areEqual(const VuoImage a, const VuoImage b)
{
	return VuoImage_areEqualWithinTolerance(a,b,0);
}

/**
 * Returns true if the size of `a` is less than the size of `b`.
 *
 * @version200New
 */
bool VuoImage_isLessThan(const VuoImage a, const VuoImage b)
{
	// Treat null images as greater than non-null images,
	// so the more useful non-null images sort to the beginning of the list.
	if (!a || !b)
		return a && !b;

	if (a->pixelsWide < b->pixelsWide) return true;
	if (b->pixelsWide < a->pixelsWide) return false;

	if (a->pixelsHigh < b->pixelsHigh) return true;
	/*if (b->pixelsHigh < a->pixelsHigh)*/ return false;
}

/**
 * - If either image dimension is 0, returns true.
 * - If the image is fully transparent (all pixels have alpha value 0), returns true.
 * - Otherwise returns false.
 */
bool VuoImage_isEmpty(const VuoImage image)
{
	if (!image || image->pixelsWide == 0 || image->pixelsHigh == 0)
		return true;

	const unsigned char *pixels = VuoImage_getBuffer(image, GL_BGRA);
	bool foundSubstantialPixel = false;
	for (unsigned int p = 3; p < image->pixelsWide * image->pixelsHigh * 4; p += 4)
		if (pixels[p])
		{
			foundSubstantialPixel = true;
			break;
		}
	return !foundSubstantialPixel;
}

/**
 * - If the image is null, returns false.
 * - If either image dimension is 0, returns false.
 * - If all pixels are black (whether transparent or opaque), returns true.
 * - Otherwise returns false.
 */
bool VuoImage_isBlackOrTransparent(const VuoImage image, const unsigned char tolerance)
{
	if (!image || image->pixelsWide == 0 || image->pixelsHigh == 0)
		return false;

	const unsigned char *pixels = VuoImage_getBuffer(image, GL_LUMINANCE);
	bool foundNonBlackPixel = false;
	for (unsigned int p = 0; p < image->pixelsWide * image->pixelsHigh; ++p)
		if (pixels[p] > tolerance)
		{
			foundNonBlackPixel = true;
			break;
		}
	return !foundNonBlackPixel;
}

/**
 * - If the image is null, returns false.
 * - If either image dimension is 0, returns false.
 * - Otherwise returns true.
 */
bool VuoImage_isPopulated(const VuoImage image)
{
	return (image && image->pixelsWide && image->pixelsHigh);
}

/**
 * Returns a rectangle (in Vuo Coordinates) at the origin, with width 2 and height matching the image's aspect ratio.
 */
VuoRectangle VuoImage_getRectangle(const VuoImage image)
{
	return VuoRectangle_make(0, 0, 2, 2. * image->pixelsHigh / image->pixelsWide);
}

/**
 * Returns the color depth of the image.
 */
VuoImageColorDepth VuoImage_getColorDepth(const VuoImage image)
{
	if (!image)
		return VuoImageColorDepth_8;

	if (image->glInternalFormat == GL_LUMINANCE8
	 || image->glInternalFormat == GL_LUMINANCE8_ALPHA8
	 || image->glInternalFormat == GL_RGB
	 || image->glInternalFormat == GL_RGBA
	 || image->glInternalFormat == GL_RGBA8
	 || image->glInternalFormat == GL_BGRA
	 || image->glInternalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	 || image->glInternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		return VuoImageColorDepth_8;
	else if (image->glInternalFormat == GL_LUMINANCE16F_ARB
		  || image->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB
		  || image->glInternalFormat == GL_DEPTH_COMPONENT
		  || image->glInternalFormat == GL_RGB16
		  || image->glInternalFormat == GL_RGBA16
		  || image->glInternalFormat == GL_RGB16F_ARB
		  || image->glInternalFormat == GL_RGBA16F_ARB)
		return VuoImageColorDepth_16;
	else if (image->glInternalFormat == GL_LUMINANCE32F_ARB
			 || image->glInternalFormat == GL_LUMINANCE_ALPHA32F_ARB
			 || image->glInternalFormat == GL_RGB32F_ARB
			 || image->glInternalFormat == GL_RGBA32F_ARB)
		return VuoImageColorDepth_32;

	char *formatString = VuoGl_stringForConstant(image->glInternalFormat);
	VUserLog("Error: Unknown glInternalFormat %x (%s)", image->glInternalFormat, formatString);
	free(formatString);
	return VuoImageColorDepth_8;
}

/**
 * @ingroup VuoImage
 * Decodes the JSON object @c js to create a new value.
 *
 * Automatically activates and deactivates a GL Context (if needed to dereference an IOSurface).
 *
 * @threadAny
 *
 * @return If @c js contains valid data, returns a pointer to the VuoImage.  If not, returns NULL.
 *
 * @param js A JSON object containing one of the following schemas:
 *
 *    - a pointer to a @c VuoImage:
 * @eg{
 *	{
 *		"pointer": ...
 *	}
 * }
 *
 *    - an @c IOSurfaceID and the texture's width and height in pixels:
 * @eg{
 *	{
 *		"ioSurface": 42,
 *		"pixelsWide": 640,
 *		"pixelsHigh": 480
 *	}
 * }
 *
 *    - a color and the texture's width and height in pixels:
 * @eg{
 *	{
 *		"color": {"r":0.5, "g":1, "b":0, "a":1},
 *		"pixelsWide": 640,
 *		"pixelsHigh": 480
 *	}
 * }
 */
VuoImage VuoImage_makeFromJson(json_object * js)
{
	return VuoImage_makeFromJsonWithDimensions(js, 0, 0);
}

/**
 * Deletes the image's texture.
 */
static void VuoImage_IOSurfaceTextureFree(VuoImage i)
{
	VuoGlTexturePool_disuse(VuoGlTexturePool_AllocateIOSurface, i->glTextureTarget, i->glInternalFormat, i->pixelsWide, i->pixelsHigh, i->glTextureName);
}

/**
 * @ingroup VuoImage
 * Decodes the JSON object `js` to create a new VuoImage.
 *
 * If the VuoImage's size matches the provided `requestedPixelsWide` x `requestedPixelsHigh`,
 * this method behaves the same as @ref VuoImage_makeFromJson.
 * Otherwise, it stretches the image to the requested size.
 *
 * @threadAny
 * @version200New
 */
VuoImage VuoImage_makeFromJsonWithDimensions(struct json_object *js, unsigned int requestedPixelsWide, unsigned int requestedPixelsHigh)
{
	if (!js)
		return NULL;

	{
		json_object * o;
		if (json_object_object_get_ex(js, "pointer", &o))
		{
			VuoImage im = (VuoImage)json_object_get_int64(o);
			if ((requestedPixelsWide == 0 && requestedPixelsHigh == 0)
			 || (im->pixelsWide == requestedPixelsWide && im->pixelsHigh == requestedPixelsHigh))
				return im;
			else
			{
				VuoImage outputImage = VuoImage_make(im->glTextureName, im->glInternalFormat, requestedPixelsWide, requestedPixelsHigh);
				outputImage->glTextureTarget = im->glTextureTarget;
				outputImage->scaleFactor = im->scaleFactor;
				return outputImage;
			}
		}
	}

	__block unsigned int glInternalFormat = 0;
	unsigned long int pixelsWide;
	unsigned long int pixelsHigh;
	float scaleFactor = 1;

	{
		json_object * o;
		if (json_object_object_get_ex(js, "pixelsWide", &o))
			pixelsWide = json_object_get_int64(o);
		else
			return NULL;
	}
	{
		json_object * o;
		if (json_object_object_get_ex(js, "pixelsHigh", &o))
			pixelsHigh = json_object_get_int64(o);
		else
			return NULL;
	}
	if (pixelsWide == 0 || pixelsHigh == 0)
		return NULL;

	{
		json_object * o;
		if (json_object_object_get_ex(js, "scaleFactor", &o))
			scaleFactor = json_object_get_double(o);
	}

	{
		json_object * o;
		if (json_object_object_get_ex(js, "color", &o))
			return VuoImage_makeColorImage(VuoColor_makeFromJson(o), pixelsWide, pixelsHigh);
	}

	{
		json_object * o;
		if (json_object_object_get_ex(js, "ioSurface", &o))
		{
			IOSurfaceID surfID = json_object_get_int(o);
//			VLog("Converting IOSurfaceID %d",surfID);

			// Read the IOSurface into a GL_TEXTURE_RECTANGLE_ARB (the only texture type IOSurface supports).
			__block IOSurfaceRef surf = NULL;
			__block GLuint textureRect;
			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				glInternalFormat = GL_RGBA;
				surf = IOSurfaceLookup(surfID);
				if (!surf)
				{
					VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
					return;
				}
				textureRect = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, glInternalFormat, pixelsWide, pixelsHigh, GL_BGRA, surf);
				glFlushRenderAPPLE();
			});
			if (!surf)
				return NULL;

			// Convert the GL_TEXTURE_RECTANGLE_ARB into GL_TEXTURE_2D.
			VuoImage image2d;
			{
				VuoImage imageRect = VuoImage_makeClientOwnedGlTextureRectangle(textureRect, glInternalFormat, pixelsWide, pixelsHigh, VuoImage_IOSurfaceTextureFree, NULL);

				imageRect->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;
				VuoLocal(imageRect);

				VuoShader shader = VuoShader_makeGlTextureRectangleShader(imageRect, 1);
				VuoLocal(shader);

				image2d = VuoImageRenderer_render(shader,
												  requestedPixelsWide ? requestedPixelsWide : pixelsWide,
												  requestedPixelsHigh ? requestedPixelsHigh : pixelsHigh,
												  VuoImage_getColorDepth(imageRect));
			}

			VuoIoSurfacePool_signal(surf);
			CFRelease(surf);

			image2d->scaleFactor = scaleFactor;

			return image2d;
		}
	}

	return NULL;
}

/**
 * Decodes the JSON object `js` onto a host-provided OpenGL texture's `GL_TEXTURE_RECTANGLE_ARB` target.
 *
 * @return If `js` contains valid data, returns the OpenGL texture name.  If not, returns 0.
 *
 * @param js An interprocess JSON object (from, e.g., @ref VuoRunner::getPublishedOutputPortValue)
 *           containing an `ioSurface`.
 * @param provider A block that returns an OpenGL texture name with the requested width and height.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param[out] outputPixelsWide Upon return, this contains the width of the output texture.
 * @param[out] outputPixelsHigh Upon return, this contains the height of the output texture.
 * @param[out] outputIOSurface Upon return, this contains the IOSurfaceRef backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurfaceRef:
 *
 *                     VuoIoSurfacePool_signal(outputIOSurface);
 *                     CFRelease(outputIOSurface);
 *
 * @threadAny
 */
GLuint VuoImage_resolveInterprocessJsonUsingTextureProvider(struct json_object *js, GLuint (^provider)(unsigned int pixelsWide, unsigned int pixelsHigh), unsigned int *outputPixelsWide, unsigned int *outputPixelsHigh, void *outputIOSurface)
{
	json_object *o;

	if (!json_object_object_get_ex(js, "pixelsWide", &o))
		return 0;
	*outputPixelsWide = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "pixelsHigh", &o))
		return 0;
	*outputPixelsHigh = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "ioSurface", &o))
		return 0;
	IOSurfaceID surfID = json_object_get_int(o);

	GLuint textureRect = provider(*outputPixelsWide, *outputPixelsHigh);
	if (!textureRect)
		return 0;

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

	IOSurfaceRef *surf = outputIOSurface;
	*surf = IOSurfaceLookup(surfID);
	if (!*surf)
	{
		VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
		return;
	}

	CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, (GLsizei)*outputPixelsWide, (GLsizei)*outputPixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, *surf, 0);
	if(err != kCGLNoError)
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		VUserLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
#pragma clang diagnostic pop
		return;
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glFlushRenderAPPLE();
	});

	return textureRect;
}

static bool VuoImage_resolveInterprocessJsonOntoFramebufferInternal(IOSurfaceRef surf, VuoGlContext context, GLsizei pixelsWide, GLsizei pixelsHigh, bool flip, bool stretch);

/**
 * Decodes the JSON object `js` onto a host-provided OpenGL texture's `GL_TEXTURE_RECTANGLE_ARB` target.
 *
 * @return True if the texture was successfully attached.
 *
 * @param js An interprocess JSON object (from, e.g., @ref VuoRunner::getPublishedOutputPortValue)
 *           containing an `ioSurface`.
 * @param clientTextureName The OpenGL texture name that the image should be attached to.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param pixelsWide The width of `clientTextureName`.
 * @param pixelsHigh The height of `clientTextureName`.
 * @param[out] outputIOSurface Upon return, this contains the IOSurfaceRef backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurfaceRef:
 *
 *                     VuoIoSurfacePool_signal(outputIOSurface);
 *                     CFRelease(outputIOSurface);
 *
 * @threadAny
 * @version200New
 */
bool VuoImage_resolveInterprocessJsonUsingClientTexture(struct json_object *js, GLuint clientTextureName, unsigned int pixelsWide, unsigned int pixelsHigh, void *outputIOSurface)
{
	if (!clientTextureName)
		return false;

	json_object *o;

	if (!json_object_object_get_ex(js, "pixelsWide", &o))
		return false;
	unsigned long inputPixelsWide = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "pixelsHigh", &o))
		return false;
	unsigned long inputPixelsHigh = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "ioSurface", &o))
		return false;
	IOSurfaceID surfID = json_object_get_int(o);

	__block bool success = true;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		IOSurfaceRef *surf = outputIOSurface;
		*surf = IOSurfaceLookup(surfID);
		if (!*surf)
		{
			VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
			success = false;
			return;
		}

		bool shouldResize = (inputPixelsWide != pixelsWide
							 || inputPixelsHigh != pixelsHigh);
		if (shouldResize)
		{
			VuoShader_resetContext(cgl_ctx);

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, clientTextureName);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, pixelsWide, pixelsHigh, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			GLuint outputFramebuffer;
			glGenFramebuffers(1, &outputFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, clientTextureName, 0);

			glViewport(0, 0, pixelsWide, pixelsHigh);

			success = VuoImage_resolveInterprocessJsonOntoFramebufferInternal(*surf, cgl_ctx, inputPixelsWide, inputPixelsHigh, false, true);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, 0, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &outputFramebuffer);
		}
		else
		{
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, clientTextureName);

			CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, (GLsizei)inputPixelsWide, (GLsizei)inputPixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, *surf, 0);
			if (err != kCGLNoError)
			{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
				VUserLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
#pragma clang diagnostic pop
				success = false;
				return;
			}

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		}

		glFlushRenderAPPLE();
	});

	return success;
}

/**
 * Decodes the JSON object `js` onto the framebuffer of a host-provided OpenGL context.
 *
 * @param js An interprocess JSON object (from, e.g., @ref VuoRunner::getPublishedOutputPortValue)
 *           containing an `ioSurface`.
 * @param context A CGLContextObj.  This must be part of the same share group
 *                as the context passed to @ref VuoGlContext_setGlobalRootContext.
 * @param flip If true, the image is flipped upside-down when rendering to the framebuffer.
 * @param stretch How to fit the image to the framebuffer.
 *                If false, the image is drawn 1:1 without scaling/stretching, and the edges are clamped.
 *                If true, the image is stretched to fit the framebuffer's viewport.
 *
 * @threadAnyGL
 * @version200New
 */
bool VuoImage_resolveInterprocessJsonOntoFramebuffer(json_object *js, VuoGlContext context, bool flip, bool stretch)
{
	json_object *o;
	if (!json_object_object_get_ex(js, "pixelsWide", &o))
		return false;
	GLsizei pixelsWide = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "pixelsHigh", &o))
		return false;
	GLsizei pixelsHigh = json_object_get_int64(o);

	if (!json_object_object_get_ex(js, "ioSurface", &o))
		return false;
	IOSurfaceID surfID = json_object_get_int(o);
	IOSurfaceRef surf = IOSurfaceLookup(surfID);
	if (!surf)
	{
		VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
		return false;
	}

	VuoShader_resetContext(context);
	bool ret = VuoImage_resolveInterprocessJsonOntoFramebufferInternal(surf, context, pixelsWide, pixelsHigh, flip, stretch);

	VuoIoSurfacePool_signal(surf);
	CFRelease(surf);
	return ret;
}

/**
 * Helper for VuoImage_resolveInterprocessJsonOntoFramebufferInternal.
 */
static GLuint CompileShader(CGLContextObj cgl_ctx, GLenum type, const char *source)
{
	GLint length = (GLint)strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);

	int infologLength = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 0)
	{
		char *infoLog = (char *)malloc(infologLength);
		int charsWritten = 0;
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		VUserLog("%s", infoLog);
		free(infoLog);
	}
	return shader;
}

/// Allows tests to reinitialize the context.
dispatch_once_t VuoImage_resolveInterprocessJsonOntoFramebufferInternal_init = 0;

/**
 * Helper for VuoImage_resolveInterprocessJsonOntoFramebuffer.
 */
static bool VuoImage_resolveInterprocessJsonOntoFramebufferInternal(IOSurfaceRef surf, VuoGlContext context, GLsizei pixelsWide, GLsizei pixelsHigh, bool flip, bool stretch)
{
	CGLContextObj cgl_ctx = (CGLContextObj)context;

	static bool openGL32Core;
	static GLuint vertexArray;
	static GLuint program;
	static GLuint receiveTextureOffsetAndSizeUniform;
	dispatch_once(&VuoImage_resolveInterprocessJsonOntoFramebufferInternal_init, ^{
		openGL32Core = VuoGlContext_isOpenGL32Core((VuoGlContext)cgl_ctx);

		char *vertexShaderSource;
		char *fragmentShaderSource;
		if (openGL32Core)
		{
			// The following 2 `gl*VertexArrays` calls use the thread-local context (not CGLMacro).
			CGLSetCurrentContext(cgl_ctx);

			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);

			vertexShaderSource = VUOSHADER_GLSL_SOURCE(150,
				in vec2 position;
				in vec2 textureCoordinate;
				out vec2 fragmentTextureCoordinate;
				void main()
				{
					fragmentTextureCoordinate = textureCoordinate;
					gl_Position = vec4(position.x, position.y, 0., 1.);
				}
			);
			fragmentShaderSource = VUOSHADER_GLSL_SOURCE(150,
				uniform sampler2DRect receiveTexture;
				uniform vec4 receiveTextureOffsetAndSize;
				in vec2 fragmentTextureCoordinate;
				out vec4 FragColor;
				void main()
				{
					FragColor = texture(receiveTexture, receiveTextureOffsetAndSize.xy + fragmentTextureCoordinate * receiveTextureOffsetAndSize.zw);
				}
			);
		}
		else
		{
			// OpenGL 2.1 context.

			glGenVertexArraysAPPLE(1, &vertexArray);
			glBindVertexArrayAPPLE(vertexArray);

			vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
				attribute vec2 position;
				attribute vec2 textureCoordinate;
				varying vec2 fragmentTextureCoordinate;
				void main()
				{
					fragmentTextureCoordinate = textureCoordinate;
					gl_Position = vec4(position.x, position.y, 0., 1.);
				}
			);
			fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
				uniform sampler2DRect receiveTexture;
				uniform vec4 receiveTextureOffsetAndSize;
				varying vec2 fragmentTextureCoordinate;
				void main()
				{
					gl_FragColor = texture2DRect(receiveTexture, receiveTextureOffsetAndSize.xy + fragmentTextureCoordinate * receiveTextureOffsetAndSize.zw);
				}
			);
		}


		const GLfloat quadPositionsAndTextureCoordinates[] = {
		//   X   Y  U  V
			-1, -1, 0, 0,
			 1, -1, 1, 0,
			-1,  1, 0, 1,

			 1,  1, 1, 1,
			-1,  1, 0, 1,
			 1, -1, 1, 0,
		};
		GLuint quadPTCBuffer;
		glGenBuffers(1, &quadPTCBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, quadPTCBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositionsAndTextureCoordinates), quadPositionsAndTextureCoordinates, GL_STATIC_DRAW);
		VuoGlPool_logVRAMAllocated(sizeof(quadPositionsAndTextureCoordinates));

		GLuint   vertexShader = CompileShader(context, GL_VERTEX_SHADER,     vertexShaderSource);
		GLuint fragmentShader = CompileShader(context, GL_FRAGMENT_SHADER, fragmentShaderSource);
		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		GLuint positionAttribute = glGetAttribLocation(program, "position");
		GLuint textureCoordinateAttribute = glGetAttribLocation(program, "textureCoordinate");
		GLuint receiveTextureUniform = glGetUniformLocation(program, "receiveTexture");
		receiveTextureOffsetAndSizeUniform = glGetUniformLocation(program, "receiveTextureOffsetAndSize");

		glUseProgram(program);

		glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);
		glEnableVertexAttribArray(positionAttribute);

		glVertexAttribPointer(textureCoordinateAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)(sizeof(GLfloat)*2));
		glEnableVertexAttribArray(textureCoordinateAttribute);

		glUniform1i(receiveTextureUniform, 0);
	});


	if (openGL32Core)
	{
		CGLSetCurrentContext(cgl_ctx);
		glBindVertexArray(vertexArray);
	}
	else
		glBindVertexArrayAPPLE(vertexArray);

	GLuint textureRect = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, pixelsWide, pixelsHigh, GL_BGRA, surf);
	if (!textureRect)
	{
		VUserLog("Error: Couldn't allocate texture.");
		VGL();
		return false;
	}
	VuoGlTexture_retain(textureRect, NULL, NULL);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

	glUseProgram(program);

	if (stretch)
		glUniform4f(receiveTextureOffsetAndSizeUniform, 0, flip ? pixelsHigh : 0, pixelsWide, pixelsHigh * (flip ? -1 : 1));
	else
	{
		// Center the image in the viewport.
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
//		VLog("Resolving %dx%d image onto a %dx%d viewport.", pixelsWide, pixelsHigh, viewport[2], viewport[3]);
		glUniform4f(receiveTextureOffsetAndSizeUniform,
					((float)pixelsWide - viewport[2]) / 2,
					flip ? viewport[3] : 0,
					viewport[2],
					viewport[3] * (flip ? -1 : 1));
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	VuoGlTexture_release(VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, pixelsWide, pixelsHigh, textureRect);
	glUseProgram(0);
	if (openGL32Core)
		glBindVertexArray(0);
	else
		glBindVertexArrayAPPLE(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

/**
 * @ingroup VuoImage
 * Encodes @c value as a JSON object.
 *
 * @threadAny
 */
json_object * VuoImage_getJson(const VuoImage value)
{
	if (!value)
		return NULL;

	json_object *js = json_object_new_object();
	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value));
	return js;
}

/**
 * @ingroup VuoImage
 * Returns a JSON object containing an interprocess handle for the specified texture.
 *
 * @c value may be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
json_object * VuoImage_getInterprocessJson(const VuoImage value)
{
	if (!value)
		return NULL;

	__block VuoShader shader = NULL;
	__block IOSurfaceID surfID = 0;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
//		VLog("Creating an IOSurface from glTextureName %d on target %lu",value->glTextureName,value->glTextureTarget);

		if (value->glTextureTarget == GL_TEXTURE_2D)
			shader = VuoShader_makeUnlitAlphaPassthruImageShader(value, false);
		else if (value->glTextureTarget == GL_TEXTURE_RECTANGLE_ARB)
			shader = VuoShader_makeGlTextureRectangleAlphaPassthruShader(value, false);
		VuoRetain(shader);

		surfID = VuoImageRenderer_draw_internal(shader, value->pixelsWide, value->pixelsHigh, VuoImage_getColorDepth(value), true, true, 0, NULL);
//		VLog("Created IOSurfaceID %d",surfID);

		// Ensure the command queue gets executed before we return,
		// since the IOSurface might immediately be used on another context.
		glFlushRenderAPPLE();
	});
	if (!surfID)
		return NULL;

	json_object * js = json_object_new_object();
	{
		json_object * o = json_object_new_int(surfID);
		json_object_object_add(js, "ioSurface", o);
	}

	{
		json_object * o = json_object_new_int64(value->pixelsWide);
		json_object_object_add(js, "pixelsWide", o);
	}
	{
		json_object * o = json_object_new_int64(value->pixelsHigh);
		json_object_object_add(js, "pixelsHigh", o);
	}
	{
		json_object *o = json_object_new_double(value->scaleFactor);
		json_object_object_add(js, "scaleFactor", o);
	}
	// VuoShader_makeUnlitImageShader retains the image; VuoRelease(shader) then releases it.
	// So don't release the shader until we're done with the image, in case this release is its last.
	VuoRelease(shader);

	return js;
}

/**
 * @ingroup VuoImage
 * A brief summary of the contents of this texture.
 *
 * @threadAny
 *
 * @eg{640x480 pixels @ 1x
 * RGB, each channel 8-bit unsigned integer
 * OpenGL: GL_TEXTURE_2D, GL_RGB, ID 42}
 */
char * VuoImage_getSummary(const VuoImage value)
{
	if (!value)
		return strdup("No image");

	const char *type;
	switch (value->glInternalFormat)
	{
		case GL_RGB:                    type = "RGB, each channel 8-bit unsigned integer"; break;
		case GL_RGB16F_ARB:             type = "RGB, each channel 16-bit signed float"; break;
		case GL_RGB32F_ARB:             type = "RGB, each channel 32-bit signed float"; break;
		case GL_RGBA:                   type = "RGBA, each channel 8-bit unsigned integer"; break;
		case GL_RGBA16F_ARB:            type = "RGBA, each channel 16-bit signed float"; break;
		case GL_RGBA32F_ARB:            type = "RGBA, each channel 32-bit signed float"; break;
		case GL_LUMINANCE8:             type = "intensity, 8-bit unsigned integer"; break;
		case GL_LUMINANCE16F_ARB:       type = "intensity, 16-bit signed float"; break;
		case GL_LUMINANCE32F_ARB:       type = "intensity, 32-bit signed float"; break;
		case GL_LUMINANCE8_ALPHA8:      type = "intensity+alpha, each channel 8-bit unsigned integer"; break;
		case GL_LUMINANCE_ALPHA16F_ARB: type = "intensity+alpha, each channel 16-bit signed float"; break;
		case GL_LUMINANCE_ALPHA32F_ARB: type = "intensity+alpha, each channel 32-bit signed float"; break;
		case GL_DEPTH_COMPONENT:        type = "intensity, 16-bit signed float"; break;
		default:                        type = "(unknown)";
	}

	char *target         = VuoGl_stringForConstant(value->glTextureTarget);
	char *internalformat = VuoGl_stringForConstant(value->glInternalFormat);

	char *summary = VuoText_format("<div>%lu×%lu pixels @ %gx</div>\n<div>%s</div>\n<div>OpenGL: %s, %s, ID %u</div>",
		value->pixelsWide, value->pixelsHigh,
		value->scaleFactor,
		type,
		target,
		internalformat,
		value->glTextureName);

	free(internalformat);
	free(target);

	return summary;
}
