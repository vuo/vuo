/**
 * @file
 * VuoMidi implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidi.h"

#include <RtMidi/RtMidi.h>
#include "VuoApp.h"
#include "VuoOsStatus.h"
#include "VuoTriggerSet.hh"

#include <CoreMIDI/CoreMIDI.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMidi",
					 "dependencies" : [
						 "VuoApp",
						 "VuoMidiController",
						 "VuoMidiInputDevice",
						 "VuoMidiOutputDevice",
						 "VuoMidiNote",
						 "VuoMidiPitchBend",
						 "VuoOsStatus",
						 "VuoList_VuoMidiInputDevice",
						 "VuoList_VuoMidiOutputDevice",
						 "rtmidi",
						 "CoreAudio.framework",
						 "CoreMIDI.framework"
					 ]
				 });
#endif
}

static VuoTriggerSet<VuoList_VuoMidiInputDevice>  VuoMidi_inputDeviceCallbacks;   ///< Trigger functions to call when the list of midi input devices changes.
static VuoTriggerSet<VuoList_VuoMidiOutputDevice> VuoMidi_outputDeviceCallbacks;  ///< Trigger functions to call when the list of midi output devices changes.
unsigned int VuoMidi_useCount = 0;  ///< Process-wide count of callers (typically node instances) interested in notifications about midi devices.
MIDIClientRef VuoMidi_client;  ///< Core MIDI client, for device notifications.

/**
 * Returns a list of the available MIDI input devices.
 */
VuoList_VuoMidiInputDevice VuoMidi_getInputDevices(void)
{
	VuoList_VuoMidiInputDevice inputDevices = VuoListCreate_VuoMidiInputDevice();
	try
	{
		RtMidiIn *midiin = new RtMidiIn();
		unsigned int portCount = midiin->getPortCount();
		for (unsigned int i = 0; i < portCount; ++i)
			VuoListAppendValue_VuoMidiInputDevice(inputDevices, VuoMidiInputDevice_make(i, VuoText_make(midiin->getPortName(i).c_str())));
		delete midiin;
	}
	catch(...) {}
	return inputDevices;
}

/**
 * Returns a list of the available MIDI output devices.
 */
VuoList_VuoMidiOutputDevice VuoMidi_getOutputDevices(void)
{
	VuoList_VuoMidiOutputDevice outputDevices = VuoListCreate_VuoMidiOutputDevice();
	try
	{
		RtMidiOut *midiout = new RtMidiOut();
		unsigned int portCount = midiout->getPortCount();
		for (unsigned int i = 0; i < portCount; ++i)
			VuoListAppendValue_VuoMidiOutputDevice(outputDevices, VuoMidiOutputDevice_make(i, VuoText_make(midiout->getPortName(i).c_str())));
		delete midiout;
	}
	catch(...) {}
	return outputDevices;
}

/**
 * Invoked by Core MIDI.
 */
static void VuoMidi_reconfigurationCallback(const MIDINotification *message, void *refCon)
{
	if (message->messageID != kMIDIMsgSetupChanged)
		return;

	VuoMidi_inputDeviceCallbacks.fire(VuoMidi_getInputDevices());
	VuoMidi_outputDeviceCallbacks.fire(VuoMidi_getOutputDevices());
}

/**
 * Indicates that the caller needs to get notifications about MIDI devices.
 *
 * @threadAny
 * @version200New
 */
void VuoMidi_use(void)
{
	if (__sync_add_and_fetch(&VuoMidi_useCount, 1) == 1)
		VuoApp_executeOnMainThread(^{
			OSStatus ret = MIDIClientCreate(CFSTR("VuoMidi_use"), VuoMidi_reconfigurationCallback, NULL, &VuoMidi_client);
			if (ret)
			{
				char *description = VuoOsStatus_getText(ret);
				VUserLog("Error: Couldn't register device change listener: %s", description);
				free(description);
			}
		});
}

/**
 * Indicates that the caller no longer needs notifications about MIDI devices.
 *
 * @threadAny
 * @version200New
 */
void VuoMidi_disuse(void)
{
	if (VuoMidi_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoMidi_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoMidi_useCount, 1) == 0)
		VuoApp_executeOnMainThread(^{
			OSStatus ret = MIDIClientDispose(VuoMidi_client);
			if (ret)
			{
				char *description = VuoOsStatus_getText(ret);
				VUserLog("Error: Couldn't unregister device change listener: %s", description);
				free(description);
			}
		});
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known MIDI devices changes.
 *
 * Call `VuoMidi_use()` before calling this.
 *
 * @threadAny
 * @version200New
 */
void VuoMidi_addDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice))
{
	VuoMidi_inputDeviceCallbacks.addTrigger(inputDevices);
	VuoMidi_outputDeviceCallbacks.addTrigger(outputDevices);
	inputDevices(VuoMidi_getInputDevices());
	outputDevices(VuoMidi_getOutputDevices());
}

/**
 * Removes a trigger callback previously added by @ref VuoMidi_addDevicesChangedTriggers.
 *
 * @threadAny
 * @version200New
 */
void VuoMidi_removeDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice))
{
	VuoMidi_inputDeviceCallbacks.removeTrigger(inputDevices);
	VuoMidi_outputDeviceCallbacks.removeTrigger(outputDevices);
}

void VuoMidiOut_destroy(VuoMidiOut mo);

/**
 * Creates a reference-counted object to manage sending messages to a MIDI device.
 */
VuoMidiOut VuoMidiOut_make(VuoMidiOutputDevice md)
{
	RtMidiOut *midiout = NULL;
	try
	{
		VuoMidiOutputDevice realizedDevice;
		if (!VuoMidiOutputDevice_realize(md, &realizedDevice))
			throw RtError("No matching device found");
		VuoMidiOutputDevice_retain(realizedDevice);

		midiout = new RtMidiOut();
		midiout->openPort(realizedDevice.id, "VuoMidiOut_make");

		VuoMidiOutputDevice_release(realizedDevice);
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to open the specified MIDI device (%s) :: %s.", VuoMidiOutputDevice_getSummary(md), error.what());
		delete midiout;
		return NULL;
	}

	VuoRegister(midiout, VuoMidiOut_destroy);
	return (VuoMidiOut)midiout;
}

/**
 * Outputs the specified @c note event through the specified MIDI output device @c mo.
 */
void VuoMidiOut_sendNote(VuoMidiOut mo, VuoMidiNote note)
{
	if (!mo)
		return;

	RtMidiOut *midiout = (RtMidiOut *)mo;
	std::vector<unsigned char> message;
	message.push_back(0x80 + (note.isNoteOn ? 0x10 : 0) + MIN(note.channel-1,15));
	message.push_back(MIN(note.noteNumber,127));
	message.push_back(MIN(note.velocity,127));
	midiout->sendMessage(&message);
}

/**
 * Outputs the specified @c controller event through the specified MIDI output device @c mo.
 */
void VuoMidiOut_sendController(VuoMidiOut mo, VuoMidiController controller)
{
	if (!mo)
		return;

	RtMidiOut *midiout = (RtMidiOut *)mo;
	std::vector<unsigned char> message;
	message.push_back(0xB0 + MIN(controller.channel-1,15));
	message.push_back(MIN(controller.controllerNumber,127));
	message.push_back(MIN(controller.value,127));
	midiout->sendMessage(&message);
}

/**
 * Outputs the specified `pitchBend` event through the specified MIDI output device `mo`.
 */
void VuoMidiOut_sendPitchBend(VuoMidiOut mo, VuoMidiPitchBend pitchBend)
{
	if (!mo)
		return;

	RtMidiOut *midiout = (RtMidiOut *)mo;
	std::vector<unsigned char> message;
	message.push_back(0xE0 + MIN(pitchBend.channel-1,15));
	message.push_back(pitchBend.value & 0x7f);
	message.push_back((pitchBend.value >> 7) & 0x7f);
	midiout->sendMessage(&message);
}

/**
 * Destroys a MIDI output device manager.
 */
void VuoMidiOut_destroy(VuoMidiOut mo)
{
	if (!mo)
		return;

	RtMidiOut *midiout = (RtMidiOut *)mo;
	delete midiout;
}

/**
 * Private data for a VuoMidiIn instance.
 */
struct VuoMidiIn_internal
{
	RtMidiIn *midiin;	///< RtMidi's device pointer.

	dispatch_queue_t callbackQueue;	///< Serializes access to the following callback functions.
	void (*receivedNote)(void *, VuoMidiNote);	///< Called when a note is received.
	void (*receivedController)(void *, VuoMidiController);	///< Called when a control change is received.
	void (*receivedPitchBend)(void *, VuoMidiPitchBend);	///< Called when a pitch bend change is received.
	void *context;	///< Caller's context data, to be passed to the callbacks.
};

/**
 * RtMidi calls this function ("on a separate high-priority thread owned by CoreMIDI") every time it receives a MIDI event.
 */
void VuoMidiIn_receivedEvent(double timeStamp, std::vector< unsigned char > *message, void *userData)
{
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)userData;
	unsigned char channel = ((*message)[0] & 0x0f) + 1;
	if (((*message)[0] & 0xf0) == 0x80) // Note Off
	{
		VuoMidiNote mn = VuoMidiNote_make(channel, false, (*message)[2], (*message)[1]);
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedNote)
								mii->receivedNote(mii->context, mn);
					   });
	}
	else if (((*message)[0] & 0xf0) == 0x90) // Note On
	{
		unsigned char velocity = (*message)[2];

		// Convert note-on messages with 0 velocity into note-off messages.
		bool isNoteOn = (velocity != 0);

		VuoMidiNote mn = VuoMidiNote_make(channel, isNoteOn, velocity, (*message)[1]);
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedNote)
								mii->receivedNote(mii->context, mn);
					   });
	}
	else if (((*message)[0] & 0xf0) == 0xb0) // Control Change
	{
		VuoMidiController mc = VuoMidiController_make(channel, (*message)[1], (*message)[2]);
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedController)
								mii->receivedController(mii->context, mc);
					   });
	}
	else if (((*message)[0] & 0xf0) == 0xe0) // Pitch Bend Change
	{
		VuoMidiPitchBend mp = VuoMidiPitchBend_make(channel, (*message)[1] + ((*message)[2] << 7));
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedPitchBend)
							   mii->receivedPitchBend(mii->context, mp);
					   });
	}
	else
	{
		const char *hex = "0123456789abcdef";
		std::string messageHex;
		for (size_t i = 0; i < message->size(); ++i)
		{
			messageHex.push_back(hex[(*message)[i] >> 4]);
			messageHex.push_back(hex[(*message)[i] & 0x0f]);
		}
		VUserLog("Warning: Received unknown message: 0x%s", messageHex.c_str());
	}
}

void VuoMidiIn_destroy(VuoMidiIn mi);

/**
 * Creates a reference-counted object to manage receiving messages from a MIDI device.
 */
VuoMidiIn VuoMidiIn_make(VuoMidiInputDevice md)
{
	struct VuoMidiIn_internal *mii;
	RtMidiIn *midiin = NULL;
	try
	{
		VuoMidiInputDevice realizedDevice;
		if (!VuoMidiInputDevice_realize(md, &realizedDevice))
			throw RtError("No matching device found");
		VuoMidiInputDevice_retain(realizedDevice);

		midiin = new RtMidiIn();
		midiin->openPort(realizedDevice.id, "VuoMidiIn_make");

		VuoMidiInputDevice_release(realizedDevice);

		mii = (struct VuoMidiIn_internal *)calloc(1, sizeof(struct VuoMidiIn_internal));
		VuoRegister(mii, VuoMidiIn_destroy);
		mii->midiin = midiin;
		mii->callbackQueue = dispatch_queue_create("vuo.midi.receive", NULL);
		// Trigger functions (receiveNote, ...) are NULLed by calloc.

		midiin->setCallback(&VuoMidiIn_receivedEvent, mii);

		// Ignore SysEx, timing, and active sensing for now.
		midiin->ignoreTypes(true,true,true);
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Error: Failed to open the specified MIDI device (%s) :: %s.", VuoMidiInputDevice_getSummary(md), error.what());
		delete midiin;
		return NULL;
	}

	return (VuoMidiIn)mii;
}

/**
 * Sets up the MIDI input device to call the trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoMidiIn_enableTriggers
(
		VuoMidiIn mi,
		void (*receivedNote)(void *context, VuoMidiNote note),
		void (*receivedController)(void *context, VuoMidiController controller),
		void (*receivedPitchBend)(void *context, VuoMidiPitchBend pitchBend),
		void *context
)
{
	if (!mi)
		return;
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)mi;
	dispatch_async(mii->callbackQueue, ^{
					   mii->receivedNote = receivedNote;
					   mii->receivedController = receivedController;
					   mii->receivedPitchBend = receivedPitchBend;
					   mii->context = context;
				   });
}

/**
 * Stops the MIDI input device from calling trigger functions when it receives an event.
 *
 * @threadAny
 */
void VuoMidiIn_disableTriggers(VuoMidiIn mi)
{
	if (!mi)
		return;
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)mi;
	dispatch_sync(mii->callbackQueue, ^{
					   mii->receivedNote = NULL;
					   mii->receivedController = NULL;
					   mii->context = NULL;
				   });
}

/**
 * Destroys a MIDI input device manager.
 */
void VuoMidiIn_destroy(VuoMidiIn mi)
{
	if (!mi)
		return;
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)mi;
	dispatch_release(mii->callbackQueue);
	delete mii->midiin;
	free(mii);
}

/// Helper for VuoMidiInputDevice_realize.
#define setRealizedDevice(newDevice) \
	realizedDevice->id = newDevice.id; \
	realizedDevice->name = VuoText_make(newDevice.name);

/**
 * If `device`'s ID or name is unknown:
 *
 *    - If a matching device is present, sets `realizedDevice` to that device, and returns true.
 *    - If no matching device is present, returns false, leaving `realizedDevice` unset.
 *
 * If `device`'s ID and name are already known (presumably from the `List MIDI Devices` node),
 * sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the device is currently present.)
 *
 * @threadAny
 */
bool VuoMidiInputDevice_realize(VuoMidiInputDevice device, VuoMidiInputDevice *realizedDevice)
{
	// Already have ID and name; nothing to do.
	if (device.id != -1 && !VuoText_isEmpty(device.name))
	{
		setRealizedDevice(device);
		return true;
	}

	// Otherwise, try to find a matching device.

	VDebugLog("Requested device:   %s", json_object_to_json_string(VuoMidiInputDevice_getJson(device)));
	VuoList_VuoMidiInputDevice devices = VuoMidi_getInputDevices();
	VuoLocal(devices);
	__block bool found = false;

	// First pass: try to find an exact match by ID.
	VuoListForeach_VuoMidiInputDevice(devices, ^(const VuoMidiInputDevice item){
		if (device.id != -1 && device.id == item.id)
		{
			VDebugLog("Matched by ID:      %s",json_object_to_json_string(VuoMidiInputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
			return false;
		}
		return true;
	});

	// Second pass: try to find a match by name.
	if (!found)
		VuoListForeach_VuoMidiInputDevice(devices, ^(const VuoMidiInputDevice item){
			if (!VuoText_isEmpty(device.name) && VuoText_compare(item.name, (VuoTextComparison){VuoTextComparison_Contains, true, ""}, device.name))
			{
				VDebugLog("Matched by name:    %s",json_object_to_json_string(VuoMidiInputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Third pass: if the user hasn't specified a device, use the first device.
	if (!found && device.id == -1 && VuoText_isEmpty(device.name))
	{
		if (VuoListGetCount_VuoMidiInputDevice(devices))
		{
			VuoMidiInputDevice item = VuoListGetValue_VuoMidiInputDevice(devices, 1);
			VDebugLog("Using first device: %s",json_object_to_json_string(VuoMidiInputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
		}
	}

	if (!found)
		VDebugLog("No matching device found.");

	return found;
}

/**
 * If `device`'s ID or name unknown:
 *
 *    - If a matching device is present, sets `realizedDevice` to that device, and returns true.
 *    - If no matching device is present, returns false, leaving `realizedDevice` unset.
 *
 * If `device`'s ID and name are already known (presumably from the `List MIDI Devices` node),
 * sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the device is currently present.)
 *
 * @threadAny
 */
bool VuoMidiOutputDevice_realize(VuoMidiOutputDevice device, VuoMidiOutputDevice *realizedDevice)
{
	// Already have ID and name; nothing to do.
	if (device.id != -1 && !VuoText_isEmpty(device.name))
	{
		setRealizedDevice(device);
		return true;
	}

	// Otherwise, try to find a matching device.

	VDebugLog("Requested device:   %s", json_object_to_json_string(VuoMidiOutputDevice_getJson(device)));
	VuoList_VuoMidiOutputDevice devices = VuoMidi_getOutputDevices();
	VuoLocal(devices);
	__block bool found = false;

	// First pass: try to find an exact match by ID.
	VuoListForeach_VuoMidiOutputDevice(devices, ^(const VuoMidiOutputDevice item){
		if (device.id != -1 && device.id == item.id)
		{
			VDebugLog("Matched by ID:      %s",json_object_to_json_string(VuoMidiOutputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
			return false;
		}
		return true;
	});

	// Second pass: try to find a match by name.
	if (!found)
		VuoListForeach_VuoMidiOutputDevice(devices, ^(const VuoMidiOutputDevice item){
			if (!VuoText_isEmpty(device.name) && VuoText_compare(item.name, (VuoTextComparison){VuoTextComparison_Contains, true, ""}, device.name))
			{
				VDebugLog("Matched by name:    %s",json_object_to_json_string(VuoMidiOutputDevice_getJson(item)));
				setRealizedDevice(item);
				found = true;
				return false;
			}
			return true;
		});

	// Third pass: if the user hasn't specified a device, use the first device.
	if (!found && device.id == -1 && VuoText_isEmpty(device.name))
	{
		if (VuoListGetCount_VuoMidiOutputDevice(devices))
		{
			VuoMidiOutputDevice item = VuoListGetValue_VuoMidiOutputDevice(devices, 1);
			VDebugLog("Using first device: %s",json_object_to_json_string(VuoMidiOutputDevice_getJson(item)));
			setRealizedDevice(item);
			found = true;
		}
	}

	if (!found)
		VDebugLog("No matching device found.");

	return found;
}
