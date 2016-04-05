/**
 * @file
 * VuoImage C type definition.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOIMAGE_H
#define VUOIMAGE_H

#include "VuoBoolean.h"
#include "VuoColor.h"
#include "VuoGlContext.h"
#include "VuoImageColorDepth.h"
#include "VuoImageWrapMode.h"
#include "VuoPoint2d.h"

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

	void *freeCallbackContext;	///< User data for @c freeCallback.  See @ref VuoImage_makeClientOwned.
};

VuoImage VuoImage_make(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh);
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeClientOwnedGlTextureRectangle(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeFromBuffer(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth colorDepth) __attribute__((nonnull(1)));
VuoImage VuoImage_makeColorImage(VuoColor color, unsigned int pixelsWide, unsigned int pixelsHigh);
VuoImage VuoImage_makeCopy(VuoImage image, bool flip);
VuoImage VuoImage_makeShallowCopy(VuoImage image);
VuoImage VuoImage_makeGlTextureRectangleCopy(VuoImage image);
VuoImage VuoImage_makeFromContextFramebuffer(VuoGlContext context);
unsigned char *VuoImage_copyBuffer(VuoImage image, unsigned int requestedFormat);
void VuoImage_setWrapMode(VuoImage image, VuoImageWrapMode wrapMode);

VuoImage VuoImage_blur(VuoImage image, VuoReal radius, VuoBoolean expandBounds);
VuoImage VuoImage_mapColors(VuoImage image, VuoList_VuoColor colors, VuoReal filterOpacity);

bool VuoImage_areEqual(const VuoImage a, const VuoImage b);
bool VuoImage_isEmpty(const VuoImage image);

VuoRectangle VuoImage_getRectangle(const VuoImage image);
VuoImageColorDepth VuoImage_getColorDepth(const VuoImage image);

VuoImage VuoImage_makeFromJson(struct json_object * js);
struct json_object * VuoImage_getJson(const VuoImage value);
struct json_object * VuoImage_getInterprocessJson(const VuoImage value);
char * VuoImage_getSummary(const VuoImage value);

/// @{
/**
 * Automatically generated function.
 */
VuoImage VuoImage_makeFromString(const char *str);
char * VuoImage_getString(const VuoImage value);
char * VuoImage_getInterprocessString(const VuoImage value);
void VuoImage_retain(VuoImage value);
void VuoImage_release(VuoImage value);
/// @}

/**
 * @}
 */

#endif
