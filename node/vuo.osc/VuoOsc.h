/**
 * @file
 * VuoOsc interface.
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

#include "VuoOscMessage.h"
#include "VuoOscInputDevice.h"
#include "VuoOscOutputDevice.h"
#include "VuoList_VuoOscMessage.h"
#include "VuoList_VuoOscInputDevice.h"
#include "VuoList_VuoOscOutputDevice.h"

void VuoOsc_use(void);
void VuoOsc_disuse(void);

VuoList_VuoOscInputDevice  VuoOsc_getInputDeviceList(void);
VuoList_VuoOscOutputDevice VuoOsc_getOutputDeviceList(void);

bool VuoOscInputDevice_realize (const VuoOscInputDevice  device, VuoOscInputDevice  *realizedDevice);
bool VuoOscOutputDevice_realize(const VuoOscOutputDevice device, VuoOscOutputDevice *realizedDevice);

void VuoOsc_addDevicesChangedTriggers
(
		VuoOutputTrigger(inputDevices,  VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
);
void VuoOsc_removeDevicesChangedTriggers
(
		VuoOutputTrigger(inputDevices,  VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
);


/**
 * Manages receiving messages via OSC.
 */
typedef void * VuoOscIn;

VuoOscIn VuoOscIn_make(const VuoOscInputDevice device);
void VuoOscIn_enableTriggers
(
		VuoOscIn oi,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
);
void VuoOscIn_disableTriggers(VuoOscIn oi);


/**
 * Manages sending OSC output.
 */
typedef void *VuoOscOut;

VuoOscOut VuoOscOut_useShared(const VuoOscOutputDevice device);
void VuoOscOut_disuseShared(VuoOscOut oo);

void VuoOscOut_sendMessages(VuoOscOut oo, VuoList_VuoOscMessage messages);

#ifdef __cplusplus
}
#endif
