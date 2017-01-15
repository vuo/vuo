/**
 * @file
 * VuoOscDevices implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoOsc.h"
#include "VuoTriggerSet.hh"

#include <CoreServices/CoreServices.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoOscDevices",
					 "dependencies" : [
						 "CoreServices.framework",
						 "VuoOscInputDevice",
						 "VuoOscOutputDevice",
						 "VuoList_VuoOscInputDevice",
						 "VuoList_VuoOscOutputDevice"
					 ]
				 });
#endif
}

static dispatch_queue_t VuoOsc_deviceQueue;											///< Serializes access to the below device lists.
static VuoList_VuoOscInputDevice  VuoOsc_inputDevices;								///< All known input devices.
static VuoList_VuoOscOutputDevice VuoOsc_outputDevices;								///< All known output devices.
static VuoTriggerSet<VuoList_VuoOscInputDevice>  VuoOsc_inputDeviceCallbacks;		///< Trigger functions to call when the list of devices changes.
static VuoTriggerSet<VuoList_VuoOscOutputDevice> VuoOsc_outputDeviceCallbacks;		///< Trigger functions to call when the list of devices changes.

/**
 * Returns a list of the currently-avaialble OSC input devices.
 */
VuoList_VuoOscInputDevice VuoOsc_getInputDeviceList(void)
{
	__block VuoList_VuoOscInputDevice devices;
	dispatch_sync(VuoOsc_deviceQueue, ^{
					  devices = VuoListCopy_VuoOscInputDevice(VuoOsc_inputDevices);
				  });
	return devices;
}

/**
 * Returns a list of the currently-avaialble OSC output devices.
 */
VuoList_VuoOscOutputDevice VuoOsc_getOutputDeviceList(void)
{
	__block VuoList_VuoOscOutputDevice devices;
	dispatch_sync(VuoOsc_deviceQueue, ^{
					  devices = VuoListCopy_VuoOscOutputDevice(VuoOsc_outputDevices);
				  });
	return devices;
}

/**
 * Called when service details have been resolved.
 */
void VuoOsc_clientCallback(CFNetServiceRef service, CFStreamError *error, void *info)
{
	if (error->domain || error->error)
	{
		VUserLog("Error: CFStreamError (%ld:%d)", error->domain, error->error);
		return;
	}

	CFStringRef nameCF = CFNetServiceGetName(service);
	VuoText name = VuoText_makeFromCFString(nameCF);
	VuoRetain(name);

	CFDataRef txt = CFNetServiceGetTXTData(service);
	CFDictionaryRef txtDict = CFNetServiceCreateDictionaryWithTXTData(NULL, txt);
	CFDataRef typeData = (CFDataRef)CFDictionaryGetValue(txtDict, CFSTR("type"));
	bool isInput = true;
	if (typeData)
	{
		VuoText type = VuoText_makeFromData(CFDataGetBytePtr(typeData), CFDataGetLength(typeData));
		VuoRetain(type);
		if (VuoText_areEqual(type, "server"))
			isInput = false;
		VuoRelease(type);
	}
	CFRelease(txtDict);


	SInt32 port = CFNetServiceGetPortNumber(service);

	CFArrayRef addresses = CFNetServiceGetAddressing(service);
	CFIndex addressCount = CFArrayGetCount(addresses);

	for (CFIndex i = 0; i < addressCount; ++i)
	{
		struct sockaddr *socketAddress = (struct sockaddr *)CFDataGetBytePtr((CFDataRef)CFArrayGetValueAtIndex(addresses, i));

		// Ignore IPv6 for now.
		if (!socketAddress || socketAddress->sa_family != AF_INET)
			continue;

		char ip[256];
		if (!inet_ntop(AF_INET, &((struct sockaddr_in *)socketAddress)->sin_addr, ip, sizeof(ip)))
		{
			VUserLog("Error: Couldn't get IP address for '%s': %s", name, strerror(errno));
			continue;
		}

		VuoText ipAddress = VuoText_make(ip);
		VuoRetain(ipAddress);

		if (isInput)
		{
			VuoOscInputDevice d = VuoOscInputDevice_make(name, ipAddress, port);
			dispatch_sync(VuoOsc_deviceQueue, ^{
							  VuoListAppendValue_VuoOscInputDevice(VuoOsc_inputDevices, d);
							  VuoOsc_inputDeviceCallbacks.fire(VuoOsc_inputDevices);
						  });
		}
		else
		{
			VuoOscOutputDevice d = VuoOscOutputDevice_makeUnicast(name, ipAddress, port);
			dispatch_sync(VuoOsc_deviceQueue, ^{
							  VuoListAppendValue_VuoOscOutputDevice(VuoOsc_outputDevices, d);
							  VuoOsc_outputDeviceCallbacks.fire(VuoOsc_outputDevices);
						  });
		}

		VuoRelease(ipAddress);
	}

	VuoRelease(name);
}

/**
 * Called when a new domain or service appears.
 */
void VuoOsc_deviceCallback(CFNetServiceBrowserRef browser, CFOptionFlags flags, CFTypeRef domainOrService, CFStreamError *error, void *info)
{
	if (error->domain || error->error)
	{
		VUserLog("Error: %ld:%d", error->domain, error->error);
		return;
	}

	// We only care about services.
	if (flags & kCFNetServiceFlagIsDomain)
		return;

	bool deviceAdded = !(flags & kCFNetServiceFlagRemove);

	CFNetServiceRef service = (CFNetServiceRef)domainOrService;
	if (!service)
		return;

	if (deviceAdded)
	{
		CFNetServiceClientContext context;
		context.version = 0;
		context.info = NULL;
		context.retain = NULL;
		context.release = NULL;
		context.copyDescription = NULL;
		if (!CFNetServiceSetClient(service, VuoOsc_clientCallback, &context))
			VUserLog("Error: Failed to set the client callback.");

		CFNetServiceScheduleWithRunLoop(service, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		CFStreamError error;
		if (!CFNetServiceResolveWithTimeout(service, 0, &error))
			VUserLog("Error: Failed to schedule service resolution (%ld:%d).",error.domain,error.error);
	}
	else
	{
		CFStringRef nameCF = CFNetServiceGetName(service);
		VuoText name = VuoText_makeFromCFString(nameCF);
		VuoRetain(name);

		dispatch_sync(VuoOsc_deviceQueue, ^{
						  VuoInteger inputCount = VuoListGetCount_VuoOscInputDevice(VuoOsc_inputDevices);
						  bool removedAnyInputs = false;
						  for (VuoInteger i = inputCount; i > 0; --i)
						  {
							  VuoOscInputDevice d = VuoListGetValue_VuoOscInputDevice(VuoOsc_inputDevices, i);
							  if (VuoText_areEqual(d.name, name))
							  {
								  VuoListRemoveValue_VuoOscInputDevice(VuoOsc_inputDevices, i);
								  removedAnyInputs = true;
							  }
						  }
						  if (removedAnyInputs)
							  VuoOsc_inputDeviceCallbacks.fire(VuoOsc_inputDevices);


						  VuoInteger outputCount = VuoListGetCount_VuoOscOutputDevice(VuoOsc_outputDevices);
						  bool removedAnyOutputs = false;
						  for (VuoInteger i = outputCount; i > 0; --i)
						  {
							  VuoOscOutputDevice d = VuoListGetValue_VuoOscOutputDevice(VuoOsc_outputDevices, i);
							  if (VuoText_areEqual(d.name, name))
							  {
								  VuoListRemoveValue_VuoOscOutputDevice(VuoOsc_outputDevices, i);
								  removedAnyOutputs = true;
							  }
						  }
						  if (removedAnyOutputs)
							  VuoOsc_outputDeviceCallbacks.fire(VuoOsc_outputDevices);
					  });

		VuoRelease(name);

		CFNetServiceUnscheduleFromRunLoop(service, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		CFNetServiceClientContext context;
		context.version = 0;
		context.info = NULL;
		context.retain = NULL;
		context.release = NULL;
		context.copyDescription = NULL;
		if (!CFNetServiceSetClient(service, NULL, &context))
			VUserLog("Error: Failed to clear the client callback.");

		CFNetServiceCancel(service);
	}
}

unsigned int VuoOsc_useCount = 0;	///< Process-wide count of callers (typically node instances) interested in notifications about OSC devices.
CFNetServiceBrowserRef VuoOsc_browser;	///< Manages notifications about OSC devices.

/**
 * Indicates that the caller needs to get notifications about OSC devices.
 *
 * @threadAny
 */
void VuoOsc_use(void)
{
	if (__sync_add_and_fetch(&VuoOsc_useCount, 1) == 1)
	{
		VuoOsc_inputDevices = VuoListCreate_VuoOscInputDevice();
		VuoRetain(VuoOsc_inputDevices);

		VuoOsc_outputDevices = VuoListCreate_VuoOscOutputDevice();
		VuoRetain(VuoOsc_outputDevices);

		VuoOsc_deviceQueue = dispatch_queue_create("org.vuo.osc.device", NULL);

		CFNetServiceClientContext context;
		context.version = 0;
		context.info = NULL;
		context.retain = NULL;
		context.release = NULL;
		context.copyDescription = NULL;

		VuoOsc_browser = CFNetServiceBrowserCreate(NULL, VuoOsc_deviceCallback, &context);
		if (!VuoOsc_browser)
		{
			VUserLog("Error: Failed to create browser.");
			return;
		}

		CFNetServiceBrowserScheduleWithRunLoop(VuoOsc_browser, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		CFStreamError error;
		if (!CFNetServiceBrowserSearchForServices(VuoOsc_browser, CFSTR(""), CFSTR("_osc._udp"), &error))
			VUserLog("Error: Failed to activate browser (%ld:%d).", error.domain, error.error);

	}
}

/**
 * Indicates that the caller no longer needs notifications about OSC devices.
 *
 * @threadAny
 */
void VuoOsc_disuse(void)
{
	if (VuoOsc_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoOsc_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoOsc_useCount, 1) == 0)
	{
		CFStreamError error;
		error.domain = kCFStreamErrorDomainCustom;
		error.error = 0;
		CFNetServiceBrowserStopSearch(VuoOsc_browser, &error);

		CFNetServiceBrowserUnscheduleFromRunLoop(VuoOsc_browser, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		CFNetServiceBrowserInvalidate(VuoOsc_browser);

		CFRelease(VuoOsc_browser);

		dispatch_sync(VuoOsc_deviceQueue, ^{});
		dispatch_release(VuoOsc_deviceQueue);

		VuoRelease(VuoOsc_inputDevices);
		VuoRelease(VuoOsc_outputDevices);
	}
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known OSC devices changes.
 *
 * Call `VuoOsc_use()` before calling this.
 *
 * @threadAny
 */
void VuoOsc_addDevicesChangedTriggers(
		VuoOutputTrigger(inputDevices,  VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
)
{
	if (inputDevices)
	{
		VuoOsc_inputDeviceCallbacks.addTrigger(inputDevices);
		inputDevices(VuoOsc_getInputDeviceList());
	}

	if (outputDevices)
	{
		VuoOsc_outputDeviceCallbacks.addTrigger(outputDevices);
		outputDevices(VuoOsc_getOutputDeviceList());
	}
}

/**
 * Removes a trigger callback previously added by @ref VuoOsc_addDevicesChangedTriggers.
 *
 * @threadAny
 */
void VuoOsc_removeDevicesChangedTriggers
(
		VuoOutputTrigger(inputDevices,  VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
)
{
	if (inputDevices)
		VuoOsc_inputDeviceCallbacks.removeTrigger(inputDevices);

	if (outputDevices)
		VuoOsc_outputDeviceCallbacks.removeTrigger(outputDevices);
}
