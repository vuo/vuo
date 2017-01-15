/**
 * @file
 * VuoMidi implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoMidi.h"

#include "RtMidi.h"
#include <dispatch/dispatch.h>


extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoMidi",
					 "dependencies" : [
						 "VuoMidiController",
						 "VuoMidiInputDevice",
						 "VuoMidiOutputDevice",
						 "VuoMidiNote",
						 "VuoMidiPitchBend",
						 "VuoList_VuoMidiInputDevice",
						 "VuoList_VuoMidiOutputDevice",
						 "RtMidi",
						 "CoreAudio.framework",
						 "CoreMIDI.framework"
					 ]
				 });
#endif
}


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

void VuoMidiOut_destroy(VuoMidiOut mo);

/**
 * Creates a reference-counted object to manage sending messages to a MIDI device.
 */
VuoMidiOut VuoMidiOut_make(VuoMidiOutputDevice md)
{
	const char *vuoMIDIID = "VuoMidiOut_make";

	RtMidiOut *midiout = NULL;
	try
	{
		midiout = new RtMidiOut();

		if (md.id == -1 && strlen(md.name) == 0)
			// Open the first MIDI device
			midiout->openPort(0, vuoMIDIID);
		else if (md.id == -1)
		{
			// Open the first MIDI %s device whose name contains mn.name
			unsigned int portCount = midiout->getPortCount();
			for (unsigned int i = 0; i < portCount; ++i)
				if (midiout->getPortName(i).find(md.name) != std::string::npos)
				{
					midiout->openPort(i, vuoMIDIID);
					break;
				}
		}
		else
		{
			// Open the MIDI device specified by mn.id
			midiout->openPort(md.id, vuoMIDIID);
		}
	}
	catch (RtError &error)
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Failed to open the specified MIDI device (%s) :: %s.", VuoMidiOutputDevice_getSummary(md), error.what());
		if (midiout)
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
	const char *vuoMIDIID = "VuoMidiIn_make";

	struct VuoMidiIn_internal *mii;
	RtMidiIn *midiin = NULL;
	try
	{
		midiin = new RtMidiIn();

		if (md.id == -1 && strlen(md.name) == 0)
			// Open the first MIDI device
			midiin->openPort(0, vuoMIDIID);
		else if (md.id == -1)
		{
			// Open the first MIDI %s device whose name contains mn.name
			unsigned int portCount = midiin->getPortCount();
			for (unsigned int i = 0; i < portCount; ++i)
				if (midiin->getPortName(i).find(md.name) != std::string::npos)
				{
					midiin->openPort(i, vuoMIDIID);
					break;
				}
		}
		else
		{
			// Open the MIDI device specified by mn.id
			midiin->openPort(md.id, vuoMIDIID);
		}

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
		if (midiin)
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
