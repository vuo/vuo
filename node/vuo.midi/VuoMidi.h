/**
 * @file
 * VuoMidi interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoMidiController.h"
#include "VuoMidiInputDevice.h"
#include "VuoMidiOutputDevice.h"
#include "VuoMidiNote.h"
#include "VuoList_VuoMidiInputDevice.h"
#include "VuoList_VuoMidiOutputDevice.h"

VuoList_VuoMidiInputDevice VuoMidi_getInputDevices(void);
VuoList_VuoMidiOutputDevice VuoMidi_getOutputDevices(void);

/**
 * Manages sending messages to a MIDI device.
 */
typedef void * VuoMidiOut;

VuoMidiOut VuoMidiOut_make(VuoMidiOutputDevice md);
void VuoMidiOut_sendNote(VuoMidiOut mo, VuoMidiNote note);
void VuoMidiOut_sendController(VuoMidiOut mo, VuoMidiController controller);


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
		void *context
);
void VuoMidiIn_disableTriggers(VuoMidiIn mi);

#ifdef __cplusplus
}
#endif
