/**
 * @file
 * VuoSerial interface.
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

#include "VuoBaudRate.h"
#include "VuoData.h"
#include "VuoParity.h"
#include "VuoSerialDevice.h"
#include "VuoList_VuoSerialDevice.h"

VuoList_VuoSerialDevice VuoSerial_getDeviceList(void);

void VuoSerial_use(void);
void VuoSerial_disuse(void);
void VuoSerial_addDevicesChangedTriggers   (VuoOutputTrigger(devices, VuoList_VuoSerialDevice));
void VuoSerial_removeDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoSerialDevice));


/**
 * Manages sending and receiving serial data.
 */
typedef void *VuoSerial;

VuoSerial VuoSerial_useShared(const VuoText devicePath);
void VuoSerial_disuseShared(VuoSerial device);

void VuoSerial_checkPendingDevices(void);

void VuoSerial_configureDevice(VuoSerial device, VuoInteger baudRate, VuoInteger dataBits, VuoParity parity, VuoInteger stopBits);

void VuoSerial_addReceiveTrigger   (VuoSerial device, VuoOutputTrigger(receivedData, VuoData));
void VuoSerial_removeReceiveTrigger(VuoSerial device, VuoOutputTrigger(receivedData, VuoData));

void VuoSerial_sendData(VuoSerial device, const VuoData data);

#ifdef __cplusplus
}
#endif
