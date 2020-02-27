/**
 * @file
 * VuoAudio interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoAudioSamples.h"
#include "VuoAudioInputDevice.h"
#include "VuoAudioOutputDevice.h"
#include "VuoList_VuoAudioSamples.h"
#include "VuoList_VuoAudioInputDevice.h"
#include "VuoList_VuoAudioOutputDevice.h"

void VuoAudio_use(void);
void VuoAudio_disuse(void);
void VuoAudio_addDevicesChangedTriggers   (VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice));
void VuoAudio_removeDevicesChangedTriggers(VuoOutputTrigger(inputDevices, VuoList_VuoAudioInputDevice), VuoOutputTrigger(outputDevices, VuoList_VuoAudioOutputDevice));
VuoList_VuoAudioInputDevice VuoAudio_getInputDevices(void);
VuoList_VuoAudioOutputDevice VuoAudio_getOutputDevices(void);

bool VuoAudioInputDevice_realize (VuoAudioInputDevice device,  VuoAudioInputDevice  *realizedDevice) VuoWarnUnusedResult;
bool VuoAudioOutputDevice_realize(VuoAudioOutputDevice device, VuoAudioOutputDevice *realizedDevice) VuoWarnUnusedResult;

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
