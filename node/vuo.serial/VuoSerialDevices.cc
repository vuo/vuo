/**
 * @file
 * VuoSerial implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSerial.h"
#include "VuoPool.hh"
#include "VuoTriggerSet.hh"

#include <dispatch/dispatch.h>
#include <vector>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/usb/USBSpec.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSerialDevices",
					 "dependencies" : [
						 "CoreFoundation.framework",
						 "IOKit.framework",
						 "VuoSerialDevice",
						 "VuoSerialDevices",
						 "VuoSerialIO",
						 "VuoList_VuoSerialDevice",
						 "VuoList_VuoInteger"
					 ]
				 });
#endif
}

static VuoTriggerSet<VuoList_VuoSerialDevice> VuoSerial_deviceCallbacks;		///< Trigger functions to call when the list of devices changes.

/**
 * Searches `o` and its parents recursively for `property`, assumed to be a string.
 */
static VuoText VuoSerial_getPropertyFromObjectOrAncestry(io_object_t o, CFStringRef property)
{
	CFStringRef valueCF = (CFStringRef)IORegistryEntryCreateCFProperty(o, property, NULL, 0);
	if (valueCF)
	{
		VuoText value = VuoText_makeFromCFString(valueCF);
		CFRelease(valueCF);
		return value;
	}

	io_iterator_t it;
	kern_return_t ret = IORegistryEntryGetParentIterator(o, kIOServicePlane, &it);
	if (ret != KERN_SUCCESS)
	{
		VUserLog("Error: Couldn't get parent iterator: %d", ret);
		return NULL;
	}

	io_object_t po;
	while ( (po = IOIteratorNext(it)) )
	{
		VuoText value = VuoSerial_getPropertyFromObjectOrAncestry(po, property);
		IOObjectRelease(po);
		if (value)
		{
			IOObjectRelease(it);
			return value;
		}
	}
	IOObjectRelease(it);

	return NULL;
}


/**
 * Returns a list of the currently-avaialble serial devices.
 */
VuoList_VuoSerialDevice VuoSerial_getDeviceList(void)
{
	CFMutableDictionaryRef match = IOServiceMatching(kIOSerialBSDServiceValue);
	if (!match)
	{
		VUserLog("Error: Couldn't create serial matching dictionary.");
		return NULL;
	}
	CFDictionarySetValue(match, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

	io_iterator_t it;
	kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, match, &it);
	if (ret != KERN_SUCCESS)
	{
		VUserLog("Error: Couldn't get serial device list: %d", ret);
		return NULL;
	}

	VuoList_VuoSerialDevice devices = VuoListCreate_VuoSerialDevice();
	io_object_t o;
	while ( (o = IOIteratorNext(it)) )
	{
		VuoSerialDevice device;
		device.matchType = VuoSerialDevice_MatchPath;

		device.path = VuoSerial_getPropertyFromObjectOrAncestry(o, CFSTR(kIOCalloutDeviceKey));
		if (!device.path)
		{
			VUserLog("Error: Device %d has no path.", o);
			continue;
		}

		// Omit devices we can't open.
		if (strstr(device.path, "-Wireless") // iPhone?
		 || strstr(device.path, "Bluetooth-"))
		{
			VuoRetain(device.path);
			VuoRelease(device.path);
			continue;
		}

		VuoText vendorName   = VuoSerial_getPropertyFromObjectOrAncestry(o, CFSTR(kUSBVendorString));
		VuoText productName  = VuoSerial_getPropertyFromObjectOrAncestry(o, CFSTR(kUSBProductString));
		VuoText serialNumber = VuoSerial_getPropertyFromObjectOrAncestry(o, CFSTR(kUSBSerialNumberString));

		VuoRetain(vendorName);
		VuoRetain(productName);
		VuoRetain(serialNumber);

		if (vendorName && productName && serialNumber)
			device.name = VuoText_make(VuoText_format("%s %s (%s)", vendorName, productName, serialNumber));
		else if (vendorName && productName)
			device.name = VuoText_make(VuoText_format("%s %s", vendorName, productName));
		else if (vendorName && serialNumber)
			device.name = VuoText_make(VuoText_format("%s (%s)", vendorName, serialNumber));
		else if (productName && serialNumber)
			device.name = VuoText_make(VuoText_format("%s (%s)", productName, serialNumber));
		else if (vendorName)
			device.name = VuoText_make(vendorName);
		else if (productName)
			device.name = VuoText_make(productName);
		else
		{
			VuoText ttyDevice    = VuoSerial_getPropertyFromObjectOrAncestry(o, CFSTR(kIOTTYDeviceKey));
			if (ttyDevice)
				device.name = ttyDevice;
			else
				device.name = VuoText_make(device.path);
		}


		VuoRelease(vendorName);
		VuoRelease(productName);
		VuoRelease(serialNumber);
		IOObjectRelease(o);

		VuoListAppendValue_VuoSerialDevice(devices, device);
	}

	return devices;
}

unsigned int VuoSerial_useCount = 0;	///< Process-wide count of callers (typically node instances) interested in notifications about serial devices.
IONotificationPortRef VuoSerial_notificationPort;	///< Manages notifications about seiral devices.

/**
 * Invoked by the IONotification system when a serial port is added or removed.
 */
static void VuoSerial_servicesChanged(void *refcon, io_iterator_t it)
{
	// The notification won't fire again until we've looked at all of the existing items once.
	while (IOIteratorNext(it));

	VuoSerial_deviceCallbacks.fire(VuoSerial_getDeviceList());

	VuoSerial_checkPendingDevices();
}

/**
 * Indicates that the caller needs to get notifications about serial devices.
 *
 * @threadAny
 */
void VuoSerial_use(void)
{
	if (__sync_add_and_fetch(&VuoSerial_useCount, 1) == 1)
	{
		VuoSerial_notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
		if (!VuoSerial_notificationPort)
		{
			VUserLog("Error: Couldn't create serial notification port.");
			return;
		}

		CFRunLoopSourceRef notificationSource = IONotificationPortGetRunLoopSource(VuoSerial_notificationPort);
		if (!notificationSource)
		{
			VUserLog("Error: Couldn't get serial notification source.");
			return;
		}

		CFRunLoopAddSource(CFRunLoopGetMain(), notificationSource, kCFRunLoopCommonModes);


		CFMutableDictionaryRef match = IOServiceMatching(kIOSerialBSDServiceValue);
		if (!match)
		{
			VUserLog("Error: Couldn't create serial matching dictionary.");
			return;
		}
		CFDictionarySetValue(match, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

		// Since each IOServiceAddMatchingNotification() call releases the matching dictionary, retain it in order to make it through both calls.
		CFRetain(match);


		io_iterator_t it;
		kern_return_t ret = IOServiceAddMatchingNotification(VuoSerial_notificationPort, kIOMatchedNotification, match, VuoSerial_servicesChanged, NULL, &it);
		if (ret != KERN_SUCCESS)
		{
			VUserLog("Error: Couldn't create serial-add notification: %d", ret);
			return;
		}
		// The notification doesn't start firing until we've looked at all of the existing items once.
		while (IOIteratorNext(it));


		ret = IOServiceAddMatchingNotification(VuoSerial_notificationPort, kIOTerminatedNotification, match, VuoSerial_servicesChanged, NULL, &it);
		if (ret != KERN_SUCCESS)
		{
			VUserLog("Error: Couldn't create serial-remove notification: %d", ret);
			return;
		}
		// The notification doesn't start firing until we've looked at all of the existing items once.
		while (IOIteratorNext(it));
	}
}

/**
 * Indicates that the caller no longer needs notifications about serial devices.
 *
 * @threadAny
 */
void VuoSerial_disuse(void)
{
	if (VuoSerial_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoSerial_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoSerial_useCount, 1) == 0)
		IONotificationPortDestroy(VuoSerial_notificationPort);
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known serial devices changes.
 *
 * Call `VuoSerial_use()` before calling this.
 *
 * @threadAny
 */
void VuoSerial_addDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoSerialDevice))
{
	VuoSerial_deviceCallbacks.addTrigger(devices);
	devices(VuoSerial_getDeviceList());
}

/**
 * Removes a trigger callback previously added by @ref VuoSerial_addDevicesChangedTriggers.
 *
 * @threadAny
 */
void VuoSerial_removeDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoSerialDevice))
{
	VuoSerial_deviceCallbacks.removeTrigger(devices);
}
