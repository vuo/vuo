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
#include "VuoGlTexturePool.h"

#include <CoreFoundation/CoreFoundation.h>

#include <IOSurface/IOSurfaceAPI.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
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
						 "VuoGlTexturePool",
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
 */
void VuoImage_free(void *texture)
{
	VuoImage t = (VuoImage)texture;
//	VLog("Freeing image %p %s",t,VuoImage_stringFromValue(t));
	VuoGlTexturePool_release(t->glTextureName);
	free(t);
}

/**
 * @ingroup VuoImage
 * Returns a new @c VuoImage structure containing the specified values.
 *
 * The VuoImage takes ownership of @c glTextureName,
 * and will call @c glDeleteTextures() on it when it's no longer needed.
 */
VuoImage VuoImage_make(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh)
{
	VuoImage t = (VuoImage)malloc(sizeof(struct _VuoImage));
	VuoRegister(t, VuoImage_free);

	VuoGlTexturePool_retain(glTextureName);
	t->glTextureName = glTextureName;
	t->glTextureTarget = GL_TEXTURE_2D;
	t->pixelsWide = pixelsWide;
	t->pixelsHigh = pixelsHigh;

//	VLog("Made image %p %s",t,VuoImage_stringFromValue(t));
	return t;
}

/**
 * @ingroup VuoImage
 * Decodes the JSON object @c js to create a new value.
 *
 * Automatically activates and deactivates a GL Context (if needed to dereference an IOSurface).
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
			CGLContextObj ctx = (CGLContextObj)VuoGlContext_use();
			VGL();
			{
				{
					glGenTextures(1, &textureRect);
					glEnable(GL_TEXTURE_RECTANGLE_ARB); //?
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textureRect);

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					IOSurfaceRef surf = IOSurfaceLookup(surfID);
					CGLError err = CGLTexImageIOSurface2D(ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surf, 0);
					// IOSurfaceDecrementUseCount(surf); ?
					CFRelease(surf);
					if(err != kCGLNoError)
					{
						fprintf(stderr,"VuoImageRenderer_draw_internal() Error in CGLTexImageIOSurface2D(): %s\n", CGLErrorString(err));
						return NULL;
					}
				}
			}
			glFlushRenderAPPLE();
			VuoGlContext_disuse();

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
				VuoShader_addTexture(shader, imageRect, "texture");
				VuoShader_setUniformPoint2d(shader, "textureSize", VuoPoint2d_make(pixelsWide, pixelsHigh));

				VuoImageRenderer ir = VuoImageRenderer_make();
				VuoRetain(ir);
				image2d = VuoImageRenderer_draw(ir, shader, pixelsWide, pixelsHigh);
				VuoRelease(ir);

				VuoImage_free(imageRect); // deletes textureRect
			}

			json_object_put(js);
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
 * Automatically activates and deactivates a GL Context.
 */
json_object * VuoImage_interprocessJsonFromValue(const VuoImage value)
{
	json_object * js = json_object_new_object();
	if (!value)
		return js;

	{
//		VLog("Creating an IOSurface from glTextureName %d on target %lu",value->glTextureName,value->glTextureTarget);

		VuoShader shader = VuoShader_makeImageShader();
		VuoRetain(shader);
		VuoShader_addTexture(shader, value, "texture");

		VuoImageRenderer ir = VuoImageRenderer_make();
		VuoRetain(ir);
		IOSurfaceID surfID = VuoImageRenderer_draw_internal(ir,shader,value->pixelsWide,value->pixelsHigh,true);
		VuoRelease(shader);
		VuoRelease(ir);
//		VLog("Created IOSurfaceID %d",surfID);

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

	return js;
}

/**
 * @ingroup VuoImage
 * A brief summary of the contents of this texture.
 *
 * @eg{GL Texture (ID 42)
 * 640x480}
 */
char * VuoImage_summaryFromValue(const VuoImage value)
{
	if (!value)
		return strdup("(no image)");

	const char * format = "GL Texture (ID %lu)<br>%lux%lu";

	int size = snprintf(NULL, 0, format, value->glTextureName, value->pixelsWide, value->pixelsHigh);
	char * valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString, size+1, format, value->glTextureName, value->pixelsWide, value->pixelsHigh);
	return valueAsString;
}
