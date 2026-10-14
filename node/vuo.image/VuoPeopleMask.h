/**
 * @file
 * VuoPeopleMask interface.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#import <IOSurface/IOSurface.h>

/**
 * A state object for generating human image masks.
 */
typedef void *VuoPeopleMask;

VuoPeopleMask VuoPeopleMask_make(void);
VuoImage VuoPeopleMask_processImage(VuoPeopleMask p, IOSurfaceRef sourceIOSurface, VuoImage sourceImage, VuoReal quality);

#ifdef __cplusplus
}
#endif
