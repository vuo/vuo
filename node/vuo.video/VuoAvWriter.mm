/**
 * @file
 * VuoAvWriter implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"
#include "VuoApp.h"
#include "VuoAvWriter.h"
#include "VuoAvWriterObject.h"

#include <VideoToolbox/VideoToolbox.h>
#include <json-c/json.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoAvWriter",
					  "dependencies" : [
						"VuoImage",
						"VuoAudioSamples",
						"VuoAvWriterObject",
						"VuoUrl",
						"CoreMedia.framework",
						"VideoToolbox.framework",
						"AVFoundation.framework"
					  ]
				 });
#endif

/**
 * Free things associated with AvWriter.
 */
void VuoAvWriter_free(VuoAvWriter writer);

static dispatch_once_t VuoAvWriter_initOnce = 0;  ///< Synchronizes initialization.

VuoAvWriter VuoAvWriter_make()
{
	VuoAvWriterObject* av = [[VuoAvWriterObject alloc] init];
	VuoRegister(av, VuoAvWriter_free);

	dispatch_once(&VuoAvWriter_initOnce, ^{
		VuoApp_executeOnMainThread(^{
			VTRegisterProfessionalVideoWorkflowVideoEncoders();
		});
	});

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
		VuoUrl urlNormalized = VuoUrl_normalize(url, VuoUrlNormalize_forSaving);
		VuoLocal(urlNormalized);

		json_object *validExtensions = json_object_new_array();
		json_object_array_add(validExtensions, json_object_new_string("mov"));
		VuoUrl urlWithExtension = VuoUrl_appendFileExtension(urlNormalized, validExtensions);
		VuoLocal(urlWithExtension);

		NSURL *file_url = [NSURL URLWithString:[NSString stringWithUTF8String:urlWithExtension]];

		// check if a file already exists at this location
		if ([[NSFileManager defaultManager] fileExistsAtPath:[file_url path]])
		{
			if(overwrite)
			{
				NSError *error = nil;
				if (![[NSFileManager defaultManager] removeItemAtURL:file_url error:&error])
				{
					VUserLog("Error deleting old video file \"%s\": %s", urlWithExtension, error.localizedDescription.UTF8String);
					return NO;
				}
			}
			else
			{
				// file exits and overwrite is false
				return NO;
			}
		}

		bool success = NO;
		@try
		{
			success = [av setupAssetWriterWithUrl:file_url
									   imageWidth:width
									  imageHeight:height
									 channelCount:channels
									  movieFormat:format];
		}
		@catch (NSException *e)
		{
			VUserLog("Error initializing AVAssetWriter: %s", e.reason.UTF8String);
		}

		if (!success)
			return false;
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
void VuoAvWriter_appendImage(VuoAvWriter writer, VuoImage image, VuoReal timestamp, bool blockIfNotReady)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;

	if(image == nil || av == nil || ![av isRecording])
		return;

	[av appendImage:image presentationTime:timestamp blockIfNotReady:blockIfNotReady];
}

void VuoAvWriter_appendAudio(VuoAvWriter writer, VuoList_VuoAudioSamples samples, VuoReal timestamp, bool blockIfNotReady)
{
	VuoAvWriterObject* av = (VuoAvWriterObject*)writer;

	if(samples == nil || av == nil || ![av isRecording])
		return;

	[av appendAudio:samples presentationTime:timestamp blockIfNotReady:blockIfNotReady];
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
