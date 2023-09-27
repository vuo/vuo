/**
 * @file
 * VuoMidi interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoMidiController.h"
#include "VuoMidiInputDevice.h"
#include "VuoMidiOutputDevice.h"
#include "VuoMidiNote.h"
#include "VuoMidiPitchBend.h"
#include "VuoList_VuoMidiInputDevice.h"
#include "VuoList_VuoMidiOutputDevice.h"

void VuoMidi_use(void);
void VuoMidi_disuse(void);
void VuoMidi_addDevicesChangedTriggers   (VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice));
void VuoMidi_removeDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoMidiInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoMidiOutputDevice));
VuoList_VuoMidiInputDevice VuoMidi_getInputDevices(void);
VuoList_VuoMidiOutputDevice VuoMidi_getOutputDevices(void);

bool VuoMidiInputDevice_realize (VuoMidiInputDevice device,  VuoMidiInputDevice *realizedDevice);
bool VuoMidiOutputDevice_realize(VuoMidiOutputDevice device, VuoMidiOutputDevice *realizedDevice);

/**
 * Manages sending messages to a MIDI device.
 */
typedef void * VuoMidiOut;

VuoMidiOut VuoMidiOut_make(VuoMidiOutputDevice md);
void VuoMidiOut_sendNote(VuoMidiOut mo, VuoMidiNote note);
void VuoMidiOut_sendController(VuoMidiOut mo, VuoMidiController controller);
void VuoMidiOut_sendPitchBend(VuoMidiOut mo, VuoMidiPitchBend pitchBend);


/**
 * Manages receiving messages from a MIDI device.
 */
typedef void * VuoMidiIn;

VuoMidiIn VuoMidiIn_make(VuoMidiInputDevice md);
void VuoMidiIn_enableTriggers
(
		VuoMidiIn mi,
		void (*receivedNote)(void *context, VuoMidiNote note),
		void (*receivedController)(void *context, VuoMidiController controller),
		void (*receivedPitchBend)(void *context, VuoMidiPitchBend pitchBend),
		void *context
);
void VuoMidiIn_disableTriggers(VuoMidiIn mi);

#ifdef __cplusplus
}
#endif
