/**
 * @file
 * VuoScreenCommon implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"
#include "VuoScreenCommon.h"
#include "VuoTriggerSet.hh"
#include "VuoOsStatus.h"
#include "VuoPnpId.h"
#include "VuoApp.h"

#include "VuoMacOSSDKWorkaround.h"
#include <IOKit/graphics/IOGraphicsLib.h>
#include <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoScreenCommon",
					  "dependencies" : [
						  "VuoApp",
						  "VuoOsStatus",
						  "VuoPnpId",
						  "VuoScreen",
						  "VuoList_VuoScreen",
						  "IOKit.framework",
						  "AppKit.framework"
					  ]
				 });
#endif

static VuoTriggerSet<VuoList_VuoScreen> VuoScreen_callbacks;  ///< Trigger functions to call when the list of screens changes.
unsigned int VuoScreen_useCount = 0;  ///< Process-wide count of callers (typically node instances) interested in notifications about screens.

/**
 * Some convenience methods for fetching information about an NSScreen.
 */
@interface NSScreen (VuoAdditions)
- (NSInteger)deviceId;
@end

/**
 * Returns a descriptive string identifying the device.  E.g., "Color LCD" or "LA2405".
 */
static VuoText VuoScreen_getName(CGDirectDisplayID displayID)
{
	NSString *modelName        = nil;
	NSString *manufacturerName = nil;
	NSString *transport        = nil;
	NSString *serialString     = nil;
	uint32_t serialNumber      = 0;
	uint8_t  manufacturedWeek  = 0;
	uint16_t manufacturedYear  = 0;


	// x86_64 supports getting extended display info via `IODisplayCreateInfoDictionary`, but arm64 doesn't support that.
	// https://b33p.net/kosada/vuo/vuo/-/issues/18596

#if __x86_64__

	// Deprecated on 10.9, but there's no alternative.
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	io_service_t port = CGDisplayIOServicePort(displayID);
	#pragma clang diagnostic pop

	NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(port, kIODisplayOnlyPreferredName);

	NSData *edid = [deviceInfo objectForKey:@kIODisplayEDIDKey];
	NSString *displayLocation = [deviceInfo objectForKey:@kIODisplayLocationKey];
	if (edid)
	{
		const unsigned char *bytes = (const unsigned char *)[edid bytes];

		uint16_t manufacturerId = (bytes[8] << 8) + bytes[9];
		char *manufacturer = VuoPnpId_getString(manufacturerId);
		manufacturerName = [NSString stringWithUTF8String:manufacturer];
		free(manufacturer);

		serialNumber = (bytes[15] << 24) + (bytes[14] << 16) + (bytes[13] << 8) + bytes[12];

		manufacturedWeek = bytes[16];
		manufacturedYear = bytes[17] + 1990;
	}
	else
	{
		// If no EDID manufacturer is provided, inspect the device path for some known unique keys.
		if ([displayLocation rangeOfString:@"SRXDisplayCard" options:0].location != NSNotFound)
			manufacturerName = @"Splashtop XDisplay";
		else if ([displayLocation rangeOfString:@"info_ennowelbers_proxyframebuffer_fbuffer" options:0].location != NSNotFound)
			manufacturerName = @"GoodDual Display";
	}

	// Yam Display reports "DIS" as its EDID manufacturer.
	// I assume they meant that as short for "Display"
	// but it's actually the apparently-unrelated company "Diseda S.A.", so ignore it.
	if ([displayLocation rangeOfString:@"com_yamstu_YamDisplayDriver" options:0].location != NSNotFound)
		manufacturerName = nil;


	[deviceInfo autorelease];
	NSDictionary *localizedNames = [deviceInfo objectForKey:@kDisplayProductName];
	if ([localizedNames count] > 0)
		modelName = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];

#elif __arm64__

	CFMutableDictionaryRef matching = IOServiceMatching("IOMobileFramebuffer");

	io_iterator_t it  = 0;
	kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &it);
	if (ret == noErr)
	{
		while (io_object_t ioo = IOIteratorNext(it))
		{
			NSDictionary *properties = nil;
			kern_return_t ret        = IORegistryEntryCreateCFProperties(ioo, (CFMutableDictionaryRef *)&properties, NULL, 0);
			if (ret == noErr)
			{
				uint32_t ioregSerialNumber = ((NSNumber *)properties[@"DisplayAttributes"][@"ProductAttributes"][@"SerialNumber"]).unsignedLongValue;
				uint32_t ioregVendorNumber = ((NSNumber *)properties[@"DisplayAttributes"][@"ProductAttributes"][@"LegacyManufacturerID"]).unsignedLongValue;
				uint32_t ioregModelNumber  = ((NSNumber *)properties[@"DisplayAttributes"][@"ProductAttributes"][@"ProductID"]).unsignedLongValue;
				if (ioregSerialNumber == CGDisplaySerialNumber(displayID)
					&& ioregVendorNumber == CGDisplayVendorNumber(displayID)
					&& ioregModelNumber == CGDisplayModelNumber(displayID))
				{
					modelName        = [[properties[@"DisplayAttributes"][@"ProductAttributes"][@"ProductName"] retain] autorelease];
					manufacturerName = [[properties[@"DisplayAttributes"][@"ProductAttributes"][@"ManufacturerID"] retain] autorelease];
					transport        = [[properties[@"Transport"][@"Downstream"] retain] autorelease];
					serialString     = [[properties[@"DisplayAttributes"][@"ProductAttributes"][@"AlphanumericSerialNumber"] retain] autorelease];
					serialNumber     = ioregSerialNumber;
					manufacturedWeek = ((NSNumber *)properties[@"DisplayAttributes"][@"ProductAttributes"][@"WeekOfManufacture"]).unsignedIntValue;
					manufacturedYear = ((NSNumber *)properties[@"DisplayAttributes"][@"ProductAttributes"][@"YearOfManufacture"]).unsignedIntValue;
				}
			}
			[properties release];
			IOObjectRelease(ioo);
		}
		IOObjectRelease(it);
	}

#else
#error "Unsupported CPU architecture"
#endif


	NSString *name = nil;
	if (modelName.length > 0)
	{
		if (manufacturerName.length > 0)
			name = [NSString stringWithFormat:@"%@: %@", manufacturerName, modelName];
		else
			name = modelName;
	}

	if (name.length == 0)
	{
		if (manufacturerName)
			name = manufacturerName;
		else
		{
			name = @"?";

			// As a last resort, upgrade to an NSApplication-app and check whether `NSScreen` reports anything.
			VuoApp_init(false);
			for (NSScreen *nsscreen in [NSScreen screens])
				if (nsscreen.deviceId == displayID)
					name = [nsscreen.localizedName autorelease];
		}
	}


	NSMutableArray *parentheticalComponents = [NSMutableArray new];

	if (transport.length > 0)
		[parentheticalComponents addObject:transport];

	if (serialString.length > 0)
		[parentheticalComponents addObject:serialString];
	else if (serialNumber)
		[parentheticalComponents addObject:[NSString stringWithFormat:@"%d", serialNumber]];

	if (manufacturedYear)
		[parentheticalComponents addObject:[NSString stringWithFormat:@"%d-W%02d", manufacturedYear, manufacturedWeek]];


	NSString *compositeName = name;
	NSString *parenthetical = [parentheticalComponents componentsJoinedByString:@", "];
	[parentheticalComponents release];
	if (parenthetical.length > 0)
		compositeName = [name stringByAppendingFormat:@" (%@)", parenthetical];

	return VuoText_make([compositeName UTF8String]);
}

@implementation NSScreen (VuoAdditions)
/**
 * Returns the screen's CGDirectDisplayID.
 */
- (NSInteger)deviceId
{
	NSDictionary *deviceDescription = [self deviceDescription];
	return [[deviceDescription objectForKey:@"NSScreenNumber"] integerValue];
}
@end

/**
 * Returns a platform screen (`NSScreen`) matching `screen`.
 */
void *VuoScreen_getNSScreen(VuoScreen screen)
{
	VuoScreen realizedScreen;
	bool realized = VuoScreen_realize(screen, &realizedScreen);
	if (!realized)
		return NULL;
	else
	{
		VuoScreen_retain(realizedScreen);
		for (NSScreen *nsscreen in [NSScreen screens])
			if ([nsscreen deviceId] == realizedScreen.id)
			{
				VuoScreen_release(realizedScreen);
				return nsscreen;
			}
		VuoScreen_release(realizedScreen);
	}

	return NULL;
}

/**
 * Fills in a VuoScreen structure describing an CGDirectDisplayID.
 */
VuoScreen VuoScreen_makeFromCGDirectDisplay(CGDirectDisplayID display)
{
	CGRect bounds = CGDisplayBounds(display);
	CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display);
	int backingScaleFactor = CGDisplayModeGetPixelWidth(mode) / bounds.size.width;
	int fakeDPI = backingScaleFactor * 72;
	CGDisplayModeRelease(mode);

	return (VuoScreen){
		VuoScreenType_MatchId,
		display,
		CGDisplayIDToOpenGLDisplayMask(display),
		VuoScreen_getName(display),
		true,
		VuoPoint2d_make(bounds.origin.x, bounds.origin.y),
		(VuoInteger)bounds.size.width,
		(VuoInteger)bounds.size.height,
		fakeDPI,
		fakeDPI
	};
}

/**
 * Fills in a VuoScreen structure describing an NSScreen.
 */
VuoScreen VuoScreen_makeFromNSScreen(void *vscreen)
{
	NSScreen *screen = (NSScreen *)vscreen;
	return VuoScreen_makeFromCGDirectDisplay(screen.deviceId);
}

/**
 * Returns a list of the currently-active screens.
 */
VuoList_VuoScreen VuoScreen_getList(void)
{
	VuoList_VuoScreen screens = VuoListCreate_VuoScreen();

	// +[NSScreen screens] causes formerly-headless apps to begin bouncing endlessly in the dock,
	// so use the CoreGraphics functions instead.
	CGDirectDisplayID displays[256];
	uint32_t displayCount = 0;
	CGError e = CGGetOnlineDisplayList(256, displays, &displayCount);
	if (e != kCGErrorSuccess)
	{
		char *errorStr = VuoOsStatus_getText(e);
		VUserLog("Error: Couldn't get display info: %s", errorStr);
		free(errorStr);
		return screens;
	}

	for (int i = 0; i < displayCount; ++i)
		VuoListAppendValue_VuoScreen(screens, VuoScreen_makeFromCGDirectDisplay(displays[i]));

	return screens;
}

/**
 * Returns the screen with the window that has keyboard focus.
 */
VuoScreen VuoScreen_getActive(void)
{
	// +[NSScreen mainScreen] causes formerly-headless apps to begin bouncing endlessly in the dock.
	// There's no CoreGraphics equivalent (CGMainDisplayID() returns the screen with the menu bar, not the focused screen).
	VuoApp_init(false);

	return VuoScreen_makeFromNSScreen([NSScreen mainScreen]);
}

/**
 * Returns the primary screen.
 *
 * (In System Preferences > Displays > Arrangement, the primary screen is the one with the menu bar.)
 */
VuoScreen VuoScreen_getPrimary(void)
{
	return VuoScreen_makeFromCGDirectDisplay(CGMainDisplayID());
}

/**
 * Returns a secondary (non-primary) screen if present, or a null screen otherwise.
 *
 * @version200Changed{Now returns a null screen if there is no secondary screen (instead of returning the primary screen.}
 */
VuoScreen VuoScreen_getSecondary(void)
{
	CGDirectDisplayID displays[256];
	uint32_t displayCount = 0;
	CGError e = CGGetOnlineDisplayList(256, displays, &displayCount);
	if (e != kCGErrorSuccess)
	{
		char *errorStr = VuoOsStatus_getText(e);
		VUserLog("Error: Couldn't get display info: %s", errorStr);
		free(errorStr);
		return VuoScreen_getPrimary();
	}

	if (displayCount > 1)
		return VuoScreen_makeFromCGDirectDisplay(displays[1]);

	return VuoScreen_makeFromName(nullptr);
}

/// If the flag is set, logs it.
#define VuoScreen_reconfigurationCallback_showFlag(a) \
	if (flags & a) VUserLog("    %s", #a);

/**
 * Invoked by macOS Quartz Display Services.
 */
void VuoScreen_reconfigurationCallback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo)
{
	// Ignore the "something's about to happen" notification.
	if (flags == kCGDisplayBeginConfigurationFlag)
		return;

	{
		VUserLog("display %x, flags %x", display, flags);
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayBeginConfigurationFlag );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayMovedFlag              );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplaySetMainFlag            );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplaySetModeFlag            );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayAddFlag                );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayRemoveFlag             );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayEnabledFlag            );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayDisabledFlag           );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayMirrorFlag             );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayUnMirrorFlag           );
		VuoScreen_reconfigurationCallback_showFlag(kCGDisplayDesktopShapeChangedFlag);
	}

	// If multiple displays are attached, this callback is invoked multiple times.
	// Coalesce into a single Vuo event.
	const double notificationDelaySeconds = 0.25;
	static dispatch_source_t timer = 0;
	if (timer)
		// Push the timer back.
		dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, notificationDelaySeconds*NSEC_PER_SEC), DISPATCH_TIME_FOREVER, notificationDelaySeconds*NSEC_PER_SEC/10);
	else
	{
		timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
		dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, notificationDelaySeconds*NSEC_PER_SEC), DISPATCH_TIME_FOREVER, notificationDelaySeconds*NSEC_PER_SEC/10);
		dispatch_source_set_event_handler(timer, ^{
			VuoScreen_callbacks.fire(VuoScreen_getList());
			dispatch_source_cancel(timer);
		});
		dispatch_source_set_cancel_handler(timer, ^{
			dispatch_release(timer);
			timer = 0;
		});
		dispatch_resume(timer);
	}
}

/**
 * Indicates that the caller needs to get notifications about screens.
 *
 * @threadAny
 * @version200New
 */
void VuoScreen_use(void)
{
	if (__sync_add_and_fetch(&VuoScreen_useCount, 1) == 1)
	{
		// Our process only receives CGDisplay notifications while running in app mode.
		VuoApp_init(false);

		CGDisplayRegisterReconfigurationCallback(VuoScreen_reconfigurationCallback, NULL);
	}
}

/**
 * Indicates that the caller no longer needs notifications about screens.
 *
 * @threadAny
 * @version200New
 */
void VuoScreen_disuse(void)
{
	if (VuoScreen_useCount <= 0)
	{
		VUserLog("Error: Unbalanced VuoScreen_use() / _disuse() calls.");
		return;
	}

	if (__sync_sub_and_fetch(&VuoScreen_useCount, 1) == 0)
		CGDisplayRemoveReconfigurationCallback(VuoScreen_reconfigurationCallback, NULL);
}

/**
 * Adds a trigger callback, to be invoked whenever the list of known screens changes.
 *
 * Call `VuoScreen_use()` before calling this.
 *
 * @threadAny
 * @version200New
 */
void VuoScreen_addDevicesChangedTriggers(VuoOutputTrigger(screens, VuoList_VuoScreen))
{
	VuoScreen_callbacks.addTrigger(screens);
	screens(VuoScreen_getList());
}

/**
 * Removes a trigger callback previously added by @ref VuoScreen_addDevicesChangedTriggers.
 *
 * @threadAny
 * @version200New
 */
void VuoScreen_removeDevicesChangedTriggers(VuoOutputTrigger(screens, VuoList_VuoScreen))
{
	VuoScreen_callbacks.removeTrigger(screens);
}
