/**
 * @file
 * VuoQTCapture implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"
#include "/usr/include/time.h"
#include "VuoQTCapture.h"
#include "VuoQtListener.h"
#include "VuoTriggerSet.hh"
#include "VuoOsStatus.h"
#include <QTKit/QTkit.h>
#include <IOKit/IOKitLib.h>
#include <CoreMediaIO/CMIOHardware.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoQtCapture",
					  "dependencies" : [
						"VuoImage",
						"VuoOsStatus",
						"VuoVideoInputDevice",
						"VuoList_VuoVideoInputDevice",
						"CoreMediaIO.framework",
						"QTKit.framework",
						"IOKit.framework",
						"AppKit.framework",
						"VuoQtListener"
					  ]
				 });
#endif

/**
 * Enable support for video capture from tethered iOS devices.
 */
static void __attribute__((constructor)) VuoQTCapture_init()
{
	// kCMIOHardwarePropertyAllowScreenCaptureDevices (which doesn't exist until OS X v10.10)
	int allowScreenCaptureDevices = 'yes ';

	CMIOObjectPropertyAddress property = { allowScreenCaptureDevices, kCMIOObjectPropertyScopeGlobal, kCMIOObjectPropertyElementMaster };
	if (CMIOObjectHasProperty(kCMIOObjectSystemObject, &property))
	{
		UInt32 yes = 1;
		OSStatus ret = CMIOObjectSetPropertyData(kCMIOObjectSystemObject, &property, 0, NULL, sizeof(yes), &yes);
		if (ret != kCMIOHardwareNoError)
		{
			char *errorText = VuoOsStatus_getText(ret);
			VUserLog("Warning: Couldn't enable tethered iOS device support: %s", errorText);
			free(errorText);
		}
	}
}

/**
 *	A singleton class that listens for changes in available video input devices.
 */
@interface OnDevicesChangedListener : NSObject
{
	VuoTriggerSet<VuoList_VuoVideoInputDevice> callbacks;							///< A TriggerSet of VuoList_VuoVideoInputDevice output triggers.
}

- (id) init;																		///< Init and register for NSNotification events.
- (void) devicesDidChange:(NSNotification*)notif;									///< Called when a change is detected.
- (void) addCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger;			///< Add a callback to be triggered when the input list changes.
- (void) removeCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger;		///< Remove a callback from the trigger set.
@end

@implementation OnDevicesChangedListener

- (id) init
{
	if(!self)
		self = [super init];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasConnectedNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasDisconnectedNotification object:nil];

	return self;
}

- (void) addCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger
{
	callbacks.addTrigger(outputTrigger);
	outputTrigger(VuoQTCapture_getInputDevices());
}

- (void) removeCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger
{
	callbacks.removeTrigger(outputTrigger);
}

- (void) devicesDidChange:(NSNotification*)notif
{
	callbacks.fire(VuoQTCapture_getInputDevices());
}
@end

static OnDevicesChangedListener *deviceListener;	///< A static reference to the OnDevicesChangedListener singleton.

/**
 *	Remove an output trigger from the OnDevicesChangedListener TriggerSet.
 */
void VuoQTCapture_removeOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) )
{
	if(deviceListener != nil)
		[deviceListener removeCallback:devicesDidChange];
}

/**
 *	Add an output trigger to the OnDevicesChangedListener TriggerSet to be fired when available input devices changes.
 */
void VuoQTCapture_addOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) )
{
	if(deviceListener == nil)
		deviceListener = [[OnDevicesChangedListener alloc] init];
	[deviceListener addCallback:devicesDidChange];
}

extern "C" {
NSString *VuoQTCapture_getVendorNameForUniqueID(NSString *uniqueID);
}

/**
 * Given a `uniqueID` string (e.g., `0x1a11000005ac8510`, from `-[QTCaptureDevice uniqueID]`),
 * searches the IORegistry to find a matching FireWire or USB device,
 * and returns the Vendor Name string.
 *
 * The returned string has a retain count +1, so the caller should release it.
 */
NSString *VuoQTCapture_getVendorNameForUniqueID(NSString *uniqueID)
{
	CFMutableDictionaryRef match_dictionary = IOServiceMatching("IOFireWireDevice");
	io_iterator_t entry_iterator;
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, match_dictionary, &entry_iterator) == kIOReturnSuccess)
	{
		io_registry_entry_t serviceObject;
		while ((serviceObject = IOIteratorNext(entry_iterator)))
		{
			CFMutableDictionaryRef serviceDictionary;
			if (IORegistryEntryCreateCFProperties(serviceObject, &serviceDictionary, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
			{
				IOObjectRelease(serviceObject);
				continue;
			}

			NSString *guidAsHexString = [NSString stringWithFormat:@"%llx",[(NSNumber *)CFDictionaryGetValue(serviceDictionary, @"GUID") longLongValue]];
			if ([uniqueID rangeOfString:guidAsHexString].location != NSNotFound)
			{
				NSString *vendorName = [(NSString *)CFDictionaryGetValue(serviceDictionary, @"FireWire Vendor Name") retain];
				CFRelease(serviceDictionary);
				IOObjectRelease(serviceObject);
				IOObjectRelease(entry_iterator);
				return vendorName;
			}

			CFRelease(serviceDictionary);
			IOObjectRelease(serviceObject);
		}
		IOObjectRelease(entry_iterator);
	}

	match_dictionary = IOServiceMatching("IOUSBDevice");
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, match_dictionary, &entry_iterator) == kIOReturnSuccess)
	{
		io_registry_entry_t serviceObject;
		while ((serviceObject = IOIteratorNext(entry_iterator)))
		{
			CFMutableDictionaryRef serviceDictionary;
			if (IORegistryEntryCreateCFProperties(serviceObject, &serviceDictionary, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
			{
				IOObjectRelease(serviceObject);
				continue;
			}

			NSString *guidAsHexString = [NSString stringWithFormat:@"%llx",[(NSNumber *)CFDictionaryGetValue(serviceDictionary, @"locationID") longLongValue]];
			if ([uniqueID rangeOfString:guidAsHexString].location != NSNotFound)
			{
				NSString *vendorName = [(NSString *)CFDictionaryGetValue(serviceDictionary, @"USB Vendor Name") retain];
				CFRelease(serviceDictionary);
				IOObjectRelease(serviceObject);
				IOObjectRelease(entry_iterator);
				return vendorName;
			}

			CFRelease(serviceDictionary);
			IOObjectRelease(serviceObject);
		}
		IOObjectRelease(entry_iterator);
	}

	return @"";
}

/**
 * Returns a list of all available video capture devices.
 */
VuoList_VuoVideoInputDevice VuoQTCapture_getInputDevices(void)
{
	NSArray *inputDevices = [[[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo] arrayByAddingObjectsFromArray:[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed]] retain];

	VuoList_VuoVideoInputDevice devices = VuoListCreate_VuoVideoInputDevice();

	for(QTCaptureDevice *dev in inputDevices) {
		VuoText uniqueID    = VuoText_make([[dev uniqueID]             UTF8String]);

		NSString *deviceName = [dev localizedDisplayName];
		NSString *vendorName = VuoQTCapture_getVendorNameForUniqueID([dev uniqueID]);
		if ([vendorName length])
			deviceName = [NSString stringWithFormat:@"%@ %@", vendorName, deviceName];
		[vendorName release];
		VuoText displayName = VuoText_make([deviceName UTF8String]);

		VuoListAppendValue_VuoVideoInputDevice(devices, VuoVideoInputDevice_make(uniqueID, displayName));
	}

	[inputDevices release];

	return devices;
}

/**
 *	Forward declaration of destructor method.
 */
void VuoQTCapture_free(VuoQTCapture movie);

/**
 * Creates a new VuoQTCapture instance for receiving video frames from `inputDevice`.
 */
VuoQTCapture VuoQTCapture_make(VuoVideoInputDevice inputDevice, VuoOutputTrigger(receivedFrame, VuoVideoFrame))
{
	VuoQtListener *listener = [[VuoQtListener alloc] init];

	[listener initWithDevice:[NSString stringWithFormat:@"%s", inputDevice.name] id:[NSString stringWithFormat:@"%s", inputDevice.id] callback:receivedFrame];

	VuoRegister(listener, VuoQTCapture_free);

	return (VuoQTCapture) listener;
}

/**
 *	Change the device input.
 */
void VuoQtCapture_setInputDevice(VuoQTCapture movie, VuoVideoInputDevice inputDevice)
{
	VuoQtListener *listener = (VuoQtListener*)movie;
	[listener setInputDevice:[NSString stringWithFormat:@"%s", inputDevice.name] id:[NSString stringWithFormat:@"%s", inputDevice.id]];
}

/**
 * Begins receiving frames.
 */
void VuoQTCapture_startListening(VuoQTCapture movie)
{
	VuoQtListener *listener = (VuoQtListener*)movie;
	[listener startRunning];
}

/**
 * Stops receiving frames (starts ignoring/dropping them).
 */
void VuoQTCapture_stopListening(VuoQTCapture movie)
{
	VuoQtListener *listener = (VuoQtListener*)movie;
	[listener stopRunning];
}

/**
 *	Returns true if QT has been initialized, false otherwise.
 */
VuoBoolean VuoQTCapture_isInitialized(VuoQTCapture movie)
{
	if(movie == NULL)
		return false;

	VuoQtListener *listener = (VuoQtListener*)movie;

	return (bool)[listener isInitialized];
}

/**
 *	Set the image callback.
 */
void VuoQTCapture_setCallback(VuoQTCapture movie, VuoOutputTrigger(receivedFrame, VuoVideoFrame))
{
	VuoQtListener *listener = (VuoQtListener*)movie;
	[listener setCallback:receivedFrame];
}

/**
 * Destructor.
 */
void VuoQTCapture_free(VuoQTCapture movie)
{
	VuoQtListener *listener = (VuoQtListener *)movie;

	[listener stopRunning];
	[listener release];
}
