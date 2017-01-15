/**
 * @file
 * VuoAudio interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoAudioSamples.h"
#include "VuoAudioInputDevice.h"
#include "VuoAudioOutputDevice.h"
#include "VuoList_VuoAudioSamples.h"
#include "VuoList_VuoAudioInputDevice.h"
#include "VuoList_VuoAudioOutputDevice.h"

VuoList_VuoAudioInputDevice VuoAudio_getInputDevices(void);
VuoList_VuoAudioOutputDevice VuoAudio_getOutputDevices(void);

/**
 * Manages sending audio output.
 */
typedef void * VuoAudioOut;

VuoAudioOut VuoAudioOut_getShared(VuoAudioOutputDevice aod);
void VuoAudioOut_sendChannels(VuoAudioOut ao, VuoList_VuoAudioSamples channels, void *id);
void VuoAudioOut_addTrigger
(
		VuoAudioOut ao,
		VuoOutputTrigger(requestedChannels, VuoReal)
);
void VuoAudioOut_removeTrigger
(
		VuoAudioOut ao,
		VuoOutputTrigger(requestedChannels, VuoReal)
);


/**
 * Manages receiving live audio input.
 */
typedef void *VuoAudioIn;

VuoAudioIn VuoAudioIn_getShared(VuoAudioInputDevice aid);
void VuoAudioIn_addTrigger
(
		VuoAudioIn ai,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
);
void VuoAudioIn_removeTrigger
(
		VuoAudioIn ai,
		VuoOutputTrigger(receivedChannels, VuoList_VuoAudioSamples)
);

#ifdef __cplusplus
}
#endif
