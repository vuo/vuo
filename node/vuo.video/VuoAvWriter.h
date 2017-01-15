/**
 * @file
 * Vuo AV Foundation video writer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoMovieFormat.h"

/**
 * An object for writing audio and images to a video file.
 */
typedef void * VuoAvWriter;

/**
 *	Create a new VuoAvWriter.
 */
VuoAvWriter VuoAvWriter_make();

/**
 * Initialize a new movie, and begin recording.
 */
bool VuoAvWriter_initializeMovie(VuoAvWriter writer, int width, int height, int channels, VuoText url, bool overwrite, VuoMovieFormat format);

/**
 * Is this movie initialized already?
 */
bool VuoAvWriter_isInitialized(VuoAvWriter writer);

/**
 * Appends an image to a movie file.  If called prior to VuoAvWriter_initializeMovie, this has no effect.
 */
void VuoAvWriter_appendImage(VuoAvWriter writer, VuoImage image);

/**
 * Appends a set of audio samples to movie file.  If called prior to VuoAvWriter_initializeMovie, this has no effect.
 */
void VuoAvWriter_appendAudio(VuoAvWriter writer, VuoList_VuoAudioSamples samples);

/**
 * Stop recording and finalize rendering the movie.
 */
void VuoAvWriter_finalize(VuoAvWriter writer);

#ifdef __cplusplus
}
#endif
