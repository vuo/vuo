/**
 * @file
 * VuoImage C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOIMAGE_H
#define VUOIMAGE_H

#ifndef DISPATCH_RETURNS_RETAINED_BLOCK
	/// Disable DISPATCH_RETURNS_RETAINED_BLOCK, which emits warnings on Mac OS 10.10.
	/// https://b33p.net/kosada/node/9139
	#define DISPATCH_RETURNS_RETAINED_BLOCK
#endif
#include <dispatch/dispatch.h>

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
 * An image residing in GPU memory (GL Texture Object),
 * with an optional CPU copy.
 */
struct _VuoImage
{
	// GPU image
	unsigned int glTextureName;	///< The unique OpenGL texture identifier.
	unsigned int glInternalFormat;	///< The OpenGL internal texture format (as provided to @c glTexImage2D).
	unsigned long int glTextureTarget;	///< Always @c GL_TEXTURE_2D, unless converting from an @c IOSurface.

	// Common
	unsigned long int pixelsWide; ///< The horizontal size of the image, in pixels.
	unsigned long int pixelsHigh; ///< The vertical size of the image, in pixels.

	void *freeCallbackContext;	///< User data for @c freeCallback.  See @ref VuoImage_makeClientOwned.

	// CPU image
	dispatch_once_t cpuQueueInitialized;	///< Has `cpuQueue` been initialized?
	dispatch_queue_t cpuQueue;				///< Serializes access to `cpuData`.
	struct json_object *cpuData;			///< Map of GL formats to their `char *` pixelbuffers and freeCallbacks.
};

VuoImage VuoImage_make(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh);
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeClientOwnedGlTextureRectangle(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeFromBuffer(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels)) __attribute__((nonnull(1)));
VuoImage VuoImage_makeFromBufferWithStride(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, unsigned int bytesPerRow, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels)) __attribute__((nonnull(1)));
VuoImage VuoImage_makeColorImage(VuoColor color, unsigned int pixelsWide, unsigned int pixelsHigh);
VuoImage VuoImage_makeCopy(VuoImage image, bool flip);
VuoImage VuoImage_makeGlTextureRectangleCopy(VuoImage image);
VuoImage VuoImage_makeFromContextFramebuffer(VuoGlContext context);
const unsigned char *VuoImage_getBuffer(VuoImage image, unsigned int requestedFormat);
void VuoImage_setWrapMode(VuoImage image, VuoImageWrapMode wrapMode);

VuoImage VuoImage_mapColors(VuoImage image, VuoList_VuoColor colors, VuoReal filterOpacity);

bool VuoImage_areEqual(const VuoImage a, const VuoImage b);
bool VuoImage_areEqualWithinTolerance(const VuoImage a, const VuoImage b, const unsigned char tolerance);
bool VuoImage_isEmpty(const VuoImage image);
bool VuoImage_isPopulated(const VuoImage image);

VuoRectangle VuoImage_getRectangle(const VuoImage image);
VuoImageColorDepth VuoImage_getColorDepth(const VuoImage image);

VuoImage VuoImage_makeFromJson(struct json_object * js);
GLuint VuoImage_resolveInterprocessJsonUsingTextureProvider(struct json_object *js, GLuint (^provider)(unsigned int pixelsWide, unsigned int pixelsHigh), unsigned int *outputPixelsWide, unsigned int *outputPixelsHigh, void *outputIOSurface);
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
