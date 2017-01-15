/**
 * @file
 * Vuo VDsp implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoAudioSamples.h"
#include "VuoAudioBinAverageType.h"
#include "VuoList_VuoReal.h"

/**
 * An object for writing audio and images to a video file.
 */
typedef void * VuoDsp;

/**
 * An enum defining different windowing modes.
 */
typedef enum {					// actual samples per-bin
	VuoWindowing_None,
	VuoWindowing_Hamming,
	VuoWindowing_Hann,
	VuoWindowing_Blackman,
} VuoWindowing;

/**
 *	Create a new VuoDsp object with frame size (the max number of audio samples to process per-bin).
 */
VuoDsp VuoDsp_make(unsigned int frameSize, VuoWindowing windowing);

/**
 * Return a VuoReal* array with size of spectrumSize of the analyzed audio samples.
 */
VuoReal* VuoDsp_frequenciesForSamples(VuoDsp dspObject, VuoReal* audio, unsigned int sampleCount, VuoAudioBinAverageType binAveraging, unsigned int* spectrumSize);

#ifdef __cplusplus
}
#endif
