/**
 * @file
 * VuoImage implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
#include <OpenGL/CGLIOSurface.h>


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
	if (t->cpuQueueInitialized)
	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
		glBindTexture(t->glTextureTarget, t->glTextureName);
		GLenum format = GL_BGRA;
		if (t->glInternalFormat == GL_DEPTH_COMPONENT)
			format = GL_DEPTH_COMPONENT;
		GLenum type = VuoGlTexture_getType(format);
//		VLog("glTexImage2D(%s, 0, %s, %ld, %ld, 0, %s, %s, NULL);", VuoGl_stringForConstant(t->glTextureTarget), VuoGl_stringForConstant(t->glInternalFormat), t->pixelsWide, t->pixelsHigh, VuoGl_stringForConstant(format), VuoGl_stringForConstant(type));
		glTexImage2D(t->glTextureTarget, 0, t->glInternalFormat, t->pixelsWide, t->pixelsHigh, 0, format, type, NULL);
		glBindTexture(t->glTextureTarget, 0);
		VuoGlContext_disuse(cgl_ctx);
	}

	VuoGlTexture_release(t->glInternalFormat, t->pixelsWide, t->pixelsHigh, t->glTextureName, t->glTextureTarget);

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
	if (!pixelsWide || !pixelsHigh)
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
	if (!freeCallback)
	{
		VUserLog("Error: freeCallback may not be NULL.");
		return NULL;
	}

	if (!pixelsWide || !pixelsHigh)
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
 * @param colorDepth The number of bits per channel.  When 8bpc, `pixels` should be `unsigned char *`; when 16bpc, `pixels` should be `float *`.
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
	if (!pixels || !pixelsWide || !pixelsHigh)
		return NULL;

	static bool freeCallbackWarningEmitted = false;
	if (!freeCallback)
	{
		if (!freeCallbackWarningEmitted)
		{
			freeCallbackWarningEmitted = true;
			VUserLog("VuoImage_makeFromBuffer() and VuoImage_makeFromBufferWithStride() now take ownership of `pixels`, and therefore `freeCallback` is required.  Since there's no `freeCallback`, I'm giving up and outputting an empty image.  Please update the plugin listed in the backtrace below.");
			VuoLog_backtrace();
		}
		return NULL;
	}

	VuoGlContext glContext = VuoGlContext_use();
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	GLenum internalformat = VuoImageColorDepth_getGlInternalFormat(format, colorDepth);
//	VLog("Using format=%s -> internalformat=%s", VuoGl_stringForConstant(format), VuoGl_stringForConstant(internalformat));
	GLuint glTextureName = VuoGlTexturePool_use(glContext, internalformat, pixelsWide, pixelsHigh, format);
	if (!glTextureName)
		return NULL;

	int bytesPerPixel = VuoGlTexture_getChannelCount(format);

	GLuint glType = (colorDepth==VuoImageColorDepth_8 ? VuoGlTexture_getType(format) : GL_FLOAT);
	if (glType == GL_FLOAT)
		bytesPerPixel *= 4;

	bool customRowLength = false;
	if (bytesPerRow && (bytesPerRow != bytesPerPixel * pixelsWide))
	{
		if (bytesPerRow % bytesPerPixel == 0)
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
			return NULL;
		}
	}

	glBindTexture(GL_TEXTURE_2D, glTextureName);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//	VLog("glTexImage2D(GL_TEXTURE_2D, 0, %s, %d, %d, 0, %s, %s, %p);", VuoGl_stringForConstant(internalformat), pixelsWide, pixelsHigh, VuoGl_stringForConstant(format), VuoGl_stringForConstant(glType), pixels);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, pixelsWide, pixelsHigh, 0, format, glType, (GLvoid *)pixels);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (customRowLength)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	// Since returning from this function implies that the texture is ready to use,
	// flush before returning, to ensure that the enqueued commands to create the texture actually get executed
	// before it gets used on a different context.
	glFlushRenderAPPLE();
//	glFinish();

	VuoGlContext_disuse(glContext);

	VuoImage image = VuoImage_make(glTextureName, internalformat, pixelsWide, pixelsHigh);

	dispatch_once(&image->cpuQueueInitialized, ^{});
	image->cpuQueue = dispatch_queue_create("org.vuo.image.cpu", NULL);
	image->cpuData = json_object_new_object();
	char *key = VuoGl_stringForConstant(format);
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
 * The return value is a pointer to a buffer of pixel data.  8 bits per channel.  Row-major, starting at the bottom (flipped).  Zero stride.  Premultiplied alpha.
 *
 * The returned buffer is owned by the VuoImage instance; the caller should not modify or `free()` it.
 *
 * @threadAny
 *
 * @param image The image to download.
 * @param requestedFormat An OpenGL format constant.  Supported formats include:
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
				strcpy((char *)pixels, tamperEvidentSeal);

			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

			glBindTexture(image->glTextureTarget, image->glTextureName);
//			VLog("glGetTexImage(%s, 0, %s, %s, …); on texture internalformat %s", VuoGl_stringForConstant(image->glTextureTarget), VuoGl_stringForConstant(actualFormat), VuoGl_stringForConstant(type), VuoGl_stringForConstant(image->glInternalFormat));
			glGetTexImage(image->glTextureTarget, 0, actualFormat, type, (GLvoid *)pixels);
			glBindTexture(image->glTextureTarget, 0);

			VuoGlContext_disuse(cgl_ctx);

			if (pixelBufferSize > tamperEvidentSealLength && strncmp((char *)pixels, tamperEvidentSeal, strlen(tamperEvidentSeal)) == 0)
			{
				GLenum error = glGetError();
				if (error == GL_NO_ERROR)
					VUserLog("Warning: glGetTexImage() says it was successful, but it didn't actually copy any data.  This might happen if the input texture has an IOSurface bound to it.");
				else
					VUserLog("OpenGL Error: %d", error);
				free(pixels);
				pixels = NULL;
				return;
			}

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
	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

	glBindTexture(image->glTextureTarget, image->glTextureName);

	GLint wrapModeGL;
	glGetTexParameteriv(image->glTextureTarget, GL_TEXTURE_WRAP_S, &wrapModeGL);
	// Ignore GL_TEXTURE_WRAP_T since Vuo assumes it's the same as _S.

	glBindTexture(image->glTextureTarget, 0);

	VuoGlContext_disuse(cgl_ctx);

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
	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

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

	VuoGlContext_disuse(cgl_ctx);
}

/**
 * Deletes @c image's GL texture.
 */
static void VuoImage_deleteImage(VuoImage image)
{
	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
	glDeleteTextures(1, &image->glTextureName);
	VuoGlContext_disuse(cgl_ctx);
}

/**
 * Makes a solid-color image.
 */
VuoImage VuoImage_makeColorImage(VuoColor color, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer imageRenderer = VuoImageRenderer_make(glContext);
	VuoRetain(imageRenderer);
	VuoShader shader = VuoShader_makeUnlitColorShader(color);
	VuoRetain(shader);
	VuoImage image = VuoImageRenderer_draw(imageRenderer, shader, pixelsWide, pixelsHigh, VuoImageColorDepth_8);
	VuoRelease(shader);
	VuoRelease(imageRenderer);
	VuoGlContext_disuse(glContext);
	return image;
}

/**
 * Returns a new texture copy of the passed image.
 *
 * @c image may be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * If `flip` is true, the copied image is flipped upside-down relative to the source image.
 */
VuoImage VuoImage_makeCopy(VuoImage image, bool flip)
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

	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer renderer = VuoImageRenderer_make(glContext);
	VuoRetain(renderer);

	VuoImage img = VuoImageRenderer_draw(renderer, shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoRelease(shader);
	VuoRelease(renderer);
	VuoGlContext_disuse(glContext);

	return img;
}

/**
 * Copies @c image to a new @c GL_TEXTURE_RECTANGLE_ARB texture.
 *
 * @note This is intended for interfacing with host apps; Vuo should always use @c GL_TEXTURE_2D internally.
 */
VuoImage VuoImage_makeGlTextureRectangleCopy(VuoImage image)
{
	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer renderer = VuoImageRenderer_make(glContext);
	VuoRetain(renderer);

	VuoShader frag = VuoShader_makeUnlitImageShader(image, 1);
	VuoRetain(frag);

	GLuint textureName = VuoImageRenderer_draw_internal(renderer, frag, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image), false, true, 0);
	VuoImage img = VuoImage_makeClientOwnedGlTextureRectangle(textureName, image->glInternalFormat, image->pixelsWide, image->pixelsHigh, VuoImage_deleteImage, NULL);

	VuoRelease(frag);
	VuoRelease(renderer);
	VuoGlContext_disuse(glContext);

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
	 || image->glInternalFormat == GL_BGRA)
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
	if (!js)
		return NULL;

	{
		json_object * o;
		if (json_object_object_get_ex(js, "pointer", &o))
			return (VuoImage)json_object_get_int64(o);
	}

	unsigned int glTextureName;
	unsigned int glInternalFormat = 0;
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
			IOSurfaceRef surf;
			GLuint textureRect;
			VuoGlContext glContext = VuoGlContext_use();
			CGLContextObj cgl_ctx = (CGLContextObj)glContext;
			{
				{
					glGenTextures(1, &textureRect);
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					surf = IOSurfaceLookup(surfID);
					if (!surf)
					{
						VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
						return NULL;
					}
					glInternalFormat = GL_RGBA;
					CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, glInternalFormat, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
					if(err != kCGLNoError)
					{
						VUserLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
						return NULL;
					}
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				}
			}
			glFlushRenderAPPLE();

			// Convert the GL_TEXTURE_RECTANGLE_ARB into GL_TEXTURE_2D.
			VuoImage image2d;
			{
				VuoImage imageRect = VuoImage_makeClientOwnedGlTextureRectangle(textureRect, glInternalFormat, pixelsWide, pixelsHigh, VuoImage_deleteImage, NULL);
				VuoLocal(imageRect);

				VuoShader shader = VuoShader_makeGlTextureRectangleShader(imageRect, 1);
				VuoLocal(shader);

				VuoImageRenderer ir = VuoImageRenderer_make(cgl_ctx);
				VuoLocal(ir);
				image2d = VuoImageRenderer_draw(ir, shader, pixelsWide, pixelsHigh, VuoImage_getColorDepth(imageRect));
			}

			VuoIoSurfacePool_signal(surf);
			CFRelease(surf);

			VuoGlContext_disuse(glContext);

			image2d->scaleFactor = scaleFactor;

			return image2d;
		}
		else
			return NULL;
	}

	VuoImage i = VuoImage_make(glTextureName, glInternalFormat, pixelsWide, pixelsHigh);
	i->scaleFactor = scaleFactor;
	return i;
}

/**
 * Decodes the JSON object `js` onto a host-provided OpenGL texture's `GL_TEXTURE_RECTANGLE_ARB` target.
 *
 * @return If `js` contains valid data, returns the OpenGL texture name.  If not, returns 0.
 *
 * @param js An interprocess JSON object containing an `ioSurface`.
 * @param provider A block that returns an OpenGL texture name with the requested width and height.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param[out] outputPixelsWide Upon return, this contains the width of the output texture.
 * @param[out] outputPixelsHigh Upon return, this contains the height of the output texture.
 * @param[out] outputIOSurface Upon return, this contains the IOSurface backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurface:
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

	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

	IOSurfaceRef *surf = outputIOSurface;
	*surf = IOSurfaceLookup(surfID);
	if (!*surf)
	{
		VUserLog("Error: IOSurfaceLookup(%d) failed.", surfID);
		return 0;
	}

	CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, (GLsizei)*outputPixelsWide, (GLsizei)*outputPixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, *surf, 0);
	if(err != kCGLNoError)
	{
		VUserLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
		return 0;
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glFlushRenderAPPLE();

	VuoGlContext_disuse(cgl_ctx);
	return textureRect;

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

	json_object * js = json_object_new_object();

	VuoShader shader = NULL;
	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
//		VLog("Creating an IOSurface from glTextureName %d on target %lu",value->glTextureName,value->glTextureTarget);

		if (value->glTextureTarget == GL_TEXTURE_2D)
			shader = VuoShader_makeUnlitAlphaPassthruImageShader(value, false);
		else if (value->glTextureTarget == GL_TEXTURE_RECTANGLE_ARB)
			shader = VuoShader_makeGlTextureRectangleAlphaPassthruShader(value, false);
		VuoRetain(shader);

		VuoImageRenderer ir = VuoImageRenderer_make(cgl_ctx);
		VuoRetain(ir);
		IOSurfaceID surfID = VuoImageRenderer_draw_internal(ir, shader, value->pixelsWide, value->pixelsHigh, VuoImage_getColorDepth(value), true, true, 0);
		VuoRelease(ir);
//		VLog("Created IOSurfaceID %d",surfID);

		json_object * o = json_object_new_int(surfID);
		json_object_object_add(js, "ioSurface", o);

		// Ensure the command queue gets executed before we return,
		// since the IOSurface might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
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
 * @eg{GL Texture (ID 42)
 * 640x480}
 */
char * VuoImage_getSummary(const VuoImage value)
{
	if (!value)
		return strdup("(no image)");

	char *format;
	switch (value->glInternalFormat)
	{
		case GL_RGB:					format = strdup("RGB, each channel stored as 8-bit unsigned integer (GL_RGB)"); break;
		case GL_RGB16F_ARB:				format = strdup("RGB, each channel stored as 16-bit signed float (GL_RGB16F_ARB)"); break;
		case GL_RGB32F_ARB:				format = strdup("RGB, each channel stored as 32-bit signed float (GL_RGB32F_ARB)"); break;
		case GL_RGBA:					format = strdup("RGBA, each channel stored as 8-bit unsigned integer (GL_RGBA)"); break;
		case GL_RGBA16F_ARB:			format = strdup("RGBA, each channel stored as 16-bit signed float (GL_RGBA16F_ARB)"); break;
		case GL_RGBA32F_ARB:			format = strdup("RGBA, each channel stored as 32-bit signed float (GL_RGBA32F_ARB)"); break;
		case GL_LUMINANCE8:				format = strdup("intensity, stored as 8-bit unsigned integer (GL_LUMINANCE8)"); break;
		case GL_LUMINANCE16F_ARB:		format = strdup("intensity, stored as 16-bit signed float (GL_LUMINANCE16F_ARB)"); break;
		case GL_LUMINANCE32F_ARB:		format = strdup("intensity, stored as 32-bit signed float (GL_LUMINANCE32F_ARB)"); break;
		case GL_LUMINANCE8_ALPHA8:		format = strdup("intensity+alpha, each channel stored as 8-bit unsigned integer (GL_LUMINANCE8_ALPHA8)"); break;
		case GL_LUMINANCE_ALPHA16F_ARB:	format = strdup("intensity+alpha, each channel stored as 16-bit signed float (GL_LUMINANCE_ALPHA16F_ARB)"); break;
		case GL_LUMINANCE_ALPHA32F_ARB:	format = strdup("intensity+alpha, each channel stored as 32-bit signed float (GL_LUMINANCE_ALPHA32F_ARB)"); break;
		case GL_DEPTH_COMPONENT:		format = strdup("intensity, stored as 16-bit signed float (GL_DEPTH_COMPONENT)"); break;
		default:						format = VuoGl_stringForConstant(value->glInternalFormat);
	}

	char *summary = VuoText_format("GL Texture (ID %u)<br>Size: %lux%lu pixels<br>Scale Factor: %gx<br>Type: %s",
		value->glTextureName,
		value->pixelsWide, value->pixelsHigh,
		value->scaleFactor,
		format);

	free(format);

	return summary;
}
