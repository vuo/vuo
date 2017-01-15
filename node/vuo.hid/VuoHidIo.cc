/**
 * @file
 * VuoHidIo implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoHid.h"
#include "VuoTriggerSet.hh"
#include "VuoIoReturn.h"

#include <IOKit/hid/IOHIDLib.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoHidIo",
					 "dependencies" : [
						 "VuoHidDevice",
						 "VuoHidDevices",
						 "VuoIoReturn",
						 "VuoList_VuoHidDevice"
					 ]
				 });
#endif
}


/**
 * Private data for a VuoHid instance.
 */
typedef struct _VuoHid_internal
{
	VuoHidDevice device;		///< The target device this instance represents.
	VuoBoolean exclusive;		///< Whether the user wants this device opened for exclusive access.

	VuoTriggerSet<VuoHidControl> triggerSet;
	IOHIDManagerRef manager;
} *VuoHid_internal;


static dispatch_queue_t VuoHid_pendingDevicesQueue;	///< Serializes access to `VuoHid_pendingDevices`.
static std::set<VuoHid_internal> VuoHid_pendingDevices;	///< Devices that have been requested but haven't yet been successfully opened.


/**
 * Initializes `VuoHid_pendingDevicesQueue`.
 */
static void __attribute__((constructor)) VuoHidIo_init()
{
	VuoHid_pendingDevicesQueue = dispatch_queue_create("org.vuo.hid.pendingDevicesQueue", NULL);
}

/**
 * Reads a HID controller value, and fires the triggers associated with its device.
 */
static void VuoHid_inputValueCallback(void *context, IOReturn result, void *manager, IOHIDValueRef value)
{
	if (result != kIOReturnSuccess)
	{
		VUserLog("Error: %s", VuoIoReturn_getText(result));
		return;
	}

	VuoHid_internal si = (VuoHid_internal)context;

	// IOHIDManager can't match on kIOHIDLocationIDKey, so we have to match on other criteria, then filter those results by location.
	IOHIDElementRef element = IOHIDValueGetElement(value);
	IOHIDDeviceRef device = IOHIDElementGetDevice(element);
	if (VuoHid_getLocation(device) != si->device.location)
		return;

	VuoHidControl control = VuoHid_getControlForElement(element);

//	if (!VuoHid_isElementValid(element))
//	{
//		VLog("Error: invalid element (%s)", control.name);
//		return;
//	}

	control.value = IOHIDValueGetIntegerValue(value);

	si->triggerSet.fire(control);
}

/**
 * Attempts to open the specified device.
 */
static void VuoHid_openDevice(VuoHid_internal si)
{
	si->manager = NULL;


	VuoHidDevice realizedDevice;
	if (!VuoHidDevice_realize(si->device, &realizedDevice))
		return;

	VuoHidDevice_retain(realizedDevice);
	VuoHidDevice_release(si->device);
	si->device = realizedDevice;


	si->manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (!si->manager)
	{
		VUserLog("Error: Couldn't initialize IOHIDManager.");
		return;
	}


	CFMutableDictionaryRef matchesCF = CFDictionaryCreateMutable(NULL, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	// IOHIDManager can't match on kIOHIDLocationIDKey, so we have to match on other criteria, then filter those results by location.
//	uint32_t location = si->device.location >> 8;
//	CFNumberRef locationCF  = CFNumberCreate(NULL, kCFNumberSInt32Type, &location);
//	CFDictionarySetValue(matchesCF, CFSTR(kIOHIDLocationIDKey), locationCF);
//	CFRelease(locationCF);

	if (realizedDevice.vendorID)
	{
		CFNumberRef vendorCF    = CFNumberCreate(NULL, kCFNumberSInt64Type, &realizedDevice.vendorID);
		CFDictionarySetValue(matchesCF, CFSTR(kIOHIDVendorIDKey), vendorCF);
		CFRelease(vendorCF);
	}

	if (realizedDevice.productID)
	{
		CFNumberRef productCF   = CFNumberCreate(NULL, kCFNumberSInt64Type, &realizedDevice.productID);
		CFDictionarySetValue(matchesCF, CFSTR(kIOHIDProductIDKey), productCF);
		CFRelease(productCF);
	}

	if (realizedDevice.usagePage)
	{
		CFNumberRef usagePageCF = CFNumberCreate(NULL, kCFNumberSInt64Type, &realizedDevice.usagePage);
		CFDictionarySetValue(matchesCF, CFSTR(kIOHIDDeviceUsagePageKey), usagePageCF);
		CFRelease(usagePageCF);
	}

	if (realizedDevice.usage)
	{
		CFNumberRef usageCF     = CFNumberCreate(NULL, kCFNumberSInt64Type, &realizedDevice.usage);
		CFDictionarySetValue(matchesCF, CFSTR(kIOHIDDeviceUsageKey), usageCF);
		CFRelease(usageCF);
	}

	IOHIDManagerSetDeviceMatching(si->manager, matchesCF);
	CFRelease(matchesCF);


	IOReturn ret = IOHIDManagerOpen(si->manager, si->exclusive ? kIOHIDOptionsTypeSeizeDevice : kIOHIDOptionsTypeNone);
	if (ret != kIOReturnSuccess)
	{
		char *text = VuoIoReturn_getText(ret);
		VUserLog("Error: Couldn't open IOHIDManager: %s", text);
		free(text);
		CFRelease(si->manager);
		si->manager = NULL;
		return;
	}


	CFSetRef deviceCF = IOHIDManagerCopyDevices(si->manager);
	if (!deviceCF)
	{
		VUserLog("Error: Couldn't copy device list.");
		CFRelease(si->manager);
		si->manager = NULL;
		return;
	}
	if (CFSetGetCount(deviceCF) == 0)
	{
		CFRelease(deviceCF);
		CFRelease(si->manager);
		si->manager = NULL;
		return;
	}
	CFRelease(deviceCF);


	IOHIDManagerRegisterInputValueCallback(si->manager, VuoHid_inputValueCallback, si);

	IOHIDManagerScheduleWithRunLoop(si->manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
}

/**
 * Attempts to open each device that has been requested but hasn't yet been successfully opened.
 *
 * @threadAny
 */
void VuoHid_checkPendingDevices(void)
{
	dispatch_async(VuoHid_pendingDevicesQueue, ^{
		for (std::set<VuoHid_internal>::iterator i = VuoHid_pendingDevices.begin(); i != VuoHid_pendingDevices.end();)
		{
			VuoHid_openDevice(*i);
			if (!(*i)->manager)
			{
				++i;
				continue;
			}

			VuoHid_pendingDevices.erase(i++);
		}
	});
}

/**
 * Frees the reference-counted HID device object.
 */
static void VuoHid_destroy(VuoHid_internal si)
{
	dispatch_sync(VuoHid_pendingDevicesQueue, ^{
		VuoHid_pendingDevices.erase(si);
	});

	if (si->manager)
	{
		VUOLOG_PROFILE_BEGIN(mainQueue);
		dispatch_sync(dispatch_get_main_queue(), ^{
						  VUOLOG_PROFILE_END(mainQueue);
						  IOHIDManagerRegisterInputValueCallback(si->manager, NULL, NULL);
						  IOHIDManagerClose(si->manager, kIOHIDOptionsTypeNone);
						  CFRelease(si->manager);
		});
	}

	VuoHidDevice_release(si->device);
	delete si;
}

/**
 * Creates a reference-counted object for the specified HID device.
 */
VuoHid VuoHid_make(const VuoHidDevice device, const VuoBoolean exclusive)
{
	if (!device.name)
		return NULL;

	VuoHid_internal si = new _VuoHid_internal;
	VuoRegister(si, (DeallocateFunctionType)VuoHid_destroy);

	si->device = device;
	VuoHidDevice_retain(si->device);

	si->exclusive = exclusive;

	VuoHid_openDevice(si);

	if (!si->manager)
	{
		VUserLog("Warning: HID device '%s' isn't currently available.  I'll keep trying.", VuoHidDevice_getSummary(si->device));
		dispatch_async(VuoHid_pendingDevicesQueue, ^{
			VuoHid_pendingDevices.insert(si);
		});
	}

	return si;
}



/**
 * Adds a trigger callback, to be invoked whenever the specified HID device receives data.
 *
 * @threadAny
 */
void VuoHid_addReceiveTrigger(VuoHid device, VuoOutputTrigger(receivedControl, VuoHidControl))
{
	if (!device)
		return;

	VuoHid_internal si = (VuoHid_internal)device;
	si->triggerSet.addTrigger(receivedControl);
}


/**
 * Removes a trigger callback previously added by @ref VuoHid_addReceiveTrigger.
 *
 * @threadAny
 */
void VuoHid_removeReceiveTrigger(VuoHid device, VuoOutputTrigger(receivedControl, VuoHidControl))
{
	if (!device)
		return;

	VuoHid_internal si = (VuoHid_internal)device;
	si->triggerSet.removeTrigger(receivedControl);
}
