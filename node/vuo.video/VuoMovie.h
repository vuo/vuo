/**
 * @file
 * VuoMovie interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#include "VuoImage.h"
#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"
#endif

/**
 * An object for controlling and extracting information from video.
 */
typedef void * VuoMovie;

VuoMovie VuoMovie_make(const char *path);

bool VuoMovie_getNextVideoFrame(VuoMovie movie, VuoImage *image, double *nextFrame);
bool VuoMovie_getPreviousVideoFrame(VuoMovie movie, VuoImage *image, double *nextFrame);
bool VuoMovie_seekToSecond(VuoMovie movie, double second);
double VuoMovie_getCurrentSecond(VuoMovie movie);
double VuoMovie_getDuration(VuoMovie movie);
bool VuoMovie_getInfo(const char *path, double *duration);
bool VuoMovie_containsAudio(VuoMovie movie);
bool VuoMovie_getNextAudioSample(VuoMovie movie, VuoList_VuoAudioSamples audioSamples, double *frameTimestampInSeconds);

#ifdef __cplusplus
}
#endif
