/**
 * @file
 * VuoAvWriter implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

// #include "module.h"
// #include "/usr/include/time.h"
#include "VuoAvWriter.h"
#include "VuoAvWriterObject.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoAvWriter",
					  "dependencies" : [
						"VuoImage",
						"VuoAudioSamples",
						"VuoAvWriterObject",
						"CoreMedia.framework",
						"AVFoundation.framework"
					  ]
				 });
#endif

/**
 * Free things associated with AvWriter.
 */
void VuoAvWriter_free(VuoAvWriter writer);

VuoAvWriter VuoAvWriter_make()
{
	VuoAvWriterObject* av = [[VuoAvWriterObject alloc] init];
	VuoRegister(av, VuoAvWriter_free);
	return (void*) av;
}

/**
 * Initialize the AssetWriter for video with dimensions, and audio with num channels.  Must be called before appendX.
 */
bool VuoAvWriter_initializeMovie(VuoAvWriter writer, int width, int height, int channels, VuoText url, bool overwrite, VuoMovieFormat format)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;

	if( [av isRecording] == NO )
	{
		NSError *error = nil;

		NSString* apple_string = [[NSString stringWithUTF8String:url] stringByExpandingTildeInPath];
		NSString* extension = [apple_string pathExtension];

		if( [extension caseInsensitiveCompare:@"mov"] != NSOrderedSame )
			apple_string = [apple_string stringByAppendingPathExtension:@"mov"];

		NSURL* file_url = [NSURL fileURLWithPath:apple_string];

		// check if a file already exists at this location
		if ([[NSFileManager defaultManager] fileExistsAtPath:[file_url path]])
		{
			if(overwrite)
			{
				if (![[NSFileManager defaultManager] removeItemAtURL:file_url error:&error])
				{
					VUserLog("Failed deleting old video file at path: %s", [apple_string UTF8String]);
					return NO;
				}
			}
			else
			{
				// file exits and overwrite is false
				return NO;
			}
		}

		// VLog("initialize with: %i, %i  %i", width, height, channels);

		// try to initialize the AssetWriter
		bool success = [av setupAssetWriterWithUrl:file_url
						imageWidth:width
						imageHeight:height
						channelCount:channels
						movieFormat:format];

		// if it doesn't succeed for whatever reason, exit
		if( !success )
		{
			return false;
		}
	}

	// we're good to start recording. now set the correct width, height, and audio channel count
	// note that once these values are set they may not be changed until Finalize is called.
	return true;
}

bool VuoAvWriter_isInitialized(VuoAvWriter writer)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;
	return av != nil && [av isRecording];
}

/**
 * Append an image to the movie file.  If this is the first frame, the writer object will be initialized for you.
 * Can return false in the event that a movie file already exists at url, or initialization failed for any reason.
 * After the initial call, this method always returns true.
 */
void VuoAvWriter_appendImage(VuoAvWriter writer, VuoImage image)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;

	if(image == nil || av == nil || ![av isRecording])
		return;

	[av appendImage:image];
}

void VuoAvWriter_appendAudio(VuoAvWriter writer, VuoList_VuoAudioSamples samples)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;

	if(samples == nil || av == nil || ![av isRecording])
		return;

	[av appendAudio:samples];
}

void VuoAvWriter_finalize(VuoAvWriter writer)
{
	VuoAvWriterObject* writerObject = (VuoAvWriterObject*)writer;

	if(writerObject)
	{
		if([writerObject isRecording])
		{
			[writerObject finalizeRecording];
		}
	}
}

void VuoAvWriter_free(VuoAvWriter writer)
{
	VuoAvWriterObject* writerObject = (VuoAvWriterObject*)writer;

	[writerObject release];
}
