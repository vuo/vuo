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

VuoModuleMetadata({
					"title" : "Save to Movie",
					"keywords" : [
						"record", "screen capture", "screencast", "video", "fraps", "append", "write", "export"
					],
					"version" : "1.0.0",
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

#define APPEND_VIDEO_AUDIO_IMAGE_INITIALIZE_DELTA .5f	///< Allow .5 seconds from reeceiving the first image to also receive audio.  If no audio detected within this range, begin recording video.

struct nodeInstanceData
{
	VuoAvWriter avWriter;
	VuoText lastUrl;

	int imageWidth, imageHeight;
	int channelCount;
	double firstEvent;

	// Used in the event that the image passed is not the right size
	// for the current movie.  Not initialized by default since most
	// times this won't be necessary.
	bool resizeShaderInitialized;
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

void initResizeShader( struct nodeInstanceData* instance )
{
	instance->resizeShaderInitialized = true;

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoImageResize_makeShader();
	VuoRetain(instance->shader);
}

void freeResizeShader( struct nodeInstanceData* instance )
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

	instance->firstEvent = -1;
	instance->imageWidth = -1;
	instance->imageHeight = -1;
	instance->channelCount = -1;

	instance->resizeShaderInitialized = false;

	return instance;
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
	if (saveImageEvent && saveImage)
	{
		if (!VuoAvWriter_isInitialized((*instance)->avWriter) && saveImage != NULL)
		{
			double curTime = VuoLogGetTime();

			if( (*instance)->firstEvent < 0 )
				(*instance)->firstEvent = curTime;

			// always use the latest image
			(*instance)->imageWidth = saveImage->pixelsWide;
			(*instance)->imageHeight = saveImage->pixelsHigh;

			if((*instance)->channelCount > 0 || ( ((*instance)->firstEvent > 0) && ((curTime - (*instance)->firstEvent) > APPEND_VIDEO_AUDIO_IMAGE_INITIALIZE_DELTA)) )
			{
				VuoAvWriter_initializeMovie((*instance)->avWriter,
											saveImage->pixelsWide,
											saveImage->pixelsHigh,
											(*instance)->channelCount,
											url,
											overwriteUrl,
											format);
			}
		}
		else
		{
			// if initialized - before appending the image, make sure it's the same dimensions as the
			// currently recording video.  if not, resize it.
			if( saveImage->pixelsWide != (*instance)->imageWidth || saveImage->pixelsHigh != (*instance)->imageHeight )
			{
				if(!(*instance)->resizeShaderInitialized)
				{
					initResizeShader( (*instance) );
				}

				VuoImage resized = VuoImageResize_resize(	saveImage,
															(*instance)->shader,
															(*instance)->imageRenderer,
															VuoSizingMode_Fit,
															(*instance)->imageWidth,
															(*instance)->imageHeight);
				VuoRetain( resized );

				VuoAvWriter_appendImage((*instance)->avWriter, resized);

				VuoRelease( resized );
			}
			else
			{
				// safe to call appendImage all day long - it will only write if the file has been initialized and is currently
				// recording.
				VuoAvWriter_appendImage((*instance)->avWriter, saveImage);
			}
		}
	}

	if (saveAudioEvent && saveAudio)
	{
		if (!VuoAvWriter_isInitialized((*instance)->avWriter) && saveAudio != NULL)
		{
			(*instance)->channelCount = VuoListGetCount_VuoAudioSamples(saveAudio);

			// don't initialize a solely audio movie file.
			if((*instance)->imageWidth > 0)
			{
				VuoAvWriter_initializeMovie((*instance)->avWriter,
							(*instance)->imageWidth,
							(*instance)->imageHeight,
							(*instance)->channelCount,
							url,
							overwriteUrl,
							format);
			}
		}

		VuoAvWriter_appendAudio((*instance)->avWriter, saveAudio);
	}

	if(finalize)
	{
		VuoAvWriter_finalize((*instance)->avWriter);

		// reset
		(*instance)->firstEvent = -1;
		(*instance)->imageWidth = -1;
		(*instance)->imageHeight = -1;
		(*instance)->channelCount = -1;
	}
}

void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData*) instance
	)
{
	if( (*instance)->resizeShaderInitialized )
		freeResizeShader( *instance );
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData*) instance
)
{
	if( (*instance)->resizeShaderInitialized )
		freeResizeShader( *instance );

	VuoRelease( (*instance)->avWriter );
	VuoRelease( (*instance)->lastUrl );
}
