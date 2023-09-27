/**
 * @file
 * VuoDsp implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDsp.h"
#include <Accelerate/Accelerate.h>

#include "VuoMacOSSDKWorkaround.h"
#import <Foundation/Foundation.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoDsp",
					  "dependencies" : [
						"VuoAudioSamples",
						"VuoList_VuoReal",
						"Accelerate.framework"
					  ]
				 });
#endif

/**
 * Holds instance data required by vDsp.
 */
@interface VuoDspObject : NSObject
{
	FFTSetup _fftSetup;		///< vDsp FFTSetup for this instance.
	DSPSplitComplex _split;	///< Holds the DSPSplitComplex values per-bucket.
	float* _frequency;		///< Hold the real values from the split complex array.
	unsigned int _frameSize;	///< The number of frames per-bucket to analyze.  Must be a power of 2.
	VuoWindowing _windowMode;	///< What type of windowing to apply to sample data.
	float* _window;			///< Holds windowed real values from samples.
	VuoAudioBinAverageType priorFrequencyMode; ///< To detect when the frequencyMode changes.
}

- (VuoReal *) frequenciesForSampleData:(float *) sampleData numFrames:(int)frames mode:(VuoAudioBinAverageType)frequencyMode outputCount:(unsigned int *)count newSumming:(bool)newSumming;
- (id) initWithSize:(unsigned int)frameSize windowing:(VuoWindowing)windowMode;
@end

@implementation VuoDspObject

/**
 * Initialize a new VuoDspObject that will analyze samples of frameSize buckets.
 * To change the frameSize, you must release and re-initialize this object.
 */
- (id) initWithSize:(unsigned int)frameSize windowing:(VuoWindowing)windowMode
{
	if (self = [super init])
	{

	_fftSetup = vDSP_create_fftsetup( log2f(frameSize), kFFTRadix2 );
	// WARNING:  we're minimizing allocations here, but we _have_ to keep in mind that _split.realp and _split.imagp _MUST_ by
	// 16-byte aligned.  malloc/calloc will automatically do this for us, but pointer math will NOT.
	// float is 4 bytes, so frameSize _MUST_ be a multiple of 4 to ensure that we're always 16-byte aligned later on.
	// This is currently guaranteed, with frameSize being between 256 and 65536.
	_frequency = (float*)calloc(sizeof(float)*frameSize * 2, 1);
	_split.realp = _frequency + frameSize;
	_split.imagp = _split.realp + frameSize / 2;
	_frameSize = frameSize;
	priorFrequencyMode = (VuoAudioBinAverageType)-1;

	// https://developer.apple.com/library/prerelease/ios/documentation/Accelerate/Reference/vDSPRef/index.html#//apple_ref/c/func/vDSP_blkman_window
	// https://stackoverflow.com/questions/12642916/fft-output-with-float-buffer-audiounit

	switch(windowMode)
	{
		case VuoWindowing_Hamming:
			_window = (float *) malloc(sizeof(float) * frameSize);
			vDSP_hamm_window(_window, frameSize, 0);
			break;

		case VuoWindowing_Hann:
			_window = (float *) malloc(sizeof(float) * frameSize);
			vDSP_hann_window(_window, frameSize, 0);
			break;

		case VuoWindowing_Blackman:
			_window = (float *) malloc(sizeof(float) * frameSize);
			vDSP_blkman_window(_window, frameSize, 0);
			break;

		default:
			_window = NULL;
			break;
	}
	}

	return self;
}

static void VuoDsp_showFrequencies(int frameCount, VuoAudioBinAverageType mode, int lowBin, int highBin, int displayBin)
{
	double nyquist = VuoAudioSamples_sampleRate / 2.;
	double width = nyquist / (frameCount/2);

	double lowFrequency    = ( lowBin - .5) * width;
	double highFrequency   = (highBin + .5) * width;
	double centerFrequency = (((double)lowBin + highBin) / 2.) * width;

	VUserLog("Bin %4d   %8.2f Hz ± %7.2f Hz   (%8.2f Hz to %8.2f Hz)", displayBin, centerFrequency, (highFrequency - lowFrequency) / 2., lowFrequency, highFrequency);
}

/**
 * ...
 */
- (VuoReal *)frequenciesForSampleData:(float *)sampleData numFrames:(int)frames mode:(VuoAudioBinAverageType)frequencyMode outputCount:(unsigned int *)count newSumming:(bool)newSumming
{
	bool showTable = false;
	if (frames != _frameSize || frequencyMode != priorFrequencyMode)
	{
		_frameSize = frames;
		priorFrequencyMode = frequencyMode;
		showTable = VuoIsDebugEnabled();
	}

	VuoReal *freqChannel = (VuoReal *)malloc(sizeof(VuoReal) * frames/2);
	// see vDSP_Library.pdf, page 20

	// apply windowing
	if(_window != NULL)
		vDSP_vmul(sampleData, 1, _window, 1, sampleData, 1, frames);

	// turn channel of (real) sampleData into a (real) even-odd array (despite the DSPSplitComplex datatype).
	unsigned int offset = 0;
	unsigned int i;
	DSPSplitComplex lSplit = _split;

	for( i=0; i < frames/2; ++i)
	{
		lSplit.realp[i] = sampleData[offset];
		offset++;
		lSplit.imagp[i] = sampleData[offset];
		offset++;
	}

	// perform real-to-complex FFT.
	vDSP_fft_zrip( _fftSetup, &lSplit, 1, log2f(_frameSize), kFFTDirection_Forward );

	// scale by 1/2*n because vDSP_fft_zrip doesn't use the right scaling factors natively ("for better performances")
	if(_window == NULL || newSumming)
	{
		// const float scale = 1.0f/(2.0f*(float)frames);
		const float scale = 1.0f/frames;

		vDSP_vsmul( lSplit.realp, 1, &scale, lSplit.realp, 1, frames/2 );
		vDSP_vsmul( lSplit.imagp, 1, &scale, lSplit.imagp, 1, frames/2 );
	}

	// vDSP_zvmags(&lSplit, 1, lSplit.realp, 1, frames/2);

	// collapse split complex array into a real array.
	// split[0] contains the DC, and the values we're interested in are split[1] to split[len/2] (since the rest are complex conjugates)
	float *lFrequency = _frequency;
	vDSP_zvabs( &lSplit, 1, lFrequency, 1, frames/2 );

	switch(frequencyMode)
	{
		case VuoAudioBinAverageType_None:	// Linear Raw
		{
			if (newSumming)
				for( i=1; i<frames/2; ++i )
					freqChannel[i-1] = lFrequency[i];
			else
				for( i=1; i<frames/2; ++i )
					freqChannel[i-1] = lFrequency[i] * ((float)sqrtf(i)*2.f + 1.f);
			*count = frames/2 - 1;

			if (showTable)
				for (i = 1; i < frames/2; ++i)
					VuoDsp_showFrequencies(frames, frequencyMode, i, i, i);

			break;
		}
		case VuoAudioBinAverageType_Quadratic:	// Quadratic Average
		{
			int lowerFrequency = 1, upperFrequency;
			int k;
			float sum;
			bool done=NO;
			i=0;
			while(!done)
			{
				upperFrequency = lowerFrequency + i;
				sum=0.f;
				if( upperFrequency >= frames/2 )
				{
					upperFrequency = frames/2-1;
					done=YES;
				}
				for( k=lowerFrequency; k<=upperFrequency; ++k )
					sum += lFrequency[k];
				sum /= (float)(upperFrequency-lowerFrequency+1);
				sum *= (float)i*2.f + 1.f;
				freqChannel[i] = sum;

				if (showTable)
					VuoDsp_showFrequencies(frames, frequencyMode, lowerFrequency, upperFrequency, i + 1);

				lowerFrequency = upperFrequency + 1;
				++i;
			}
			*count = i;
			break;
		}
		case VuoAudioBinAverageType_Logarithmic:	// Logarithmic Average
		{
			const float log2FrameSize = log2f(_frameSize);
			int numBuckets = log2FrameSize;
			int lowerFrequency, upperFrequency;
			int k;
			float sum;
			for( i=0; i<numBuckets; ++i)
			{
				lowerFrequency = (frames/2) / powf(2.f,log2FrameSize-i  )+1;
				upperFrequency = (frames/2) / powf(2.f,log2FrameSize-i-1);
				sum=0.f;
				if(upperFrequency>=frames/2)
					upperFrequency=frames/2-1;
				for( k=lowerFrequency; k<=upperFrequency; ++k )
					sum += lFrequency[k];
				sum /= (float)(upperFrequency-lowerFrequency+1);
				sum *= (float)powf(i,1.5f) + 1.f;
				freqChannel[i] = sum;

				if (showTable)
					VuoDsp_showFrequencies(frames, frequencyMode, lowerFrequency, upperFrequency, i + 1);
			}
			*count = numBuckets;
			break;
		}
	}

	return freqChannel;
}

/**
 * Free the fftsetup and _frequency values.
 */
- (void)dealloc
{
	vDSP_destroy_fftsetup(_fftSetup);
	free(_frequency);

	if(_window)
		free(_window);

	[super dealloc];
}

@end

/**
 * Free things associated with AvWriter.
 */
void VuoDsp_free(VuoDsp dspObject);

VuoDsp VuoDsp_make(unsigned int frameSize, VuoWindowing windowing)
{
	VuoDspObject* fft = [[VuoDspObject alloc] initWithSize:frameSize windowing:windowing];
	VuoRegister(fft, VuoDsp_free);
	return (void*) fft;
}

/**
 * Analyze the provided audio samples using the specified bin averaging method.  Returns the spectrum band as a double * array putting the size into spectrumSize.
 */
VuoReal* VuoDsp_frequenciesForSamples(VuoDsp dspObject, VuoReal* audio, unsigned int sampleCount, VuoAudioBinAverageType binAveraging, unsigned int* spectrumSize, bool newSumming)
{
	VuoDspObject* dsp = (VuoDspObject*)dspObject;

	if(dsp == NULL)
		return NULL;

	float* vals = (float*)malloc(sizeof(float)*sampleCount);
	for(int i = 0; i < sampleCount; i++) vals[i] = (float)audio[i];

	VuoReal *freq = [dsp frequenciesForSampleData:vals numFrames:sampleCount mode:binAveraging outputCount:spectrumSize newSumming:newSumming];
	free(vals);

	return freq;
}

/**
 * Release the FFT object.
 */
void VuoDsp_free(VuoDsp dspObject)
{
	VuoDspObject* fft = (VuoDspObject*)dspObject;

	if(fft)
		[fft release];
}
