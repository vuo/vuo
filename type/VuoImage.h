/**
 * @file
 * VuoImage C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoImage_h
#define VuoImage_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISPATCH_RETURNS_RETAINED_BLOCK
	/// Disable DISPATCH_RETURNS_RETAINED_BLOCK, which emits warnings on Mac OS 10.10.
	/// https://b33p.net/kosada/node/9139
	#define DISPATCH_RETURNS_RETAINED_BLOCK
#endif
#include "VuoMacOSSDKWorkaround.h"
#include <dispatch/dispatch.h>

#include "VuoBoolean.h"
#include "VuoColor.h"
#include "VuoGlContext.h"
#include "VuoImageColorDepth.h"
#include "VuoImageWrapMode.h"
#include "VuoPoint2d.h"
#include "VuoRectangle.h"

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

#define VuoImage_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#define VuoImage_OVERRIDES_INTERPROCESS_SERIALIZATION  ///< This type implements `_getInterprocessJson()`.

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
	float scaleFactor;            ///< Number of linear pixels per point in this image.  scaleFactor=1 for @1x images, 2 for @2x (Retina) images.

	void *freeCallbackContext;	///< User data for @c freeCallback.  See @ref VuoImage_makeClientOwned.

	// CPU image
	dispatch_once_t cpuQueueInitialized;	///< Has `cpuQueue` been initialized?
	dispatch_queue_t cpuQueue;				///< Serializes access to `cpuData`.
	struct json_object *cpuData;			///< Map of GL formats to their `char *` pixelbuffers and freeCallbacks.
};

/**
 * Different types of image interpolation.
 */
typedef enum {
	VuoImageSampling_Nearest,
	VuoImageSampling_Linear,
} VuoImageSamplingMode;

VuoImage VuoImage_make(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh);
VuoImage VuoImage_makeClientOwned(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeClientOwnedGlTextureRectangle(unsigned int glTextureName, unsigned int glInternalFormat, unsigned long int pixelsWide, unsigned long int pixelsHigh, VuoImage_freeCallback freeCallback, void *freeCallbackContext) __attribute__((nonnull(5)));
VuoImage VuoImage_makeFromBuffer(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels)) __attribute__((nonnull(1)));
VuoImage VuoImage_makeFromBufferWithStride(const void *pixels, unsigned int format, unsigned int pixelsWide, unsigned int pixelsHigh, unsigned int bytesPerRow, VuoImageColorDepth colorDepth, void (^freeCallback)(void *pixels)) __attribute__((nonnull(1))) __attribute__((nonnull(7)));
VuoImage VuoImage_makeColorImage(VuoColor color, unsigned int pixelsWide, unsigned int pixelsHigh);
VuoImage VuoImage_makeCopy(VuoImage image, bool flip, unsigned int forcePixelsWide, unsigned int forcePixelsHigh, bool forceAlpha);
VuoImage VuoImage_makeGlTextureRectangleCopy(VuoImage image);
const unsigned char *VuoImage_getBuffer(VuoImage image, unsigned int requestedFormat);

VuoImageWrapMode VuoImage_getWrapMode(VuoImage image);
void VuoImage_setWrapMode(VuoImage image, VuoImageWrapMode wrapMode);

void VuoImage_setSamplingMode(VuoImage image, VuoImageSamplingMode samplingMode);

VuoImage VuoImage_mapColors(VuoImage image, VuoList_VuoColor colors, VuoReal filterOpacity);

bool VuoImage_areEqual(const VuoImage a, const VuoImage b);
bool VuoImage_areEqualWithinTolerance(const VuoImage a, const VuoImage b, const unsigned char tolerance);
bool VuoImage_isLessThan(const VuoImage a, const VuoImage b);
bool VuoImage_isEmpty(const VuoImage image);
bool VuoImage_isBlackOrTransparent(const VuoImage image, const unsigned char tolerance);
bool VuoImage_isPopulated(const VuoImage image);

VuoRectangle VuoImage_getRectangle(const VuoImage image);
VuoImageColorDepth VuoImage_getColorDepth(const VuoImage image);

VuoImage VuoImage_makeFromJson(struct json_object * js);
VuoImage VuoImage_makeFromJsonWithDimensions(struct json_object *js, unsigned int requestedPixelsWide, unsigned int requestedPixelsHigh) VuoWarnUnusedResult;
GLuint VuoImage_resolveInterprocessJsonUsingTextureProvider(struct json_object *js, GLuint (^provider)(unsigned int pixelsWide, unsigned int pixelsHigh), unsigned int *outputPixelsWide, unsigned int *outputPixelsHigh, void *outputIOSurface) VuoWarnUnusedResult;
bool VuoImage_resolveInterprocessJsonUsingClientTexture(struct json_object *js, GLuint textureName, unsigned int pixelsWide, unsigned int pixelsHigh, void *outputIOSurface) VuoWarnUnusedResult;
bool VuoImage_resolveInterprocessJsonOntoFramebuffer(struct json_object *js, VuoGlContext context, bool flip, bool stretch) VuoWarnUnusedResult;
struct json_object * VuoImage_getJson(const VuoImage value);

struct json_object * VuoImage_getInterprocessJson(const VuoImage value);

char * VuoImage_getSummary(const VuoImage value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoImage_getString(const VuoImage value);
char * VuoImage_getInterprocessString(const VuoImage value);
void VuoImage_retain(VuoImage value);
void VuoImage_release(VuoImage value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
