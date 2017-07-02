/**
 * @file
 * vuo.video.save node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAvWriter.h"
#include "VuoMovieFormat.h"
#include "VuoImageResize.h"
#include "../vuo.image/VuoSizingMode.h"
#include <pthread.h>

VuoModuleMetadata({
					"title" : "Save to Movie",
					"keywords" : [
						"record", "screen capture", "screencast", "video", "fraps", "append", "write", "export"
					],
					"version" : "1.0.1",
					"dependencies" : [
						"VuoAvWriter",
						"VuoImageResize"
					],
					"node": {
					"isInterface" : true,
					"exampleCompositions" : [
						"RecordMovie.vuo"
					]
					}
				 });

/**
 * AVFoundation needs to initialize audio & video at the same time.
 * This is the buffer time between receiving a video frame and initializing
 * video without audio.
 */
#define APPEND_VIDEO_AUDIO_IMAGE_INITIALIZE_DELTA .5

typedef enum {
	VuoAvWriterState_None,
	VuoAvWriterState_Initializing,
	VuoAvWriterState_Ready,
	VuoAvWriterState_Failed
} AvWriterState;

struct nodeInstanceData
{
	VuoAvWriter avWriter;
	VuoText lastUrl;

	int imageWidth, imageHeight;
	int channelCount;
	double firstEvent;
	bool stopping;

	dispatch_queue_t avWriterQueue;
	dispatch_group_t avWriterQueueGroup;

	AvWriterState writerState;
	pthread_mutex_t writerStateMutex;

	// Used in the event that the image passed is not the right size
	// for the current movie.  Not initialized by default since most
	// times this won't be necessary.
	bool resizeShaderInitialized;
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

#define setWriterState(instance, state) pthread_mutex_lock(&((instance)->writerStateMutex)); \
(instance)->writerState = (state); \
pthread_mutex_unlock(&((instance)->writerStateMutex));

static void initResizeShader( struct nodeInstanceData* instance )
{
	instance->resizeShaderInitialized = true;

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoImageResize_makeShader();
	VuoRetain(instance->shader);
}

static void freeResizeShader( struct nodeInstanceData* instance )
{
	instance->resizeShaderInitialized = false;
	VuoRelease( instance->shader );
	VuoRelease( instance->imageRenderer );
	VuoGlContext_disuse( instance->glContext );
}

struct nodeInstanceData* nodeInstanceInit()
{
	struct nodeInstanceData* instance = (struct nodeInstanceData*) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->avWriter = VuoAvWriter_make();
	VuoRetain(instance->avWriter);

	instance->lastUrl = VuoText_make("");
	VuoRetain(instance->lastUrl);

	instance->writerState = VuoAvWriterState_None;
	pthread_mutex_init(&(instance->writerStateMutex), NULL);

	instance->avWriterQueue = dispatch_queue_create("org.vuo.video_writer_queue", DISPATCH_QUEUE_SERIAL);
	instance->avWriterQueueGroup = dispatch_group_create();

	instance->firstEvent = -1;
	instance->imageWidth = -1;
	instance->imageHeight = -1;
	instance->channelCount = -1;

	instance->resizeShaderInitialized = false;

	return instance;
}

static void reset(struct nodeInstanceData* instance)
{
	// reset
	instance->firstEvent = -1;
	instance->imageWidth = -1;
	instance->imageHeight = -1;
	instance->channelCount = -1;
	setWriterState(instance, VuoAvWriterState_None);
}

void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData*) instance
	)
{
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData*) instance,
	VuoInputData(VuoText, {"default":"~/Desktop/MyMovie.mov", "name":"URL"}) url,
	VuoInputData(VuoImage) saveImage,
	VuoInputEvent({"eventBlocking":"none", "data":"saveImage"}) saveImageEvent,
	VuoInputData(VuoList_VuoAudioSamples) saveAudio,
	VuoInputEvent({"eventBlocking":"none", "data":"saveAudio"}) saveAudioEvent,
	VuoInputEvent({"eventBlocking":"none"}) finalize,
	VuoInputData(VuoBoolean, {"default":false, "name":"Overwrite URL"}) overwriteUrl,
	VuoInputData(VuoMovieFormat, {"default":{"imageEncoding":"H264", "imageQuality":1, "audioEncoding":"LinearPCM", "audioQuality":1}}) format
	)
{
	// grab timestamp at earliest possible instance
	VuoReal received = VuoLogGetTime();

	pthread_mutex_lock(&((*instance)->writerStateMutex));
	AvWriterState state = (*instance)->writerState;
	pthread_mutex_unlock(&((*instance)->writerStateMutex));

	// if the previous attempt to initialize failed empty the caches
	if(state == VuoAvWriterState_Failed)
	{
		reset(*instance);
		state = VuoAvWriterState_None;
	}

	if (saveImageEvent && saveImage)
	{
		if ((*instance)->imageWidth == -1)
		{
			(*instance)->imageWidth = saveImage->pixelsWide;
			(*instance)->imageHeight = saveImage->pixelsHigh;
		}

		if (state == VuoAvWriterState_None)
		{
			// set to initializing so that init function isn't called repeatedly before AvWriter_Init returns,
			// and also prevents node from tryning to write to uninitialized writer
			setWriterState(*instance, VuoAvWriterState_Initializing);

			if( (*instance)->firstEvent < 0 )
				(*instance)->firstEvent = received;

			dispatch_group_async((*instance)->avWriterQueueGroup, (*instance)->avWriterQueue, ^{
				double waitForAudioStart = VuoLogGetTime();
				while ((*instance)->channelCount == -1 && VuoLogGetTime() - waitForAudioStart < APPEND_VIDEO_AUDIO_IMAGE_INITIALIZE_DELTA)
					usleep(USEC_PER_SEC/100);

				VuoAvWriter_initializeMovie((*instance)->avWriter,
											(*instance)->imageWidth,
											(*instance)->imageHeight,
											(*instance)->channelCount,
											url,
											overwriteUrl,
											format);

				bool initSuccess = VuoAvWriter_isInitialized((*instance)->avWriter);
				setWriterState(*instance, initSuccess ? VuoAvWriterState_Ready : VuoAvWriterState_Failed);
			});
		}

		VuoRetain(saveImage);
		dispatch_group_async((*instance)->avWriterQueueGroup, (*instance)->avWriterQueue, ^{
			if (saveImage->pixelsWide != (*instance)->imageWidth || saveImage->pixelsHigh != (*instance)->imageHeight)
			{
				if (!(*instance)->resizeShaderInitialized)
					initResizeShader( (*instance) );

				VuoImage resized = VuoImageResize_resize(saveImage,
														 (*instance)->shader,
														 (*instance)->imageRenderer,
														 VuoSizingMode_Fit,
														 (*instance)->imageWidth,
														 (*instance)->imageHeight);
				VuoRetain( resized );
				VuoAvWriter_appendImage((*instance)->avWriter, resized, received - (*instance)->firstEvent);
				VuoRelease( resized );
			}
			else
			{
				// safe to call appendImage all day long - it will only write if the file has been initialized and is currently
				// recording.
				VuoAvWriter_appendImage((*instance)->avWriter, saveImage, received - (*instance)->firstEvent);
			}
			VuoRelease(saveImage);
		});
	}

	if (saveAudioEvent && saveAudio)
	{
		if ((*instance)->channelCount == -1)
		{
			// setting channel count lets the image processing part know that it's okay to initialize movie.
			(*instance)->channelCount = VuoListGetCount_VuoAudioSamples(saveAudio);
		}

		if (state == VuoAvWriterState_Initializing || state == VuoAvWriterState_Ready)
		{
			VuoRetain(saveAudio);
			dispatch_group_async((*instance)->avWriterQueueGroup, (*instance)->avWriterQueue, ^{
				VuoAvWriter_appendAudio((*instance)->avWriter, saveAudio, received - (*instance)->firstEvent);
				VuoRelease(saveAudio);
			});
		}
	}

	if(finalize)
	{
		(*instance)->stopping = true;
		dispatch_group_wait((*instance)->avWriterQueueGroup, DISPATCH_TIME_FOREVER);
		VuoAvWriter_finalize((*instance)->avWriter);
		reset(*instance);
	}
}

void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData*) instance
	)
{
	(*instance)->stopping = true;

	// block til whatever is currently writing finishes
	dispatch_group_wait((*instance)->avWriterQueueGroup, DISPATCH_TIME_FOREVER);

	if( (*instance)->resizeShaderInitialized )
		freeResizeShader( *instance );
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData*) instance
)
{
	(*instance)->stopping = true;

	if( (*instance)->resizeShaderInitialized )
		freeResizeShader( *instance );

	int ret =  pthread_mutex_destroy(&((*instance)->writerStateMutex));
	if(ret != 0) VUserLog("Failed destroying video writer mutex: (%i)", ret);

	// wait for any existing frames to be written
	dispatch_group_wait((*instance)->avWriterQueueGroup, DISPATCH_TIME_FOREVER);
	dispatch_release((*instance)->avWriterQueue);

	// release calls finalize
	VuoRelease( (*instance)->avWriter );
	VuoRelease( (*instance)->lastUrl );
}
