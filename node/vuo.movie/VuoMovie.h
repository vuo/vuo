/**
 * @file
 * VuoMovie interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#include "VuoImage.h"
#endif

#include "node.h"

/**
 * An object for controlling and extracting information from video.
 */
typedef void * VuoMovie;

VuoMovie * VuoMovie_make(const char *path);

bool VuoMovie_getNextFrame(VuoMovie *decoder, VuoImage *image, double *nextFrame);
bool VuoMovie_seekToSecond(VuoMovie *movie, double second);
bool VuoMovie_getInfo(const char *path, double *duration);

#ifdef __cplusplus
}
#endif
