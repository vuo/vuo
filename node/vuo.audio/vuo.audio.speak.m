/**
 * @file
 * vuo.audio.speak node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioFile.h"

#include "VuoCompositionState.h"
#include "VuoSpeechVoice.h"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

VuoModuleMetadata({
	"title" : "Speak",
	"keywords" : [
		"sound",
		"speech synthesis", "say", "talk", "pronounce", "vocal",
		"string",
	],
	"version" : "1.0.1",
	"dependencies" : [
		"VuoAudioFile"
	],
	"node" : {
		"exampleCompositions" : [ "SpeakGreetings.vuo" ]
	}
});

@interface VuoAudioSpeakDelegate : NSObject<NSSpeechSynthesizerDelegate>
@property struct nodeInstanceData *context;
@property void *compositionState;
@end

struct nodeInstanceData
{
	NSSpeechSynthesizer *synth;
	VuoAudioSpeakDelegate *delegate;
	VuoAudioFile af;
	VuoText currentPlaybackFile;
	dispatch_semaphore_t finishedSynthesizing;

	double wordsPerMinute;
	double pitch;
	double modulation;

	void (*spokenChannels)(VuoList_VuoAudioSamples);
	void (*finishedSpeaking)(void);

	bool triggersEnabled;
};

@implementation VuoAudioSpeakDelegate
@synthesize context;
@synthesize compositionState;
- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender didFinishSpeaking:(BOOL)finishedSpeaking
{
	if (!context->triggersEnabled)
		return;

	if (context->af)
		VuoRelease(context->af);

	vuoAddCompositionStateToThreadLocalStorage(compositionState);

	context->af = VuoAudioFile_make(context->currentPlaybackFile);
	VuoRetain(context->af);

	VuoAudioFile_enableTriggers(context->af, context->spokenChannels, context->finishedSpeaking);

	VuoAudioFile_play(context->af);

	remove(context->currentPlaybackFile);
	VuoRelease(context->currentPlaybackFile);
	context->currentPlaybackFile = NULL;

	dispatch_semaphore_signal(context->finishedSynthesizing);

	vuoRemoveCompositionStateFromThreadLocalStorage();
}
@end

struct nodeInstanceData *nodeInstanceInit
(
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->delegate = [VuoAudioSpeakDelegate new];
	context->delegate.context = context;
	context->delegate.compositionState = vuoCopyCompositionStateFromThreadLocalStorage();
	context->finishedSynthesizing = dispatch_semaphore_create(1);

	return context;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoOutputTrigger(spokenChannels, VuoList_VuoAudioSamples, {"eventThrottling":"enqueue"}),
	VuoOutputTrigger(finishedSpeaking, void)
)
{
	(*context)->triggersEnabled = true;
	(*context)->spokenChannels = spokenChannels;
	(*context)->finishedSpeaking = finishedSpeaking;
	if ((*context)->af)
		VuoAudioFile_enableTriggers((*context)->af, (*context)->spokenChannels, (*context)->finishedSpeaking);
}

double VuoAudioSpeak_midiNoteFromFrequency(double frequency)
{
	// https://en.wikipedia.org/wiki/MIDI_Tuning_Standard
	return 12. * log2(frequency / 440.) + 69.;
}

double VuoAudioSpeak_frequencyFromMidiNote(double note)
{
	return pow(2., (note - 69.) / 12.) * 440.;
}

void nodeInstanceEvent
(
	VuoInputEvent() speak,
	VuoInputEvent() stop,
	VuoInputData(VuoText) text,
	VuoInputData(VuoSpeechVoice, {"default":"com.apple.speech.synthesis.voice.Alex"}) voice,
	VuoInputData(VuoReal, {"default":175, "auto":infinity, "autoSupersedesDefault":true, "suggestedMin":100, "suggestedMax":300, "suggestedStep":10, "name":"Words per Minute"}) wordsPerMinute,
	VuoInputData(VuoReal, {"default":100, "auto":infinity, "autoSupersedesDefault":true, "suggestedMin":50, "suggestedMax":300, "suggestedStep":10}) pitch,
	VuoInputData(VuoReal, {"default":0.5, "auto":infinity, "autoSupersedesDefault":true, "suggestedMin":0, "suggestedMax":1, "suggestedStep":0.1}) modulation,

	VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (!(*context)->triggersEnabled)
		return;

	if (speak && !VuoText_isEmpty(text))
	{
		NSString *voiceNS = [NSString stringWithUTF8String:voice];

		if (!(*context)->synth
		 || ![voiceNS isEqualToString:(*context)->synth.voice]
		 || wordsPerMinute != (*context)->wordsPerMinute
		 || pitch != (*context)->pitch
		 || modulation != (*context)->modulation)
		{
			[(*context)->synth release];
			(*context)->synth = [NSSpeechSynthesizer new];
			(*context)->synth.delegate = (*context)->delegate;

			(*context)->synth.voice = voiceNS;

			(*context)->wordsPerMinute = wordsPerMinute;
			if (!isinf(wordsPerMinute))
			{
				NSError *error;
				if (![(*context)->synth setObject:[NSNumber numberWithDouble:wordsPerMinute] forProperty:NSSpeechRateProperty error:&error])
					VUserLog("Warning: Couldn't set rate to %g words per minute: %s — %s",
							 wordsPerMinute,
							 [[error localizedDescription] UTF8String],
							[[[[error userInfo] objectForKey:NSUnderlyingErrorKey] localizedDescription] UTF8String]);
			}

			(*context)->pitch = pitch;
			if (!isinf(pitch))
			{
				NSError *error;
				// numberWithDouble: doesn't work — the speech is always low-pitch.
				if (![(*context)->synth setObject:[NSNumber numberWithInt:VuoAudioSpeak_midiNoteFromFrequency(pitch)] forProperty:NSSpeechPitchBaseProperty error:&error])
					VUserLog("Warning: Couldn't set pitch %g Hz (%d): %s — %s",
							 pitch,
							 (int)VuoAudioSpeak_midiNoteFromFrequency(pitch),
							 [[error localizedDescription] UTF8String],
							 [[[[error userInfo] objectForKey:NSUnderlyingErrorKey] localizedDescription] UTF8String]);
			}

			(*context)->modulation = modulation;
			if (!isinf(modulation))
			{
				int modulationNotes = modulation * 200;
				NSError *error;
				// numberWithDouble: doesn't work — the speech is always monotone.
				if (![(*context)->synth setObject:[NSNumber numberWithInt:modulationNotes] forProperty:NSSpeechPitchModProperty error:&error])
					VUserLog("Warning: Couldn't set modulation %g Hz (%d): %s — %s",
							 modulation,
							 modulationNotes,
							 [[error localizedDescription] UTF8String],
							 [[[[error userInfo] objectForKey:NSUnderlyingErrorKey] localizedDescription] UTF8String]);
			}
		}

		NSString *textNS = [NSString stringWithUTF8String:text];
		NSString *pathTemplateNS = [NSString stringWithFormat:@"%@VuoAudioSpeak-XXXXXX.aiff", NSTemporaryDirectory()];
		(*context)->currentPlaybackFile = VuoText_make([pathTemplateNS UTF8String]);
		VuoRetain((*context)->currentPlaybackFile);
		int fd = mkstemps((char *)(*context)->currentPlaybackFile, strlen(".aiff"));
		close(fd);

		NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:(*context)->currentPlaybackFile]];
		dispatch_semaphore_wait((*context)->finishedSynthesizing, DISPATCH_TIME_FOREVER);
		[(*context)->synth startSpeakingString:textNS toURL:url];
	}

	if (stop && (*context)->af)
		VuoAudioFile_pause((*context)->af);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	dispatch_semaphore_wait((*context)->finishedSynthesizing, DISPATCH_TIME_FOREVER);
	dispatch_semaphore_signal((*context)->finishedSynthesizing);

	if ((*context)->af)
		VuoAudioFile_disableTriggers((*context)->af);
	(*context)->spokenChannels = NULL;
	(*context)->finishedSpeaking = NULL;
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	dispatch_release((*context)->finishedSynthesizing);

	if ((*context)->af)
		VuoRelease((*context)->af);
	[(*context)->synth release];
	[(*context)->delegate release];
}
