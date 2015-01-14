/**
 * @file
 * VuoMidi implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
VuoList_VuoMidiDevice VuoMidi_getInputDevices(void)
{
	RtMidiIn *midiin = new RtMidiIn();
	unsigned int portCount = midiin->getPortCount();
	VuoList_VuoMidiDevice inputDevices = VuoListCreate_VuoMidiDevice();
	for (unsigned int i = 0; i < portCount; ++i)
		VuoListAppendValue_VuoMidiDevice(inputDevices, VuoMidiDevice_make(i, VuoText_make(midiin->getPortName(i).c_str()), true));
	delete midiin;
	return inputDevices;
}

/**
 * Returns a list of the available MIDI output devices.
 */
VuoList_VuoMidiDevice VuoMidi_getOutputDevices(void)
{
	RtMidiOut *midiout = new RtMidiOut();
	unsigned int portCount = midiout->getPortCount();
	VuoList_VuoMidiDevice outputDevices = VuoListCreate_VuoMidiDevice();
	for (unsigned int i = 0; i < portCount; ++i)
		VuoListAppendValue_VuoMidiDevice(outputDevices, VuoMidiDevice_make(i, VuoText_make(midiout->getPortName(i).c_str()), false));
	delete midiout;
	return outputDevices;
}

void VuoMidiOut_destroy(VuoMidiOut mo);

/**
 * Creates a reference-counted object to manage sending messages to a MIDI device.
 */
VuoMidiOut VuoMidiOut_make(VuoMidiDevice md)
{
	if (md.isInput)
	{
		/// @todo https://b33p.net/kosada/node/4724
		fprintf(stderr, "VuoMidi: The specified MIDI device (%s) isn't an output device.\n", VuoMidiDevice_summaryFromValue(md));
		return NULL;
	}

	const char *vuoMIDIID = "VuoMidiOut_make";

	RtMidiOut *midiout = new RtMidiOut();
	VuoRegister(midiout, VuoMidiOut_destroy);
	if (!midiout->getPortCount())
	{
		/// @todo Temporary workaround until https://b33p.net/kosada/node/6307 is fixed.
		/// @todo https://b33p.net/kosada/node/4724
		fprintf(stderr, "VuoMidi: No MIDI output devices available.\n");
		delete midiout;
		return NULL;
	}
	try
	{
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
		fprintf(stderr, "VuoMidi: Failed to open the specified MIDI device (%s) :: %s.\n", VuoMidiDevice_summaryFromValue(md), error.what());
		delete midiout;
		return NULL;
	}

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
	void (*receivedNote)(VuoMidiNote);	///< This node instance's trigger function.
	void (*receivedController)(VuoMidiController);	///< This node instance's trigger function.
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
								mii->receivedNote(mn);
					   });
	}
	else if (((*message)[0] & 0xf0) == 0x90) // Note On
	{
		VuoMidiNote mn = VuoMidiNote_make(channel, true, (*message)[2], (*message)[1]);
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedNote)
								mii->receivedNote(mn);
					   });
	}
	else if (((*message)[0] & 0xf0) == 0xb0) // Control Change
	{
		VuoMidiController mc = VuoMidiController_make(channel, (*message)[1], (*message)[2]);
		dispatch_async(mii->callbackQueue, ^{
						   if (mii->receivedController)
								mii->receivedController(mc);
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
		fprintf(stderr, "VuoMidi: Received unknown message: 0x%s\n", messageHex.c_str());
	}
}

void VuoMidiIn_destroy(VuoMidiIn mi);

/**
 * Creates a reference-counted object to manage receiving messages from a MIDI device.
 */
VuoMidiIn VuoMidiIn_make(VuoMidiDevice md)
{
	if (!md.isInput)
	{
		/// @todo https://b33p.net/kosada/node/4724
		fprintf(stderr, "VuoMidi: The specified MIDI device (%s) isn't an input device.\n", VuoMidiDevice_summaryFromValue(md));
		return NULL;
	}

	const char *vuoMIDIID = "VuoMidiIn_make";

	struct VuoMidiIn_internal *mii;
	RtMidiIn *midiin = new RtMidiIn();
	if (!midiin->getPortCount())
	{
		/// @todo Temporary workaround until https://b33p.net/kosada/node/6307 is fixed.
		/// @todo https://b33p.net/kosada/node/4724
		fprintf(stderr, "VuoMidi: No MIDI input devices available.\n");
		delete midiin;
		return NULL;
	}
	try
	{
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
		fprintf(stderr, "VuoMidi: Failed to open the specified MIDI device (%s) :: %s.\n", VuoMidiDevice_summaryFromValue(md), error.what());
		delete midiin;
		return NULL;
	}

	return (VuoMidiIn)mii;
}

/**
 * Sets up the MIDI input device to call the trigger functions when it receives an event.
 *
 * Can be called from any thread.
 */
void VuoMidiIn_enableTriggers
(
		VuoMidiIn mi,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController)
)
{
	if (!mi)
		return;
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)mi;
	dispatch_async(mii->callbackQueue, ^{
					   mii->receivedNote = receivedNote;
					   mii->receivedController = receivedController;
				   });
}

/**
 * Stops the MIDI input device from calling trigger functions when it receives an event.
 *
 * Can be called from any thread.
 */
void VuoMidiIn_disableTriggers(VuoMidiIn mi)
{
	if (!mi)
		return;
	struct VuoMidiIn_internal *mii = (struct VuoMidiIn_internal *)mi;
	dispatch_sync(mii->callbackQueue, ^{
					   mii->receivedNote = NULL;
					   mii->receivedController = NULL;
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
	delete mii->midiin;
	free(mii);
}
