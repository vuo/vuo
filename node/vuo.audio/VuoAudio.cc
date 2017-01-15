/**
 * @file
 * VuoAudio implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoAudio.h"
#include "VuoPool.hh"
#include "VuoTriggerSet.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include "RtAudio.h"
#pragma clang diagnostic pop

#include <dispatch/dispatch.h>
#include <map>
#include <queue>
#include <set>


extern "C"
{
#include "module.h"

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
						 "RtAudio",
						 "CoreAudio.framework"
					 ]
				 });
#endif
}

const VuoInteger VuoAudio_queueSize = 8;	///< The number of buffers that must be enqueued before starting playback.

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
			VuoListAppendValue_VuoAudioInputDevice(inputDevices, VuoAudioInputDevice_make(i, VuoText_make(info.name.c_str()), info.inputChannels));
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
			VuoListAppendValue_VuoAudioOutputDevice(outputDevices, VuoAudioOutputDevice_make(i, VuoText_make(info.name.c_str()), info.outputChannels));
	}
	return outputDevices;
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
		VUserLog("Stream overflow on %s.", ai->inputDevice.name);

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

/// @{
VUOKEYEDPOOL(unsigned int, VuoAudio_internal);
static void VuoAudio_destroy(VuoAudio_internal ai);
VuoAudio_internal VuoAudio_make(unsigned int deviceId)
{
	VuoAudio_internal ai = NULL;
	try
	{
		ai = new _VuoAudio_internal;
		ai->inputDevice.id = deviceId;
		ai->outputDevice.id = deviceId;

		ai->pendingOutputQueue = dispatch_queue_create("VuoAudio pending output", NULL);

		ai->rta = new RtAudio();

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
					&options
					);
		ai->rta->startStream();
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to open the audio device (%d) :: %s.\n", deviceId, error.what());

		if (ai)
		{
			if (ai->rta)
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
	VuoAudio_internalPool->removeSharedInstance(ai->inputDevice.id);

	try
	{
		if (ai->rta->isStreamOpen())
		{
			ai->rta->stopStream();
			ai->rta->closeStream();
		}
	}
	catch (RtError &error)
	{
		VUserLog("Failed to close the audio device (%s) :: %s.\n", ai->inputDevice.name, error.what());
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
 * Returns the reference-counted object for the specified audio input device.
 */
VuoAudioIn VuoAudioIn_getShared(VuoAudioInputDevice aid)
{
	int deviceId = -1;

	try
	{
		RtAudio temporaryRTA;	// Just for getting device info prior to opening a shared device.

		if (aid.id == -1 && strlen(aid.name) == 0)
			// Choose the default device
			deviceId = temporaryRTA.getDefaultInputDevice();
		else if (aid.id == -1)
		{
			// Choose the first input device whose name contains aid.name
			unsigned int deviceCount = temporaryRTA.getDeviceCount();
			for (unsigned int i = 0; i < deviceCount; ++i)
			{
				RtAudio::DeviceInfo di = temporaryRTA.getDeviceInfo(i);
				if (di.inputChannels && di.name.find(aid.name) != std::string::npos)
				{
					deviceId = i;
					break;
				}
			}
		}
		else
			// Choose the device specified by aid.id
			deviceId = aid.id;
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to enumerate audio devices :: %s.\n", error.what());
		return NULL;
	}

	return (VuoAudioIn)VuoAudio_internalPool->getSharedInstance(deviceId);
}

/**
 * Returns the reference-counted object for the specified audio output device.
 */
VuoAudioOut VuoAudioOut_getShared(VuoAudioOutputDevice aod)
{
	int deviceId = -1;

	try
	{
		RtAudio temporaryRTA;	// Just for getting device info prior to opening a shared device.

		if (aod.id == -1 && strlen(aod.name) == 0)
			// Choose the default device
			deviceId = temporaryRTA.getDefaultOutputDevice();
		else if (aod.id == -1)
		{
			// Choose the first output device whose name contains aid.name
			unsigned int deviceCount = temporaryRTA.getDeviceCount();
			for (unsigned int i = 0; i < deviceCount; ++i)
			{
				RtAudio::DeviceInfo di = temporaryRTA.getDeviceInfo(i);
				if (di.outputChannels && di.name.find(aod.name) != std::string::npos)
				{
					deviceId = i;
					break;
				}
			}
		}
		else
			// Choose the device specified by aid.id
			deviceId = aod.id;
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to enumerate audio devices :: %s.\n", error.what());
		return NULL;
	}

	return (VuoAudioOut)VuoAudio_internalPool->getSharedInstance(deviceId);
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
