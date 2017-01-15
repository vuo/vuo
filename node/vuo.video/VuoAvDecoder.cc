/**
 * @file
 * VuoAvDecoder implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoAvDecoder.h"

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAvDecoder",
					 "dependencies" : [
						"VuoVideoDecoder",
						"VuoAvPlayerInterface",
						"VuoAvPlayerObject",
						"VuoImage",
						"VuoAudioFrame",
						"VuoAudioSamples",
						"VuoReal",
						"VuoList_VuoAudioSamples"
					 ]
				 });
#endif
}

/**
 * Callback invoked when playback is ready.
 */
void OnDecoderPlaybackReady(void* id, bool canPlayMedia)
{
	VuoAvDecoder* decoder = (VuoAvDecoder*) id;

	if(decoder != NULL)
		decoder->DecoderReady(canPlayMedia);
}

void VuoAvDecoder::DecoderReady(bool canPlayMedia)
{
	if(videoPlayer != NULL && onReadyToPlay != NULL)
	{
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void)
		{
			(videoPlayer->*onReadyToPlay)(canPlayMedia);
		});
	}
}

VuoAvDecoder* VuoAvDecoder::Create(VuoUrl url)
{
	VuoAvDecoder* decoder = new VuoAvDecoder();

	dispatch_sync(decoder->queue, ^
	{
		decoder->player = VuoAvPlayer_make(url);

		if(decoder->player != NULL)
		{
			VuoAvPlayer_setOnPlaybackReadyCallbackObject(decoder->player, OnDecoderPlaybackReady, decoder);
		}
	});

	return decoder;
}

VuoAvDecoder::~VuoAvDecoder()
{
	videoPlayer = NULL;
	onReadyToPlay = NULL;

	if(player != NULL)
	{
		dispatch_sync(queue, ^
		{
			VuoAvPlayer_setOnPlaybackReadyCallbackObject(player, NULL, NULL);
			VuoAvPlayer_release(player);
		});
	}

	dispatch_release(queue);
}

bool VuoAvDecoder::IsReady()
{
	__block bool isReady = false;
	dispatch_sync(queue, ^{
		isReady = VuoAvPlayer_isReady(player);
	});
	return isReady;
}

bool VuoAvDecoder::CanPlayAudio()
{
	__block bool canPlayAudio = false;
	dispatch_sync(queue, ^{
		canPlayAudio = VuoAvPlayer_canPlayAudio(player);
	});
	return canPlayAudio;
}

unsigned int VuoAvDecoder::GetAudioChannelCount()
{
	__block unsigned int count = 0;
	dispatch_sync(queue, ^{
		count = VuoAvPlayer_audioChannelCount(player);
	});
	return count;
}

bool VuoAvDecoder::NextVideoFrame(VuoVideoFrame* frame)
{
	__block bool success = false;
	dispatch_sync(queue, ^{
		success = VuoAvPlayer_nextVideoFrame(player, frame);
	});
	return success;
}

bool VuoAvDecoder::NextAudioFrame(VuoAudioFrame* frame)
{
	__block bool success = false;
	dispatch_sync(queue, ^{
		success = VuoAvPlayer_nextAudioFrame(player, frame);
	});
	return success;
}

bool VuoAvDecoder::SeekToSecond(double second)
{
	__block bool success = false;
	dispatch_sync(queue, ^{
		success = VuoAvPlayer_seekToSecond(player, second);
	});
	return success;
}

double VuoAvDecoder::GetDuration()
{
	__block double duration = 0;
	dispatch_sync(queue, ^{
		duration = VuoAvPlayer_getDuration(player);
	});
	return duration;
}

double VuoAvDecoder::GetFrameRate()
{
	__block double fps = 0;
	dispatch_sync(queue, ^{
		fps = VuoAvPlayer_getFrameRate(player);
	});
	return fps;
}

void VuoAvDecoder::SetPlaybackRate(double second)
{
	dispatch_sync(queue, ^{
		VuoAvPlayer_setPlaybackRate(player, second);
	});
}

double VuoAvDecoder::GetLastDecodedVideoTimeStamp()
{
	__block double timestamp = 0;
	dispatch_sync(queue, ^{
		timestamp = VuoAvPlayer_getCurrentTimestamp(player);
	});
	return timestamp;
}
