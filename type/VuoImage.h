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
typedef struct _VuoImage *VuoImage;

/**
 * A callback to be implemented if non-Vuo code needs to retain ownership of the GL texture
 * (instead of the default behavior where @ref VuoImage takes ownership of the texture).
 *
 * @see VuoImage_makeClientOwned
 */
typedef void (*VuoImage_freeCallback)(VuoImage imageToFree);

/**
 * An image residing in GPU memory (GL Texture Object).
 */
struct _VuoImage
{
	unsigned int glTextureName;	///< The unique OpenGL texture identifier.
	unsigned int glInternalFormat;	///< The OpenGL internal texture format (as provided to @c glTexImage2D).
	unsigned long int glTextureTarget;	///< Always @c GL_TEXTURE_2D, unless converting from an @c IOSurface.
	unsigned long int pixelsWide; ///< The horizontal size of the image, in pixels.
	unsigned long int pixelsHigh; ///< The vertical size of the image, in pixels.

	VuoImage_freeCallback freeCallback;	///< A callback to be implemented if non-Vuo code needs to retain ownership of the GL texture.  See @ref VuoImage_makeClientOwned.
	void *freeCallbackContext;	///< User data for @c freeCallback.  See @ref VuoImage_makeClientOwned.
};

VuoImage VuoImage_make(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh);
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext);
VuoImage VuoImage_makeFromBuffer(unsigned char *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh);

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
