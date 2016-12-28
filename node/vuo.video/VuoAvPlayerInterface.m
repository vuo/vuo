
#include "VuoAvPlayerInterface.h"
#include "VuoAvPlayerObject.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAvPlayerInterface",
					 "dependencies" : [
						"VuoAvPlayerObject"
					 ]
				 });
#endif

VuoAvPlayerObjPtr VuoAvPlayer_make(const char* url)
{
	VuoAvPlayerObject* player = [[VuoAvPlayerObject alloc] init];

	// [NSURL URLWithString:] returns nil if the string contains UTF-8 characters,
	// and [NSString stringByAddingPercentEscapesUsingEncoding:] is deprecated (and results in double-escaped ASCII-7 characters),
	// so just feed it a path; [NSURL fileURLWithPath:] can deal with UTF-8 characters.
	VuoText path = VuoUrl_getPosixPath(url);
	VuoRetain(path);
	NSURL *file_url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path] isDirectory:NO];
	VuoRelease(path);

	if( ![player setURL:file_url] )
	{
		[player release];
		player = NULL;
	}

	return player;
}

void VuoAvPlayer_release(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*)player;
	[obj release];
}

bool VuoAvPlayer_isReady(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*)player;
	return [obj canBeginPlayback];
}

void VuoAvPlayer_setPlaybackRate(VuoAvPlayerObjPtr player, VuoReal rate)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*)player;
	[obj setPlaybackRate:rate];
}

double VuoAvPlayer_getFrameRate(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*)player;
	return [obj getFrameRate];
}

bool VuoAvPlayer_seekToSecond(VuoAvPlayerObjPtr player, VuoReal second)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*)player;
	return [obj seekToSecond:second withRange:-1];
}

void VuoAvPlayer_setOnPlaybackReadyCallbackObject(VuoAvPlayerObjPtr player, void (*callback)(void* id, bool canPlayMedia), void* id)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	[obj setPlayerCallback:callback target:id];
}

bool VuoAvPlayer_nextVideoFrame(VuoAvPlayerObjPtr player, VuoVideoFrame* videoFrame)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj nextVideoFrame:videoFrame];
}

bool VuoAvPlayer_nextAudioFrame(VuoAvPlayerObjPtr player, VuoAudioFrame* audioFrame)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj nextAudioFrame:audioFrame];
}

VuoReal VuoAvPlayer_getCurrentTimestamp(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj getCurrentTimestamp];
}

VuoReal VuoAvPlayer_getDuration(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj getDuration];
}

bool VuoAvPlayer_canPlayAudio(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj canPlayAudio];
}

unsigned int VuoAvPlayer_audioChannelCount(VuoAvPlayerObjPtr player)
{
	VuoAvPlayerObject* obj = (VuoAvPlayerObject*) player;
	return [obj audioChannels];
}
