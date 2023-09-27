/**
 * @file
 * VuoAudio implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudio.h"
#include "VuoPool.hh"
#include "VuoTriggerSet.hh"
#include "VuoApp.h"
#include "VuoEventLoop.h"
#include "VuoOsStatus.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <RtAudio/RtAudio.h>
#pragma clang diagnostic pop

#include <queue>
#include <CoreAudio/CoreAudio.h>
#include <objc/objc-runtime.h>


extern "C"
{

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoAudio",
					 "dependencies" : [
						 "VuoAudioSamples",
						 "VuoAudioInputDevice",
						 "VuoAudioOutputDevice",
						 "VuoList_VuoAudioSamples",
						 "VuoList_VuoAudioInputDevice",
						 "VuoList_VuoAudioOutputDevice",
						 "VuoOsStatus",
						 "rtaudio",
						 "CoreAudio.framework"
					 ]
				 });
#endif
}

const VuoInteger VuoAudio_queueSize = 8;	///< The number of buffers that must be enqueued before starting playback.

static VuoTriggerSet<VuoList_VuoAudioInputDevice>  VuoAudio_inputDeviceCallbacks;   ///< Trigger functions to call when the list of audio input devices changes.
static VuoTriggerSet<VuoList_VuoAudioOutputDevice> VuoAudio_outputDeviceCallbacks;  ///< Trigger functions to call when the list of audio output devices changes.
unsigned int VuoAudio_useCount = 0;  ///< Process-wide count of callers (typically node instances) interested in notifications about audio devices.

/**
 * Ensure we're listening for the system's device notifications.
 * https://b33p.net/kosada/vuo/vuo/-/issues/12551
 * https://web.archive.org/web/20140109183704/http://lists.apple.com/archives/Coreaudio-api/2010/Aug//msg00304.html
 */
static void __attribute__((constructor)) VuoAudio_init()
{
	// Some audio drivers (such as Jack) assume that they're being initialized on the main thread,
	// so our first audio-related call (which initializes the drivers) should be on the main thread.
	// https://b33p.net/kosada/node/12798
	dispatch_async(dispatch_get_main_queue(), ^{
		CFRunLoopRef theRunLoop = NULL;
		AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
		AudioObjectSetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
	});
}

/**
 * Returns a list of the available audio input devices.
 */
VuoList_VuoAudioInputDevice VuoAudio_getInputDevices(void)
{
	RtAudio rta;
	unsigned int deviceCount = rta.getDeviceCount();
	VuoList_VuoAudioInputDevice inputDevices = VuoListCreate_VuoAudioInputDevice();
	for (unsigned int i = 0; i < deviceCount; ++i)
	{
		RtAudio::DeviceInfo info = rta.getDeviceInfo(i);
		if (info.inputChannels)
			VuoListAppendValue_VuoAudioInputDevice(inputDevices, VuoAudioInputDevice_make(i, VuoText_make(info.modelUid.c_str()), VuoText_make(info.name.c_str()), info.inputChannels));
	}
	return inputDevices;
}

/**
 * Returns a list of the available audio output devices.
 */
VuoList_VuoAudioOutputDevice VuoAudio_getOutputDevices(void)
{
	RtAudio rta;
	unsigned int deviceCount = rta.getDeviceCount();
	VuoList_VuoAudioOutputDevice outputDevices = VuoListCreate_VuoAudioOutputDevice();
	for (unsigned int i = 0; i < deviceCount; ++i)
	{
		RtAudio::DeviceInfo info = rta.getDeviceInfo(i);
		if (info.outputChannels)
			VuoListAppendValue_VuoAudioOutputDevice(outputDevices, VuoAudioOutputDevice_make(i, VuoText_make(info.modelUid.c_str()), VuoText_make(info.name.c_str()), info.outputChannels));
	}
	return outputDevices;
}

/**
 * Invoked by Core Audio.
 */
static OSStatus VuoAudio_reconfigurationCallback(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inClientData)
{
	VuoAudio_inputDeviceCallbacks.fire(VuoAudio_getInputDevices());
	VuoAudio_outputDeviceCallbacks.fire(VuoAudio_getOutputDevices());
	return noErr;
}

/**
 * Indicates that the caller needs to get notifications about audio devices.
 *
 * @threadAny
 * @version200New
 */
void VuoAudio_use(void)
{
	if (__sync_add_and_fetch(&VuoAudio_useCount, 1) == 1)
	{
		AudioObjectPropertyAddress address;
		address.mSelector = kAudioHardwarePropertyDevices;
		address.mScope = kAudioObjectPropertyScopeGlobal;
		address.mElement = kAudioObjectPropertyElementMaster;
		OSStatus ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, &VuoAudio_reconfigurationCallback, NULL);
		if (ret)
		{
			char *description = VuoOsStatus_getText(ret);
			VUserLog("Error: Couldn't register device change listener: %s", description);
			free(description);
		}
	}
}

/**
 * Indicates that the caller no longer needs notifications about audio devices.
 *
 * @threadAny
 * @version200New
 */
void VuoAudio_disuse(void)
{
	if (VuoAudio_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoAudio_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoAudio_useCount, 1) == 0)
	{
		AudioObjectPropertyAddress address;
		address.mSelector = kAudioHardwarePropertyDevices;
		address.mScope = kAudioObjectPropertyScopeGlobal;
		address.mElement = kAudioObjectPropertyElementMaster;
		OSStatus ret = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, &VuoAudio_reconfigurationCallback, NULL);
		if (ret)
		{
			char *description = VuoOsStatus_getText(ret);
			VUserLog("Error: Couldn't unregister device change listener: %s", description);
			free(description);
		}
	}
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known audio devices changes.
 *
 * Call `VuoAudio_use()` before calling this.
 *
 * @threadAny
 * @version200New
 */
void VuoAudio_addDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice))
{
	VuoAudio_inputDeviceCallbacks.addTrigger(inputDevices);
	VuoAudio_outputDeviceCallbacks.addTrigger(outputDevices);
	inputDevices(VuoAudio_getInputDevices());
	outputDevices(VuoAudio_getOutputDevices());
}

/**
 * Removes a trigger callback previously added by @ref VuoAudio_addDevicesChangedTriggers.
 *
 * @threadAny
 * @version200New
 */
void VuoAudio_removeDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice))
{
	VuoAudio_inputDeviceCallbacks.removeTrigger(inputDevices);
	VuoAudio_outputDeviceCallbacks.removeTrigger(outputDevices);
}

/**
 * For each unique audio source (identified by `void *`), a queue of buffers to output.
 */
typedef std::map<void *, std::queue<VuoList_VuoAudioSamples> > pendingOutputType;

/**
 * For each unique audio source (identified by `void *`), the last sample value that was output on each channel.
 */
typedef std::map<void *, std::map<VuoInteger, VuoReal> > lastOutputSampleType;

/**
 * Private data for a VuoAudio instance.
 */
typedef struct _VuoAudio_internal
{
	RtAudio *rta;	///< RtAudio's device pointer.
	VuoAudioInputDevice inputDevice;		///< The device's id must be nonnegative, and channelCount must be set.
	VuoAudioOutputDevice outputDevice;		///< The device's id must be nonnegative, and channelCount must be set.

	VuoTriggerSet<VuoList_VuoAudioSamples> inputTriggers;	///< Trigger methods to call when an audio buffer is received.
	VuoTriggerSet<VuoReal> outputTriggers;	///< Trigger methods to call when an audio buffer is needed.

	dispatch_queue_t pendingOutputQueue;	///< Serializes access to the following set of buffers.
	pendingOutputType pendingOutput;		///< Sample buffers waiting to be output.

	lastOutputSampleType lastOutputSample;	///< The last sample value sent on each channel (to smoothly move back to 0 DC when there's a dropout).  Not present means no buffer was sent to the channel during the last frame.

	VuoReal priorStreamTime;				///< The time at which the prior VuoAudio_receivedEvent() happened, to track stream overflows.
} *VuoAudio_internal;

/**
 * RtAudio calls this function when a new sample buffer is ready or needed.
 */
int VuoAudio_receivedEvent(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	VuoAudio_internal ai = (VuoAudio_internal)userData;

//	if (streamTime - ai->priorStreamTime > ((float)VuoAudioSamples_bufferSize/VuoAudioSamples_sampleRate)*1.02)
//		VLog("called too late: %g (should have been < %g)", streamTime-ai->priorStreamTime, ((float)VuoAudioSamples_bufferSize/VuoAudioSamples_sampleRate)*1.02);
//	ai->priorStreamTime=streamTime;

	if (status)
		VUserLog("Stream %s (%d) on %s.",
				 status == RTAUDIO_INPUT_OVERFLOW ? "overflow"
					 : (status == RTAUDIO_OUTPUT_UNDERFLOW ? "underflow" : "error"),
				 status,
				 ai->inputDevice.name);

	// Fire triggers requesting audio output buffers.
	ai->outputTriggers.fire(streamTime);

	// Fire triggers providing audio input buffers.
	if (ai->inputTriggers.size())
	{
		VuoList_VuoAudioSamples channels = VuoListCreate_VuoAudioSamples();
		VuoRetain(channels);

		unsigned int samplesPerSecond = ai->rta->getStreamSampleRate();

		// When creating the stream, we requested that inputBuffer be non-interleaved, so the samples should appear consecutively.
		for (VuoInteger i = 0; i < ai->inputDevice.channelCount; ++i)
		{
			VuoAudioSamples samples = VuoAudioSamples_alloc(nBufferFrames);
			samples.samplesPerSecond = samplesPerSecond;
			memcpy(samples.samples, (VuoReal *)inputBuffer + i*nBufferFrames, sizeof(VuoReal)*nBufferFrames);
			VuoListAppendValue_VuoAudioSamples(channels, samples);
		}

		ai->inputTriggers.fire(channels);
		VuoRelease(channels);
	}

	// Zero the final output buffer.
	double *outputBufferDouble = (double *)outputBuffer;
	unsigned int outputChannelCount = ai->outputDevice.channelCount;
	if (outputBuffer)
		memset(outputBufferDouble, 0, nBufferFrames*sizeof(VuoReal)*outputChannelCount);

	// Process the pending output buffers.
	dispatch_sync(ai->pendingOutputQueue, ^{
					  if (!outputBuffer)
					  {
						  // If there are any pending buffers, squander them.
						  bool pendingOutput = false;
						  for (pendingOutputType::iterator it = ai->pendingOutput.begin(); it != ai->pendingOutput.end(); ++it)
							  while (!it->second.empty())
							  {
								  VuoRelease(it->second.front());
								  it->second.pop();
								  pendingOutput = true;
							  }

						  if (pendingOutput)
							  VUserLog("This audio device (%s) doesn't support output.", ai->outputDevice.name);
						  return;
					  }

					  // Mix the next buffer from each source into a single output buffer.
					  /// @todo handle differing sample rates
					  for (pendingOutputType::iterator it = ai->pendingOutput.begin(); it != ai->pendingOutput.end(); ++it)
					  {
						  void *id = it->first;
						  if (it->second.empty())	// No pending buffers for this source.
						  {
							  if (ai->lastOutputSample.find(id) == ai->lastOutputSample.end())
								  continue;

							  for (unsigned int channel = 0; channel < outputChannelCount; ++channel)
							  {
								  if (ai->lastOutputSample[id].find(channel) == ai->lastOutputSample[id].end())
									  continue;

								  // Since this channel was previously playing audio, smoothly fade the last amplitude to zero...
								  VuoReal lastOutputSample = ai->lastOutputSample[id][channel];
								  for (VuoInteger i=0; i<nBufferFrames; ++i)
									  outputBufferDouble[nBufferFrames*(channel) + i] = VuoReal_lerp(lastOutputSample, 0, (float)i/nBufferFrames);

								  // ...and indicate that we already faded out.
								  ai->lastOutputSample[id].erase(ai->lastOutputSample[id].find(channel));
							  }
						  }
						  else	// Have pending buffers for this source.
						  {
							  VuoList_VuoAudioSamples channels = it->second.front();

							  for (unsigned int channel = 0; channel < outputChannelCount; ++channel)
							  {
								  VuoAudioSamples as = VuoListGetValue_VuoAudioSamples(channels, channel+1);
								  if (!as.samples || as.sampleCount == 0)
									  continue;

								  if (ai->lastOutputSample[id].find(channel) == ai->lastOutputSample[id].end())
								  {
									  // This is the first sample buffer ever, or the first after a dropout.
									  // Make sure the queue is primed with a few buffers before we start draining it.
									  if (it->second.size() < VuoAudio_queueSize)
										  goto skipPop;

									  // Smoothly fade from zero to the sample buffer.
									  for (VuoInteger i=0; i<nBufferFrames; ++i)
										  outputBufferDouble[nBufferFrames*channel + i] = (float)i/nBufferFrames * as.samples[i];
								  }
								  else
								  {
									  // We were previously playing audio, so just copy the samples intact.
									  for (VuoInteger i=0; i<nBufferFrames; ++i)
										  /// @todo Should we clamp here (or after all buffers for this channel have been summed), or does CoreAudio handle that for us?
										  outputBufferDouble[nBufferFrames*(channel) + i] += as.samples[i];
								  }

								  ai->lastOutputSample[id][channel] = as.samples[as.sampleCount-1];
							  }

							  it->second.pop();
							  VuoRelease(channels);

							  skipPop:;
						  }
					  }
				  });
	return 0;
}

/**
 * Routes RtAudio messages through VuoLog.
 */
void VuoAudio_rtAudioError(RtAudioError::Type type, const std::string &errorText)
{
	VUserLog("%s: %s",
		(type == RtAudioError::WARNING || type == RtAudioError::DEBUG_WARNING) ? "Warning" : "Error",
		errorText.c_str());
}

/// @{
VUOKEYEDPOOL(unsigned int, VuoAudio_internal);
static void VuoAudio_destroy(VuoAudio_internal ai);
VuoAudio_internal VuoAudio_make(unsigned int deviceId)
{
	VuoAudio_internal ai = NULL;
	try
	{
		Class avCaptureDeviceClass = objc_getClass("AVCaptureDevice");
		if (class_getClassMethod(avCaptureDeviceClass, sel_getUid("authorizationStatusForMediaType:")))
		{
			CFStringRef mediaType = CFStringCreateWithCString(NULL, "soun", kCFStringEncodingUTF8);
			long status = ((long (*)(id, SEL, CFStringRef))objc_msgSend)((id)avCaptureDeviceClass, sel_getUid("authorizationStatusForMediaType:"), mediaType);
			CFRelease(mediaType);

			if (status == 0 /* AVAuthorizationStatusNotDetermined */)
				VUserLog("Warning: Audio input may be unavailable due to system restrictions.  Check System Settings > Privacy & Security > Microphone.");
			else if (status == 1 /* AVAuthorizationStatusRestricted */
				  || status == 2 /* AVAuthorizationStatusDenied */)
				VUserLog("Error: Audio input is unavailable due to system restrictions.  Check System Settings > Privacy & Security > Microphone.");
		}

		ai = new _VuoAudio_internal;
		ai->inputDevice.id = deviceId;
		ai->outputDevice.id = deviceId;

		ai->pendingOutputQueue = dispatch_queue_create("VuoAudio pending output", VuoEventLoop_getDispatchInteractiveAttribute());

		// Though neither RtAudio's documentation nor Apple's documentation
		// specify that audio must be initialized on the main thread,
		// some audio drivers (such as Jack) make that assumption.
		// https://b33p.net/kosada/node/12068
		VuoApp_executeOnMainThread(^{
									   ai->rta = new RtAudio();
								   });

		RtAudio::StreamParameters inputParameters;
		inputParameters.deviceId = deviceId;
		inputParameters.nChannels = ai->rta->getDeviceInfo(deviceId).inputChannels;
		ai->inputDevice.name = VuoText_make(ai->rta->getDeviceInfo(deviceId).name.c_str());
		VuoRetain(ai->inputDevice.name);
		ai->inputDevice.channelCount = inputParameters.nChannels;

		RtAudio::StreamParameters outputParameters;
		outputParameters.deviceId = deviceId;
		outputParameters.nChannels = ai->rta->getDeviceInfo(deviceId).outputChannels;
		ai->outputDevice.name = VuoText_make(ai->rta->getDeviceInfo(deviceId).name.c_str());
		VuoRetain(ai->outputDevice.name);
		ai->outputDevice.channelCount = outputParameters.nChannels;

		RtAudio::StreamOptions options;
		options.flags = RTAUDIO_NONINTERLEAVED;

		unsigned int bufferFrames = VuoAudioSamples_bufferSize;

		ai->rta->openStream(
					outputParameters.nChannels ? &outputParameters : NULL,
					inputParameters.nChannels ? &inputParameters : NULL,
					RTAUDIO_FLOAT64,
					VuoAudioSamples_sampleRate,
					&bufferFrames,
					&VuoAudio_receivedEvent,
					ai,
					&options,
					VuoAudio_rtAudioError);
		ai->rta->startStream();
	}
	catch (RtAudioError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to open the audio device (%d): %s", deviceId, error.what());

		if (ai)
		{
			delete ai->rta;
			delete ai;
			ai = NULL;
		}
	}

	if (ai)
		VuoRegister(ai, (DeallocateFunctionType)VuoAudio_destroy);

	return ai;
}
static void VuoAudio_destroy(VuoAudio_internal ai)
{
	try
	{
		if (ai->rta->isStreamOpen())
		{
			ai->rta->stopStream();
			ai->rta->closeStream();
		}
	}
	catch (RtAudioError &error)
	{
		VUserLog("Failed to close the audio device (%s): %s", ai->inputDevice.name, error.what());
	}

	// Now that the audio stream is stopped (and the last callback has returned), it's safe to delete the queue.
	dispatch_release(ai->pendingOutputQueue);

	// Release any leftover buffers.
	for (pendingOutputType::iterator it = ai->pendingOutput.begin(); it != ai->pendingOutput.end(); ++it)
		while (!it->second.empty())
		{
			VuoRelease(it->second.front());
			it->second.pop();
		}

	delete ai->rta;
	VuoRelease(ai->inputDevice.name);
	VuoRelease(ai->outputDevice.name);
	delete ai;
}
VUOKEYEDPOOL_DEFINE(unsigned int, VuoAudio_internal, VuoAudio_make);
/// @}

/**
 * @copydoc VuoKeyedPool<std::string,VuoAudio_internal>::useSharedInstance
 */
VuoAudioIn VuoAudioIn_useShared(VuoAudioInputDevice aid)
{
	VuoAudioInputDevice realizedDevice;
	if (!VuoAudioInputDevice_realize(aid, &realizedDevice))
		return nullptr;

	VuoAudioInputDevice_retain(realizedDevice);
	VuoAudioIn ai = static_cast<VuoAudioIn>(VuoAudio_internalPool->useSharedInstance(realizedDevice.id));
	VuoAudioInputDevice_release(realizedDevice);

	return ai;
}

/**
 * @copydoc VuoKeyedPool<std::string,VuoAudio_internal>::disuseSharedInstance
 */
void VuoAudioIn_disuseShared(VuoAudioIn ai)
{
	VuoAudio_internalPool->disuseSharedInstance(static_cast<VuoAudio_internal>(ai));
}

/**
 * @copydoc VuoKeyedPool<std::string,VuoAudio_internal>::useSharedInstance
 */
VuoAudioOut VuoAudioOut_useShared(VuoAudioOutputDevice aod)
{
	VuoAudioOutputDevice realizedDevice;
	if (!VuoAudioOutputDevice_realize(aod, &realizedDevice))
		return nullptr;

	VuoAudioOutputDevice_retain(realizedDevice);
	VuoAudioOut ao = static_cast<VuoAudioOut>(VuoAudio_internalPool->useSharedInstance(realizedDevice.id));
	VuoAudioOutputDevice_release(realizedDevice);

	return ao;
}

/**
 * @copydoc VuoKeyedPool<std::string,VuoAudio_internal>::disuseSharedInstance
 */
void VuoAudioOut_disuseShared(VuoAudioOut ao)
{
	VuoAudio_internalPool->disuseSharedInstance(static_cast<VuoAudio_internal>(ao));
}

/**
 * Sets up the audio input device to call the trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoAudioIn_addTrigger
(
		VuoAudioIn ai,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	if (!ai)
		return;

	VuoAudio_internal aii = (VuoAudio_internal)ai;
	aii->inputTriggers.addTrigger(receivedChannels);
}

/**
 * Sets up the audio output device to call the trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoAudioOut_addTrigger
(
		VuoAudioOut ao,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	if (!ao)
		return;

	VuoAudio_internal aii = (VuoAudio_internal)ao;
	aii->outputTriggers.addTrigger(requestedChannels);
}

/**
 * Stops the audio input device from calling trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoAudioIn_removeTrigger
(
		VuoAudioIn ai,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
)
{
	if (!ai)
		return;

	VuoAudio_internal aii = (VuoAudio_internal)ai;
	aii->inputTriggers.removeTrigger(receivedChannels);
}

/**
 * Stops the audio output device from calling trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoAudioOut_removeTrigger
(
		VuoAudioOut ao,
		VuoOutputTrigger(requestedChannels, VuoReal)
)
{
	if (!ao)
		return;

	VuoAudio_internal aii = (VuoAudio_internal)ao;
	aii->outputTriggers.removeTrigger(requestedChannels);
}

/**
 * Enqueues @c channels for eventual playback.
 *
 * @c id should be a unique identifier for the audio stream (e.g., a pointer to the node's instance data).
 *
 * When multiple sources are simultaneously sending to the same audio device,
 * each is buffered independently — each unique @c id gets its own queue,
 * and at output time a single buffer from each source's queue is mixed
 * to form the final output stream.
 */
void VuoAudioOut_sendChannels(VuoAudioOut ao, VuoList_VuoAudioSamples channels, void *id)
{
	if (!ao)
		return;

	VuoRetain(channels);
	VuoAudio_internal aii = (VuoAudio_internal)ao;
	dispatch_async(aii->pendingOutputQueue, ^{
					  aii->pendingOutput[id].push(channels);
				  });
}

/// Helper for VuoAudioInputDevice_realize.
#define setRealizedDevice(newDevice) \
	realizedDevice->id = newDevice.id; \
	realizedDevice->modelUid = VuoText_make(newDevice.modelUid); \
	realizedDevice->name = VuoText_make(newDevice.name); \
	realizedDevice->channelCount = newDevice.channelCount;

/**
 * If `device`'s channel count is unknown (zero):
 *
 *    - If a matching device is present, sets `realizedDevice` to that device, and returns true.
 *    - If no matching device is present, returns false, leaving `realizedDevice` unset.
 *
 * If `device`'s channel count is already known (presumably from the `List Audio Devices` node),
 * sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the device is currently present.)
 *
 * @threadAny
 */
bool VuoAudioInputDevice_realize(VuoAudioInputDevice device, VuoAudioInputDevice *realizedDevice)
{
	// Already have channel count; nothing to do.
	if (device.channelCount > 0)
	{
		setRealizedDevice(device);
		return true;
	}

	// Otherwise, try to find a matching device.

	VUserLog("Requested device:          %s", json_object_to_json_string(VuoAudioInputDevice_getJson(device)));
	VuoList_VuoAudioInputDevice devices = VuoAudio_getInputDevices();
	VuoLocal(devices);
	__block bool found = false;

	// First pass: try to find an exact match by ID.
	VuoListForeach_VuoAudioInputDevice(devices, ^(const VuoAudioInputDevice item){
		if (device.id != -1 && device.id == item.id)
		{
			VUserLog("Matched by ID:             %s",json_object_to_json_string(VuoAudioInputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
			return false;
		}
		return true;
	});

	// Second pass: try to find a match by model AND name.
	// (Try AND first since, for example, Soundflower creates multiple devices with different names but the same model.)
	if (!found)
		VuoListForeach_VuoAudioInputDevice(devices, ^(const VuoAudioInputDevice item){
			if (device.id == -1 && !VuoText_isEmpty(device.modelUid) && !VuoText_isEmpty(device.name)
			 && VuoText_compare(item.modelUid, (VuoTextComparison){VuoTextComparison_Contains, true}, device.modelUid)
			 && VuoText_compare(item.name,     (VuoTextComparison){VuoTextComparison_Contains, true}, device.name))
			{
				VUserLog("Matched by model and name: %s",json_object_to_json_string(VuoAudioInputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Third pass: try to find a loose match by model OR name.
	if (!found)
		VuoListForeach_VuoAudioInputDevice(devices, ^(const VuoAudioInputDevice item){
			if ((!VuoText_isEmpty(device.modelUid) && VuoText_compare(item.modelUid, (VuoTextComparison){VuoTextComparison_Contains, true}, device.modelUid))
			 || (!VuoText_isEmpty(device.name)     && VuoText_compare(item.name,     (VuoTextComparison){VuoTextComparison_Contains, true}, device.name)))
			{
				VUserLog("Matched by model or name:  %s",json_object_to_json_string(VuoAudioInputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Fourth pass: if the user hasn't specified a device, use the default device.
	if (!found && device.id == -1 && VuoText_isEmpty(device.modelUid) && VuoText_isEmpty(device.name))
	{
		__block RtAudio *temporaryRTA;  // Just for getting device info prior to opening a shared device.

		try
		{
			// https://b33p.net/kosada/node/12068
			VuoApp_executeOnMainThread(^{
				temporaryRTA = new RtAudio();
			});

			int defaultID = temporaryRTA->getDefaultInputDevice();
			VuoListForeach_VuoAudioInputDevice(devices, ^(const VuoAudioInputDevice item){
				if (item.id == defaultID)
				{
					VUserLog("Using default device:      %s",json_object_to_json_string(VuoAudioInputDevice_getJson(item)));
					setRealizedDevice(item);
					found = true;
					return false;
				}
				return true;
			});

			delete temporaryRTA;
		}
		catch (RtAudioError &error)
		{
			VUserLog("Error: Couldn't enumerate audio devices: %s", error.what());
			delete temporaryRTA;
		}
	}

	if (!found)
		VUserLog("No matching device found.");

	return found;
}

/**
 * If `device`'s channel count is unknown (zero):
 *
 *    - If a matching device is present, sets `realizedDevice` to that device, and returns true.
 *    - If no matching device is present, returns false, leaving `realizedDevice` unset.
 *
 * If `device`'s channel count is already known (presumably from the `List Audio Devices` node),
 * sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the device is currently present.)
 *
 * @threadAny
 */
bool VuoAudioOutputDevice_realize(VuoAudioOutputDevice device, VuoAudioOutputDevice *realizedDevice)
{
	// Already have channel count; nothing to do.
	if (device.channelCount > 0)
	{
		setRealizedDevice(device);
		return true;
	}

	// Otherwise, try to find a matching device.

	VUserLog("Requested device:          %s", json_object_to_json_string(VuoAudioOutputDevice_getJson(device)));
	VuoList_VuoAudioOutputDevice devices = VuoAudio_getOutputDevices();
	VuoLocal(devices);
	__block bool found = false;

	// First pass: try to find an exact match by ID.
	VuoListForeach_VuoAudioOutputDevice(devices, ^(const VuoAudioOutputDevice item){
		if (device.id != -1 && device.id == item.id)
		{
			VUserLog("Matched by ID:             %s",json_object_to_json_string(VuoAudioOutputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
			return false;
		}
		return true;
	});

	// Second pass: try to find a match by model AND name.
	// (Try AND first since, for example, Soundflower creates multiple devices with different names but the same model.)
	if (!found)
		VuoListForeach_VuoAudioOutputDevice(devices, ^(const VuoAudioOutputDevice item){
			if (device.id == -1 && !VuoText_isEmpty(device.modelUid) && !VuoText_isEmpty(device.name)
				&& VuoText_compare(item.modelUid, (VuoTextComparison){VuoTextComparison_Contains, true}, device.modelUid)
				&& VuoText_compare(item.name,     (VuoTextComparison){VuoTextComparison_Contains, true}, device.name))
			{
				VUserLog("Matched by model and name: %s",json_object_to_json_string(VuoAudioOutputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Third pass: try to find a loose match by model OR name.
	if (!found)
		VuoListForeach_VuoAudioOutputDevice(devices, ^(const VuoAudioOutputDevice item){
			if ((!VuoText_isEmpty(device.modelUid) && VuoText_compare(item.modelUid, (VuoTextComparison){VuoTextComparison_Contains, true}, device.modelUid))
			 || (!VuoText_isEmpty(device.name)     && VuoText_compare(item.name,     (VuoTextComparison){VuoTextComparison_Contains, true}, device.name)))
			{
				VUserLog("Matched by model or name:  %s",json_object_to_json_string(VuoAudioOutputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Fourth pass: if the user hasn't specified a device, use the default device.
	if (!found && device.id == -1 && VuoText_isEmpty(device.modelUid) && VuoText_isEmpty(device.name))
	{
		__block RtAudio *temporaryRTA;  // Just for getting device info prior to opening a shared device.

		try
		{
			// https://b33p.net/kosada/node/12068
			VuoApp_executeOnMainThread(^{
				temporaryRTA = new RtAudio();
			});

			int defaultID = temporaryRTA->getDefaultOutputDevice();
			VuoListForeach_VuoAudioOutputDevice(devices, ^(const VuoAudioOutputDevice item){
				if (item.id == defaultID)
				{
					VUserLog("Using default device:      %s",json_object_to_json_string(VuoAudioOutputDevice_getJson(item)));
					setRealizedDevice(item);
					found = true;
					return false;
				}
				return true;
			});

			delete temporaryRTA;
		}
		catch (RtAudioError &error)
		{
			VUserLog("Error: Couldn't enumerate audio devices: %s", error.what());
			delete temporaryRTA;
		}
	}

	if (!found)
		VUserLog("No matching device found.");

	return found;
}
