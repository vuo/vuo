/**
 * @file
 * VuoImage C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOIMAGE_H
#define VUOIMAGE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoImage VuoImage
 * An image residing in GPU memory (GL Texture Object).
 *
 * @{
 */

/**
 * An image residing in GPU memory (GL Texture Object).
 *
 * The struct is typedef'd to a pointer so that VuoImages are reference-counted,
 * enabling us to automatically delete the GL Texture Object when the last reference is released.
 */
typedef struct _VuoImage
{
	unsigned int glTextureName;
	unsigned long int glTextureTarget;	///< Always GL_TEXTURE_2D, unless converting from an IOSurface.
	unsigned long int pixelsWide; ///< We could @c glGetTexLevelParameteri() but that would require a GPU roundtrip (inefficient).
	unsigned long int pixelsHigh;
} *VuoImage;

VuoImage VuoImage_make(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh);
VuoImage VuoImage_valueFromJson(struct json_object * js);
struct json_object * VuoImage_jsonFromValue(const VuoImage value);
struct json_object * VuoImage_interprocessJsonFromValue(const VuoImage value);
char * VuoImage_summaryFromValue(const VuoImage value);

/// @{
/**
 * Automatically generated function.
 */
VuoImage VuoImage_valueFromString(const char *str);
char * VuoImage_stringFromValue(const VuoImage value);
char * VuoImage_interprocessStringFromValue(const VuoImage value);
/// @}

/**
 * @}
 */

#endif
