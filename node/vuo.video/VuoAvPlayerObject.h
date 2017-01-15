/**
 * @file
 * VuoAvPlayerObject implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

/// https://b33p.net/kosada/node/9140
#define NS_RETURNS_INNER_POINTER
#import <Cocoa/Cocoa.h>
#undef NS_RETURNS_INNER_POINTER

#include "VuoVideoFrame.h"
#include "VuoAudioFrame.h"
#include "VuoImage.h"
#include "VuoLoopType.h"
#import <AVFoundation/AVFoundation.h>

/**
* Internal video player implementation.  Use VuoVideoPlayer to work with video.
*/
@interface VuoAvPlayerObject : NSObject
{
	/// The AVAsset currnetly loaded.
	AVAsset* asset;
	/// AVPlayer object.
	// AVPlayer* player;
	/// The current frame timestamp where start of video = 0.
	float videoTimestamp;
	/// asset reader
	AVAssetReader* assetReader;
	/// video reader output
	AVAssetReaderTrackOutput* assetReaderVideoTrackOutput;
	/// audio reader output
	AVAssetReaderTrackOutput* assetReaderAudioTrackOutput;

	/// is the asset ready to begin playback
	volatile bool isReady;
	/// shoudl play once tracks are ready?
	bool playOnReady;
	/// the speed of playback (used to decide which direction to decode video in, and whether audio should be read or not)
	float playbackRate;
	/// the playback speed that this asset wants to be played at by default
	float nominalFrameRate;
	/// A pointer to the C++ VuoAvDecoder object that's driving this thing.  Used to fire callbacks into C++ land.
	void* avDecoderCppObject;
	/// A function pointer to a static C++ function that accepts a pointer to a VuoAvDecoder object to send an `OnDecoderReady` event.
	void(*readyToPlayCallback)(void* decoderCppObject, bool canPlayMedia);

	/// list of video frames to be output
	NSMutableArray* videoQueue;

	/// Holds audio extracted from copyNextSampleBuffer until an audio frame comes to collect it.  Is interleaved.
	Float32* audioBuffer;
	/// the last timestamp as reported by the audio asset reader.
	float audioTimestamp;
	/// the capacity in byte size that the audioBuffer possesses
	size_t audioBufferCapacity;
	/// how many channels copyNextSampleBuffer gives the audio buffer.
	unsigned int audioBufferChannelCount;
	/// the number of samples one channel of audio contains in the audioBuffer
	unsigned int audioBufferSamplesPerChannel;
	/// The current index that audio frames are copying samples from the audioBuffer.
	unsigned int audioBufferSampleIndex;
	/// How many channels of audio are in the current asset track.
	unsigned int audioChannelCount;
	/// True if there is an audio stream that cannot be played by AvFoundation.  False otherwise.
	bool containsUnplayableAudio;

	/// GL_TEXTURE_RECTANGLEs from the movie
	CVOpenGLTextureCacheRef textureCache;
	/// Context for `textureCache`
	VuoGlContext glContext;
}

/// Initialize new instance.
- (id) init;
/// tell this player where the decoder is so that it can invoke callbacks
- (void) setPlayerCallback:(void (*)(void* functionPtr, bool canPlayMedia))callback target:(void*)decoderCppObject;
/// Set the file path and begin loading assets.
- (bool) setURL:(NSURL*)url;
/// unload the currently playing video
// - (void) unload;
/// Set the rate of playback.
- (void) setPlaybackRate:(double)rate;
/// decode a number of video samples prior to the current timestamp
- (bool) decodePreceedingVideoSamples;
/// seek playhead to point and time range to decode within.  If range is < 0, `duration - second` is set as range.
- (bool) seekToSecond:(float)second withRange:(float)range;
/// Is the video ready to begin playback?
- (bool) canBeginPlayback;
/// Get the last decoded video frame.
- (bool) nextVideoFrame:(VuoVideoFrame*)frame;
/// Get the last decoded audio frame.
- (bool) nextAudioFrame:(VuoAudioFrame*) frame;
/// Get the last decoded video timestamp.
- (VuoReal) getCurrentTimestamp;
/// reset the assetreader to a new range.
- (bool) setAssetReaderTimeRange:(CMTimeRange)timeRange;
/// read the next video samplebuffer to store.
- (bool) copyNextVideoSampleBuffer;
/// Get the number of audio channels.
- (unsigned int) getAudioChannelCount:(AVAssetTrack*) track;
/// is audio playback enabled?
- (bool) audioEnabled;
/// get the total duration of this asset
- (VuoReal) getDuration;
/// the ideal framerate
- (double) getFrameRate;
/// true if no audio stream is found, or one is fonud and the decoder is capable of playing it
- (bool) canPlayAudio;
/// return the number of available audio channels
- (unsigned int) audioChannels;
/// Release an AVAssetReader after a 200 ms delay.
+ (void) releaseAssetReader:(AVAssetReader*)reader;
@end
