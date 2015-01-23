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

#include "node.h"
#include "VuoMidiController.h"
#include "VuoMidiDevice.h"
#include "VuoMidiNote.h"
#include "VuoList_VuoMidiDevice.h"

VuoList_VuoMidiDevice VuoMidi_getInputDevices(void);
VuoList_VuoMidiDevice VuoMidi_getOutputDevices(void);

/**
 * Manages sending messages to a MIDI device.
 */
typedef void * VuoMidiOut;

VuoMidiOut VuoMidiOut_make(VuoMidiDevice md);
void VuoMidiOut_sendNote(VuoMidiOut mo, VuoMidiNote note);
void VuoMidiOut_sendController(VuoMidiOut mo, VuoMidiController controller);


/**
 * Manages receiving messages from a MIDI device.
 */
typedef void * VuoMidiIn;

VuoMidiIn VuoMidiIn_make(VuoMidiDevice md);
void VuoMidiIn_enableTriggers
(
		VuoMidiIn mi,
		VuoOutputTrigger(receivedNote, VuoMidiNote),
		VuoOutputTrigger(receivedController, VuoMidiController)
);
void VuoMidiIn_disableTriggers(VuoMidiIn mi);

#ifdef __cplusplus
}
#endif
