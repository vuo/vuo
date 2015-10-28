/**
 * @file
 * VuoImage implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
//	VLog("Freeing image %p %s",t,VuoImage_summaryFromValue(t));

	if (t->freeCallback)
		t->freeCallback(t);
	else
		VuoGlTexture_release(t->glInternalFormat, t->pixelsWide, t->pixelsHigh, t->glTextureName);

	free(t);
}

/**
 * Helper for @c VuoImage_make and @c VuoImage_makeClientOwned.
 */
VuoImage VuoImage_make_internal(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh)
{
	VuoImage t = (VuoImage)malloc(sizeof(struct _VuoImage));
	VuoRegister(t, VuoImage_free);

	t->glTextureName = glTextureName;
	t->glTextureTarget = GL_TEXTURE_2D;
	t->glInternalFormat = glInternalFormat;
	t->pixelsWide = pixelsWide;
	t->pixelsHigh = pixelsHigh;

	t->freeCallback = NULL;
	t->freeCallbackContext = NULL;

//	VLog("Made image %p %s",t,VuoImage_summaryFromValue(t));
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_2D.
 *    - use wrap mode GL_CLAMP_TO_BORDER on the S and T axes
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

	VuoImage t = VuoImage_make_internal(glTextureName, glInternalFormat, pixelsWide, pixelsHigh);
	VuoGlTexture_retain(glTextureName);
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_2D
 *    - use wrap mode GL_CLAMP_TO_BORDER on the S and T axes
 *    - use minifying and magnification filter GL_LINEAR
 *
 * When the VuoImage is no longer needed, @c freeCallback is called.
 * The @c freeCallback may then activate a GL context and delete the texture, or send it back to a texture pool.
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
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	VuoImage t = VuoImage_make_internal(glTextureName, glInternalFormat, pixelsWide, pixelsHigh);
	t->freeCallback = freeCallback;
	t->freeCallbackContext = freeCallbackContext;
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must:
 *
 *    - be of type @c GL_TEXTURE_RECTANGLE_ARB
 *    - use wrap mode GL_CLAMP_TO_BORDER on the S and T axes
 *    - use minifying and magnification filter GL_LINEAR
 *
 * When the VuoImage is no longer needed, @c freeCallback is called.
 * The @c freeCallback may then activate a GL context and delete the texture, or send it back to a texture pool.
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
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	VuoImage t = VuoImage_make_internal(glTextureName, glInternalFormat, pixelsWide, pixelsHigh);
	t->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;
	t->freeCallback = freeCallback;
	t->freeCallbackContext = freeCallbackContext;
	return t;
}

/**
 * @ingroup VuoImage
 * Uploads the specified pixel data to the GPU and returns a new @ref VuoImage referencing it.
 *
 * @threadAny
 *
 * @param pixels Pointer to a buffer of pixel data.  Row-major, starting at the bottom (flipped).  Zero stride.  Premultiplied alpha.  The caller is responsible for freeing this buffer sometime after this function returns.
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
 */
VuoImage VuoImage_makeFromBuffer(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth colorDepth)
{
	if (!pixelsWide || !pixelsHigh)
		return NULL;

	VuoGlContext glContext = VuoGlContext_use();
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	GLenum internalformat = VuoImageColorDepth_getGlInternalFormat(format, colorDepth);
//	VLog("Using format=%s -> internalformat=%s", VuoGl_stringForConstant(format), VuoGl_stringForConstant(internalformat));
	GLuint glTextureName = VuoGlTexturePool_use(glContext, internalformat, pixelsWide, pixelsHigh, format);

	GLuint glType = (colorDepth==VuoImageColorDepth_8 ? VuoGlTexture_getType(format) : GL_FLOAT);
	glBindTexture(GL_TEXTURE_2D, glTextureName);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixelsWide, pixelsHigh, format, glType, (GLvoid *)pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Since returning from this function implies that the texture is ready to use,
	// flush before returning, to ensure that the enqueued commands to create the texture actually get executed
	// before it gets used on a different context.
	glFlushRenderAPPLE();
//	glFinish();

	VuoGlContext_disuse(glContext);

	return VuoImage_make(glTextureName, internalformat, pixelsWide, pixelsHigh);
}

/**
 * Downloads the specified image's pixel data from the GPU to the CPU, and returns the CPU memory buffer.
 *
 * The return value is a pointer to a buffer of pixel data.  8 bits per channel.  Row-major, starting at the bottom (flipped).  Zero stride.  Premultiplied alpha.
 *
 * The caller should @c free() the returned buffer when it is finished using it.
 *
 * @threadAny
 *
 * @param image The image to download.
 * @param requestedFormat An OpenGL format constant.  Supported formats include:
 *    - @c GL_RGBA (32 bits per pixel)
 *    - @c GL_BGRA (32 bits per pixel)
 *    - @c GL_LUMINANCE (8 bits per pixel)
 *    - @c GL_LUMINANCE_ALPHA (16 bits per pixel)
 */
unsigned char *VuoImage_copyBuffer(VuoImage image, unsigned int requestedFormat)
{
	if (!image)
		return NULL;

	unsigned int channels;
	if (requestedFormat == GL_LUMINANCE)
		channels = 1;
	else if (requestedFormat == GL_LUMINANCE_ALPHA)
		channels = 2;
	else if (requestedFormat == GL_RGBA || requestedFormat == GL_BGRA)
		channels = 4;
	else
	{
		VLog("Error: Unknown format %d", requestedFormat);
		return NULL;
	}
	unsigned char *pixels = (unsigned char *)malloc(image->pixelsWide * image->pixelsHigh * channels);

	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

	glBindTexture(image->glTextureTarget, image->glTextureName);
	glGetTexImage(image->glTextureTarget, 0, requestedFormat, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
	glBindTexture(image->glTextureTarget, 0);

	VuoGlContext_disuse(cgl_ctx);

	return pixels;
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

	glBindTexture(GL_TEXTURE_2D, 0);

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
 */
VuoImage VuoImage_makeCopy(VuoImage image)
{
	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer renderer = VuoImageRenderer_make(glContext);
	VuoRetain(renderer);

	VuoShader shader = NULL;
	if (image->glTextureTarget == GL_TEXTURE_2D)
		shader = VuoShader_makeUnlitImageShader(image, 1);
	else if (image->glTextureTarget == GL_TEXTURE_RECTANGLE_ARB)
		shader = VuoShader_makeGlTextureRectangleShader(image, 1);
	VuoRetain(shader);

	VuoImage img = VuoImageRenderer_draw(renderer, shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoRelease(shader);
	VuoRelease(renderer);
	VuoGlContext_disuse(glContext);

	return img;
}

/**
 * Deletes the GL texture.
 */
static void VuoImage_makeGlTextureRectangleCopy_freeCallback(VuoImage imageToFree)
{
	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
	glDeleteTextures(1, &imageToFree->glTextureName);
	VuoGlContext_disuse(cgl_ctx);
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

	GLuint textureName = VuoImageRenderer_draw_internal(renderer, frag, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image), false, true);
	VuoImage img = VuoImage_makeClientOwnedGlTextureRectangle(textureName, image->glInternalFormat, image->pixelsWide, image->pixelsHigh, VuoImage_makeGlTextureRectangleCopy_freeCallback, NULL);

	VuoRelease(frag);
	VuoRelease(renderer);
	VuoGlContext_disuse(glContext);

	return img;
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
	if (!a && !b)
		return true;
	if (!a || !b)
		return false;

	if (a->pixelsWide != b->pixelsWide
	 || a->pixelsHigh != b->pixelsHigh)
		return false;

	unsigned char *aPixels = VuoImage_copyBuffer(a, GL_RGBA);
	unsigned char *bPixels = VuoImage_copyBuffer(b, GL_RGBA);
	for (unsigned int i = 0; i < a->pixelsWide * a->pixelsHigh * 4; ++i)
		if (aPixels[i] != bPixels[i])
		{
			free(aPixels);
			free(bPixels);
			return false;
		}

	free(aPixels);
	free(bPixels);
	return true;
}

/**
 * - If either image dimension is 0, returns true.
 * - If the image is fully transparent (all pixels have alpha value 0), returns true.
 * - Otherwise returns false.
 */
bool VuoImage_isEmpty(const VuoImage image)
{
	if (image->pixelsWide == 0 || image->pixelsHigh == 0)
		return true;

	unsigned char *pixels = VuoImage_copyBuffer(image, GL_RGBA);
	bool foundSubstantialPixel = false;
	for (unsigned int p = 3; p < image->pixelsWide * image->pixelsHigh * 4; p += 4)
		if (pixels[p])
		{
			foundSubstantialPixel = true;
			break;
		}
	free(pixels);
	return !foundSubstantialPixel;
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
	 || image->glInternalFormat == GL_RGBA)
		return VuoImageColorDepth_8;
	else if (image->glInternalFormat == GL_LUMINANCE16F_ARB
		  || image->glInternalFormat == GL_LUMINANCE_ALPHA16F_ARB
		  || image->glInternalFormat == GL_RGB16
		  || image->glInternalFormat == GL_RGBA16
		  || image->glInternalFormat == GL_RGB16F_ARB
		  || image->glInternalFormat == GL_RGBA16F_ARB)
		return VuoImageColorDepth_16;

	VLog("Error: Unknown glInternalFormat %x (%s)", image->glInternalFormat, VuoGl_stringForConstant(image->glInternalFormat));
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
 *    - a GL Texture Name, format, and the texture's width and height in pixels:
 * @eg{
 *	{
 *		"glTextureName": 42,
 *		"glInternalFormat": 6407,
 *		"pixelsWide": 640,
 *		"pixelsHigh": 480
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
VuoImage VuoImage_valueFromJson(json_object * js)
{
	if (!js)
		return NULL;

	unsigned int glTextureName;
	unsigned int glInternalFormat = 0;
	unsigned long int pixelsWide;
	unsigned long int pixelsHigh;

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
		if (json_object_object_get_ex(js, "color", &o))
			return VuoImage_makeColorImage(VuoColor_valueFromJson(o), pixelsWide, pixelsHigh);
	}

	{
		json_object * o;
		if (json_object_object_get_ex(js, "glTextureName", &o))
		{
			glTextureName = json_object_get_int64(o);
			if (json_object_object_get_ex(js, "glInternalFormat", &o))
				glInternalFormat = json_object_get_int64(o);
		}
		else if (json_object_object_get_ex(js, "ioSurface", &o))
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
					glEnable(GL_TEXTURE_RECTANGLE_ARB); //?
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					surf = IOSurfaceLookup(surfID);
					if (!surf)
					{
						VLog("Error: IOSurfaceLookup(%d) failed.", surfID);
						return NULL;
					}
					glInternalFormat = GL_RGBA;
					CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, glInternalFormat, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
					if(err != kCGLNoError)
					{
						VLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
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
				VuoRetain(imageRect);

				const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
					// Inputs
					uniform sampler2DRect texture;
					uniform vec2 textureSize;
					varying vec4 fragmentTextureCoordinate;

					void main()
					{
						gl_FragColor = texture2DRect(texture, fragmentTextureCoordinate.xy*textureSize);
					}
				);

				/// @todo candidate for VuoShader_makeGlTextureRectangleShader()?
				VuoShader shader = VuoShader_make("Convert IOSurface GL_TEXTURE_RECTANGLE_ARB to GL_TEXTURE_2D");
				VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
				VuoRetain(shader);
				VuoShader_setUniform_VuoImage  (shader, "texture", imageRect);
				VuoShader_setUniform_VuoPoint2d(shader, "textureSize", VuoPoint2d_make(pixelsWide, pixelsHigh));

				VuoImageRenderer ir = VuoImageRenderer_make(cgl_ctx);
				VuoRetain(ir);
				image2d = VuoImageRenderer_draw(ir, shader, pixelsWide, pixelsHigh, VuoImage_getColorDepth(imageRect));
				VuoRelease(ir);

				VuoRelease(shader);
				VuoRelease(imageRect);
			}

			VuoIoSurfacePool_signal(surf);
			CFRelease(surf);

			VuoGlContext_disuse(glContext);
			return image2d;
		}
		else
			return NULL;
	}

	return VuoImage_make(glTextureName, glInternalFormat, pixelsWide, pixelsHigh);
}

/**
 * @ingroup VuoImage
 * Encodes @c value as a JSON object.
 *
 * @threadAny
 */
json_object * VuoImage_jsonFromValue(const VuoImage value)
{
	json_object * js = json_object_new_object();
	if (!value)
		return js;

	{
		json_object * o = json_object_new_int64(value->glTextureName);
		json_object_object_add(js, "glTextureName", o);
	}
	{
		json_object * o = json_object_new_int64(value->glInternalFormat);
		json_object_object_add(js, "glInternalFormat", o);
	}
	{
		json_object * o = json_object_new_int64(value->pixelsWide);
		json_object_object_add(js, "pixelsWide", o);
	}
	{
		json_object * o = json_object_new_int64(value->pixelsHigh);
		json_object_object_add(js, "pixelsHigh", o);
	}

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
json_object * VuoImage_interprocessJsonFromValue(const VuoImage value)
{
	json_object * js = json_object_new_object();
	if (!value)
		return js;

	VuoShader shader = NULL;
	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
//		VLog("Creating an IOSurface from glTextureName %d on target %lu",value->glTextureName,value->glTextureTarget);

		if (value->glTextureTarget == GL_TEXTURE_2D)
			shader = VuoShader_makeUnlitAlphaPassthruImageShader(value);
		else if (value->glTextureTarget == GL_TEXTURE_RECTANGLE_ARB)
			shader = VuoShader_makeGlTextureRectangleAlphaPassthruShader(value);
		VuoRetain(shader);

		VuoImageRenderer ir = VuoImageRenderer_make(cgl_ctx);
		VuoRetain(ir);
		IOSurfaceID surfID = VuoImageRenderer_draw_internal(ir, shader, value->pixelsWide, value->pixelsHigh, VuoImage_getColorDepth(value), true, true);
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
char * VuoImage_summaryFromValue(const VuoImage value)
{
	if (!value)
		return strdup("(no image)");

	const char *clientOwned = value->freeCallback ? "client-owned " : "";
	return VuoText_format("GL Texture (%sID %u)<br>%lux%lu", clientOwned, value->glTextureName, value->pixelsWide, value->pixelsHigh);
}
