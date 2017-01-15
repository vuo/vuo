/**
 * @file
 * VuoHid interface.
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
#include "VuoHidDevice.h"
#include "VuoList_VuoHidDevice.h"
#include "VuoHidControl.h"

char *VuoHid_getUsageText(uint32_t usagePage, uint32_t usage);

VuoList_VuoHidDevice VuoHid_getDeviceList(void);

VuoInteger VuoHid_getLocation(void *device);
bool VuoHid_isElementValid(void *element);
VuoHidControl VuoHid_getControlForElement(void *element);

void VuoHid_use(void);
void VuoHid_disuse(void);
void VuoHid_addDevicesChangedTriggers   (VuoOutputTrigger(devices, VuoList_VuoHidDevice));
void VuoHid_removeDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoHidDevice));


/**
 * Manages receiving HID control data.
 */
typedef void *VuoHid;

VuoHid VuoHid_make(const VuoHidDevice device, const VuoBoolean exclusive);
void VuoHid_checkPendingDevices(void);

void VuoHid_addReceiveTrigger   (VuoHid device, VuoOutputTrigger(receivedControl, VuoHidControl));
void VuoHid_removeReceiveTrigger(VuoHid device, VuoOutputTrigger(receivedControl, VuoHidControl));

#ifdef __cplusplus
}
#endif
