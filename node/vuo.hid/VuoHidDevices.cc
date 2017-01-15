/**
 * @file
 * VuoHid implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoHid.h"
#include "VuoPool.hh"
#include "VuoTriggerSet.hh"
#include "VuoIoReturn.h"
#include "VuoHidControl.h"
#include "VuoList_VuoHidControl.h"
#include "VuoEventLoop.h"
#include "VuoUsbVendor.h"

#include <dispatch/dispatch.h>
#include <vector>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/usb/USB.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoHidDevices",
					 "dependencies" : [
						 "CoreFoundation.framework",
						 "IOKit.framework",
						 "VuoHidControl",
						 "VuoHidDevice",
						 "VuoHidUsage",
						 "VuoIoReturn",
						 "VuoList_VuoHidControl",
						 "VuoList_VuoHidDevice",
						 "VuoList_VuoInteger",
						 "VuoUsbVendor"
					 ]
				 });
#endif
}

static VuoTriggerSet<VuoList_VuoHidDevice> VuoHid_deviceCallbacks;		///< Trigger functions to call when the list of devices changes.


/**
 * Given a device, returns its usagePage and usage.
 */
void VuoHid_getDeviceUsage(IOHIDDeviceRef device, uint32_t *usagePage, uint32_t *usage)
{
	CFNumberRef usagePageCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDDeviceUsagePageKey));
	if (usagePageCF)
		CFNumberGetValue(usagePageCF, kCFNumberSInt32Type, usagePage);
	else
	{
		usagePageCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));
		if (usagePageCF)
			CFNumberGetValue(usagePageCF, kCFNumberSInt32Type, usagePage);
	}

	CFNumberRef usageCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDDeviceUsageKey));
	if (usageCF)
		CFNumberGetValue(usageCF, kCFNumberSInt32Type, usage);
	else
	{
		usageCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));
		if (usageCF)
			CFNumberGetValue(usageCF, kCFNumberSInt32Type, usage);
	}
}

/**
 * Given a device, returns strings describing its manufacturer and product model.
 */
void VuoHid_getDeviceDescription(IOHIDDeviceRef device, VuoText *manufacturer, VuoText *product, VuoInteger *vendorID, VuoInteger *productID)
{
	CFNumberRef vendorIDCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
	if (vendorIDCF)
	{
		uint32_t vendorID32 = 0;
		CFNumberGetValue(vendorIDCF, kCFNumberSInt32Type, &vendorID32);
		*vendorID = vendorID32;
	}

	CFStringRef manufacturerCF = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey));
	if (manufacturerCF)
		*manufacturer = VuoText_makeFromCFString(manufacturerCF);
	else
	{
		if (*vendorID)
		{
			char *vendorText = VuoUsbVendor_getText(*vendorID);
			*manufacturer = VuoText_make(vendorText);
			free(vendorText);
		}
	}


	CFNumberRef productIDCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
	if (productIDCF)
	{
		uint32_t productID32 = 0;
		CFNumberGetValue(productIDCF, kCFNumberSInt32Type, &productID32);
		*productID = productID32;
	}


	// When present, use the interface name, which disambiguates endpoints that have multiple interfaces.
	io_service_t service = IOHIDDeviceGetService(device);
	io_registry_entry_t parent;
	IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
	CFStringRef interfaceNameCF = (CFStringRef)IORegistryEntryCreateCFProperty(parent, CFSTR("USB Interface Name"), NULL, 0);
	if (interfaceNameCF)
		*product = VuoText_makeFromCFString(interfaceNameCF);
	else
	{
		CFStringRef productCF = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
		if (productCF)
			*product = VuoText_makeFromCFString(productCF);
		else
		{
			if (*productID)
			{
				char *productText = VuoText_format("0x%04llx", *productID);
				*product = VuoText_make(productText);
				free(productText);
			}
		}
	}
}

/**
 * Given an `IOHIDDeviceRef`, returns its hopefully-unique Vuo-specific location (based on kIOHIDLocationIDKey and kUSBInterfaceNumber).
 */
VuoInteger VuoHid_getLocation(void *d)
{
	IOHIDDeviceRef device = (IOHIDDeviceRef)d;

	CFNumberRef locationIDCF = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDLocationIDKey));
	uint32_t location = 0;
	if (locationIDCF)
		CFNumberGetValue(locationIDCF, kCFNumberSInt32Type, &location);

	io_service_t service = IOHIDDeviceGetService(device);
	io_registry_entry_t parent;
	IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
	CFNumberRef interfaceNumberCF = (CFNumberRef)IORegistryEntryCreateCFProperty(parent, CFSTR(kUSBInterfaceNumber), NULL, 0);
	uint32_t interfaceNumber = 0xff;
	if (interfaceNumberCF)
	{
		CFNumberGetValue(interfaceNumberCF, kCFNumberSInt32Type, &interfaceNumber);
		CFRelease(interfaceNumberCF);
	}

	return (location << 8) + interfaceNumber;
}

/**
 * Returns true if the specified `IOHIDElementRef` is valid (contains usable data).
 */
bool VuoHid_isElementValid(void *e)
{
	IOHIDElementRef element = (IOHIDElementRef)e;

//	uint32_t usagePage = IOHIDElementGetUsagePage(element);
	uint32_t usage     = IOHIDElementGetUsage(element);

	// Skip vendor-defined pages since they don't provide data in a well-defined format.
	// For example, in "Apple Internal Keyboard / Trackpad" there are 514 vendor-defined elements, apparently always having value 0.
//	if (usagePage >= 0xff00)
//		return false;

	// Skip out-of-range usages since they don't provide valid data.
	// For example, in "Apple Mikey HID Driver" the values are less than min.
	if (usage > 0xffff)
		return false;

	// All the interesting data seems to be Misc (integer values) and Button.
	IOHIDElementType elementType = IOHIDElementGetType(element);
	if (elementType != kIOHIDElementTypeInput_Misc
	 && elementType != kIOHIDElementTypeInput_Button)
		 return false;

	return true;
}

/**
 * Creates a VuoHidControl from the values of the specified `IOHIDElementRef`.
 */
VuoHidControl VuoHid_getControlForElement(void *e)
{
	IOHIDElementRef element = (IOHIDElementRef)e;

	uint32_t usagePage = IOHIDElementGetUsagePage(element);
	uint32_t usage     = IOHIDElementGetUsage(element);
	char *usageText = VuoHid_getUsageText(usagePage, usage);

	VuoInteger min = IOHIDElementGetLogicalMin(element);
	VuoInteger max = IOHIDElementGetLogicalMax(element);
	VuoHidControl control = {
		VuoText_make(usageText),
		VuoInteger_clamp(0, min, max),
		min,
		max
	};
	free(usageText);

	return control;
}

/**
 * Returns a list of the currently-avaialble HID devices.
 */
VuoList_VuoHidDevice VuoHid_getDeviceList(void)
{
	VDebugLog("Devices:");

	IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (!manager)
	{
		VUserLog("Error: Couldn't initialize IOHIDManager.");
		return NULL;
	}

	IOHIDManagerSetDeviceMatching(manager, NULL);

	IOHIDDeviceRef *devicesIO = NULL;
	CFIndex deviceCount;
	{
		IOReturn ret = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
		if (ret != kIOReturnSuccess)
		{
			if (ret == kIOReturnExclusiveAccess)
			{
				// This apparently just means that at least one device is already open for exclusive access,
				// and other devices were successfully opened.
			}
			else
			{
				char *text = VuoIoReturn_getText(ret);
				VUserLog("Error: Couldn't open IOHIDManager: %s", text);
				free(text);
				CFRelease(manager);
				return NULL;
			}
		}

		CFSetRef deviceCF = IOHIDManagerCopyDevices(manager);
		if (!deviceCF)
		{
			VUserLog("Error: Couldn't copy device list.");
			CFRelease(manager);
			return NULL;
		}

		deviceCount = CFSetGetCount(deviceCF);
		devicesIO = (IOHIDDeviceRef *)malloc(sizeof(IOHIDDeviceRef) * deviceCount);
		CFSetGetValues(deviceCF, (const void **)devicesIO);
		CFRelease(deviceCF);
	}

	VuoList_VuoHidDevice devices = VuoListCreate_VuoHidDevice();
	for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		uint32_t usagePage = 0;
		uint32_t usage = 0;
		VuoHid_getDeviceUsage(devicesIO[deviceIndex], &usagePage, &usage);

		VuoText manufacturer = NULL;
		VuoText product = NULL;
		VuoInteger vendorID = 0;
		VuoInteger productID = 0;
		VuoHid_getDeviceDescription(devicesIO[deviceIndex], &manufacturer, &product, &vendorID, &productID);
		VuoRetain(manufacturer);
		VuoRetain(product);

		// Ignore "Touchpad (Apple Inc. Mouse)" and "Touchpad (Apple Inc. Vendor-defined)" since they doesn't seem to output any data.
		if (vendorID == 0x05ac
				&& (   productID == 0x0262	// MacBookPro10,1
					|| productID == 0x0259)	// MacBookPro10,2
				&& ((usagePage == 0x0001 && usage == 0x0002)
				 || (usagePage == 0xff00 && usage == 0x0001))
				&& manufacturer
				&& product
				&& strcmp(manufacturer, "Apple Inc.") == 0
				&& strcmp(product, "Touchpad") == 0)
		{
			VuoRelease(manufacturer);
			VuoRelease(product);
			continue;
		}

		// Provide a clearer name for "Apple Internal Keyboard / Trackpad".
		if (product && strcmp(product, "Apple Internal Keyboard / Trackpad") == 0)
		{
			VuoRelease(product);
			product = VuoText_make("Apple Internal Trackpad");
			VuoRetain(product);
		}

		char *usageText = VuoHid_getUsageText(usagePage,usage);
		VuoHidDevice device = {VuoHidDevice_MatchLocation, NULL, 0, NULL, vendorID, productID, usagePage, usage};
		if (manufacturer && product)
			device.name = VuoText_make(VuoText_format("%s (%s %s)", product, manufacturer, usageText));
		else if (manufacturer)
			device.name = VuoText_make(VuoText_format("%s (%s)", manufacturer, usageText));
		else if (product)
			device.name = VuoText_make(VuoText_format("%s (%s)", product, usageText));
		else
			device.name = VuoText_make(VuoText_format("%s", usageText));
		free(usageText);

		device.location = VuoHid_getLocation(devicesIO[deviceIndex]);

		VDebugLog("\t%08llx:%02llx \"%s\"", device.location>>8, device.location&0xff, device.name);

		CFArrayRef elementsCF = IOHIDDeviceCopyMatchingElements(devicesIO[deviceIndex], NULL, kIOHIDOptionsTypeNone);
		if (elementsCF)
		{
			CFIndex elementCount = CFArrayGetCount(elementsCF);
			VuoList_VuoHidControl controls = VuoListCreate_VuoHidControl();
			for (CFIndex elementIndex = 0; elementIndex < elementCount; ++elementIndex)
			{
				IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elementsCF, elementIndex);
				if (!element)
					continue;

				if (!VuoHid_isElementValid(element))
					continue;

				VuoListAppendValue_VuoHidControl(controls, VuoHid_getControlForElement(element));
			}

			CFRelease(elementsCF);

			device.controls = controls;

			if (VuoListGetCount_VuoHidControl(controls))
				VuoListAppendValue_VuoHidDevice(devices, device);
			else
				VDebugLog("\t\tSkipping this device since it doesn't have any valid elements.");
		}
		else
			VDebugLog("\t\tSkipping this device since I can't get its elements (perhaps another process has it open for exclusive access).");

		VuoRelease(manufacturer);
		VuoRelease(product);
	}

	VuoListSort_VuoHidDevice(devices);

	free(devicesIO);
	CFRelease(manager);
	return devices;
}

unsigned int VuoHid_useCount = 0;	///< Process-wide count of callers (typically node instances) interested in notifications about Hid devices.
IOHIDManagerRef VuoHid_manager;	///< Manages notifications about HID devices.
bool VuoHid_devicesChangedCalled = false;	///< Has VuoHid_devicesChanged been called since our HID Manager was initialized?
bool VuoHid_devicesChangedInitialized = false;	///< Has HID Manager initialization completed?

/**
 * Invoked by the IONotification system when a HID device is added or removed.
 */
static void VuoHid_devicesChanged(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
	if (VuoHid_devicesChangedInitialized)
		VuoHid_deviceCallbacks.fire(VuoHid_getDeviceList());

	VuoHid_checkPendingDevices();
	VuoHid_devicesChangedCalled = true;
}

/**
 * Indicates that the caller needs to get notifications about HID devices.
 *
 * @threadAny
 */
void VuoHid_use(void)
{
	if (__sync_add_and_fetch(&VuoHid_useCount, 1) == 1)
	{
		VuoHid_devicesChangedCalled = false;
		VuoHid_devicesChangedInitialized = false;

		VuoHid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
		if (!VuoHid_manager)
		{
			VUserLog("Error: Couldn't initialize IOHIDManager.");
			return;
		}

		IOHIDManagerSetDeviceMatching(VuoHid_manager, NULL);


		IOReturn ret = IOHIDManagerOpen(VuoHid_manager, kIOHIDOptionsTypeNone);
		if (ret != kIOReturnSuccess)
		{
			if (ret == kIOReturnExclusiveAccess)
			{
				// This "error" apparently just means that at least one device is already open for exclusive access,
				// yet other devices may have been successfully opened.
			}
			else
			{
				char *text = VuoIoReturn_getText(ret);
				VUserLog("Error: Couldn't open IOHIDManager: %s", text);
				free(text);
				CFRelease(VuoHid_manager);
				return;
			}
		}

		IOHIDManagerRegisterDeviceMatchingCallback(VuoHid_manager, &VuoHid_devicesChanged, NULL);
		IOHIDManagerRegisterDeviceRemovalCallback (VuoHid_manager, &VuoHid_devicesChanged, NULL);
		IOHIDManagerScheduleWithRunLoop(VuoHid_manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		// Wait for the HID Manager to invoke the callback with the existing devices.
		while (!VuoHid_devicesChangedCalled)
		{
			VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
			usleep(USEC_PER_SEC / 10);
		}
		VuoHid_devicesChangedInitialized = true;
	}
}

/**
 * Indicates that the caller no longer needs notifications about HID devices.
 *
 * @threadAny
 */
void VuoHid_disuse(void)
{
	if (VuoHid_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoHid_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoHid_useCount, 1) == 0)
		CFRelease(VuoHid_manager);
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known HID devices changes.
 *
 * Call `VuoHid_use()` before calling this.
 *
 * @threadAny
 */
void VuoHid_addDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoHidDevice))
{
	VuoHid_deviceCallbacks.addTrigger(devices);
	devices(VuoHid_getDeviceList());
}

/**
 * Removes a trigger callback previously added by @ref VuoHid_addDevicesChangedTriggers.
 *
 * @threadAny
 */
void VuoHid_removeDevicesChangedTriggers(VuoOutputTrigger(devices, VuoList_VuoHidDevice))
{
	VuoHid_deviceCallbacks.removeTrigger(devices);
}
