/**
 * @file
 * VuoImage implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
						 "c",
						 "json",
						 "VuoGlContext",
						 "VuoGlPool",
						 "VuoImageRenderer",
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
		VuoGlTexture_release(t->glTextureName);

	free(t);
}

/**
 * Helper for @c VuoImage_make and @c VuoImage_makeClientOwned.
 */
VuoImage VuoImage_make_internal(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh)
{
	VuoImage t = (VuoImage)malloc(sizeof(struct _VuoImage));
	VuoRegister(t, VuoImage_free);

	t->glTextureName = glTextureName;
	t->glTextureTarget = GL_TEXTURE_2D;
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
 * The texture must be of type @c GL_TEXTURE_2D.
 *
 * The @ref VuoImage takes ownership of @c glTextureName,
 * and will call @c glDeleteTextures() on it when it's no longer needed.
 *
 * @ref VuoImage is intended to be immutable — do not modify
 * the contents of the @c glTextureName after passing it to this function.
 *
 * @threadAny
 */
VuoImage VuoImage_make(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh)
{
	VuoImage t = VuoImage_make_internal(glTextureName, pixelsWide, pixelsHigh);
	VuoGlTexture_retain(glTextureName);
	return t;
}

/**
 * @ingroup VuoImage
 * Returns a new @ref VuoImage structure representing the specified @c glTextureName.
 *
 * The texture must be of type @c GL_TEXTURE_2D.
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
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext)
{
	VuoImage t = VuoImage_make_internal(glTextureName, pixelsWide, pixelsHigh);
	t->freeCallback = freeCallback;
	t->freeCallbackContext = freeCallbackContext;
	return t;
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
 * @param js A JSON object containing a GL Texture Name or @c IOSurfaceID, and the texture's width and height in pixels.
 *
 * @eg{
 *	{
 *		"glTextureName": 42,
 *		"pixelsWide": 640,
 *		"pixelsHigh": 480
 *	}
 * }
 *
 * @eg{
 *	{
 *		"ioSurface": 42,
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
	unsigned long int pixelsWide;
	unsigned long int pixelsHigh;

	{
		json_object * o;
		if (json_object_object_get_ex(js, "pixelsWide", &o))
			pixelsWide = json_object_get_int64(o);
		else
			goto error;
	}
	{
		json_object * o;
		if (json_object_object_get_ex(js, "pixelsHigh", &o))
			pixelsHigh = json_object_get_int64(o);
		else
			goto error;
	}
	{
		json_object * o;
		if (json_object_object_get_ex(js, "glTextureName", &o))
			glTextureName = json_object_get_int64(o);
		else if (json_object_object_get_ex(js, "ioSurface", &o))
		{
			IOSurfaceID surfID = json_object_get_int(o);
//			VLog("Converting IOSurfaceID %d",surfID);

			// Read the IOSurface into a GL_TEXTURE_RECTANGLE_ARB (the only texture type IOSurface supports).
			GLuint textureRect;
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			{
				{
					textureRect = VuoGlPool_use(VuoGlPool_Texture);
					glEnable(GL_TEXTURE_RECTANGLE_ARB); //?
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					IOSurfaceRef surf = IOSurfaceLookup(surfID);
					CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
					// IOSurfaceDecrementUseCount(surf); ?
					CFRelease(surf);
					if(err != kCGLNoError)
					{
						fprintf(stderr,"VuoImageRenderer_draw_internal() Error in CGLTexImageIOSurface2D(): %s\n", CGLErrorString(err));
						return NULL;
					}
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				}
			}
			glFlushRenderAPPLE();

			// Convert the GL_TEXTURE_RECTANGLE_ARB into GL_TEXTURE_2D.
			VuoImage image2d;
			{
				VuoImage imageRect = VuoImage_make(textureRect, pixelsWide, pixelsHigh);
				imageRect->glTextureTarget = GL_TEXTURE_RECTANGLE_ARB;

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

				VuoShader shader = VuoShader_make("convert IOSurface GL_TEXTURE_RECTANGLE_ARB to GL_TEXTURE_2D", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
				VuoShader_addTexture(shader, cgl_ctx, "texture", imageRect);
				VuoShader_setUniformPoint2d(shader, cgl_ctx, "textureSize", VuoPoint2d_make(pixelsWide, pixelsHigh));

				VuoImageRenderer ir = VuoImageRenderer_make();
				VuoRetain(ir);
				image2d = VuoImageRenderer_draw(ir, cgl_ctx, shader, pixelsWide, pixelsHigh);
				VuoRelease(ir);

				VuoImage_free(imageRect); // deletes textureRect
			}

			json_object_put(js);
			VuoGlContext_disuse(cgl_ctx);
			return image2d;
		}
		else
			goto error;
	}

	return VuoImage_make(glTextureName, pixelsWide, pixelsHigh);

error:
	json_object_put(js);
	return NULL;
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
 * @threadAny
 */
json_object * VuoImage_interprocessJsonFromValue(const VuoImage value)
{
	json_object * js = json_object_new_object();
	if (!value)
		return js;

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
//		VLog("Creating an IOSurface from glTextureName %d on target %lu",value->glTextureName,value->glTextureTarget);

		VuoShader shader = VuoShader_makeImageShader();
		VuoRetain(shader);
		VuoShader_addTexture(shader, cgl_ctx, "texture", value);

		VuoImageRenderer ir = VuoImageRenderer_make();
		VuoRetain(ir);
		IOSurfaceID surfID = VuoImageRenderer_draw_internal(ir,cgl_ctx,shader,value->pixelsWide,value->pixelsHigh,true);
		VuoRelease(shader);
		VuoRelease(ir);
//		VLog("Created IOSurfaceID %d",surfID);

		json_object * o = json_object_new_int(surfID);
		json_object_object_add(js, "ioSurface", o);

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

	const char * format = "GL Texture (%sID %u)<br>%lux%lu";

	const char *clientOwned = value->freeCallback ? "client-owned " : "";

	int size = snprintf(NULL, 0, format, clientOwned, value->glTextureName, value->pixelsWide, value->pixelsHigh);
	char * valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString, size+1, format, clientOwned, value->glTextureName, value->pixelsWide, value->pixelsHigh);
	return valueAsString;
}
