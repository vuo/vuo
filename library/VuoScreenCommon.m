/**
 * @file
 * VuoScreenCommon implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"
#include "VuoScreenCommon.h"
#include "VuoPnpId.h"

#include <IOKit/graphics/IOGraphicsLib.h>
#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoScreenCommon",
					  "dependencies" : [
						  "VuoPnpId",
						  "VuoScreen",
						  "VuoList_VuoScreen",
						  "IOKit.framework",
						  "AppKit.framework"
					  ]
				 });
#endif

/**
 * Some convenience methods for fetching information about an NSScreen.
 */
@interface NSScreen (VuoAdditions)
- (NSString *)deviceName;
- (NSInteger)deviceId;
@end

@implementation NSScreen (VuoAdditions)
/**
 * Returns a descriptive string identifying the device.  E.g., "Color LCD" or "LA2405".
 */
- (NSString *)deviceName
{
	CGDirectDisplayID displayID = [self deviceId];

	// Deprecated on 10.9, but there's no alternative.
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	io_service_t port = CGDisplayIOServicePort(displayID);
	#pragma clang diagnostic pop

	NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(port, kIODisplayOnlyPreferredName);

	NSData *edid = [deviceInfo objectForKey:@kIODisplayEDIDKey];
	NSString *displayLocation = [deviceInfo objectForKey:@kIODisplayLocationKey];
	NSString *manufacturerName = nil;
	uint32_t serialNumber = 0;
	uint8_t manufacturedWeek = 0;
	uint16_t manufacturedYear = 0;
	if (edid)
	{
		const unsigned char *bytes = [edid bytes];

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
	NSString *name = nil;
	if ([localizedNames count] > 0)
	{
		NSString *modelName = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];
		if ([modelName length] > 0)
		{
			if (manufacturerName)
				name = [NSString stringWithFormat:@"%@: %@", manufacturerName, modelName];
			else
				name = modelName;
		}
	}

	if (!name)
	{
		if (manufacturerName)
			name = manufacturerName;
		else
			name = @"";
	}

	if (serialNumber && manufacturedYear)
		return [NSString stringWithFormat:@"%@ (%d, %d-W%02d)", name, serialNumber, manufacturedYear, manufacturedWeek];
	else if (serialNumber)
		return [NSString stringWithFormat:@"%@ (%d)", name, serialNumber];
	else if (manufacturedYear)
		return [NSString stringWithFormat:@"%@ (%d-W%02d)", name, manufacturedYear, manufacturedWeek];
	else
		return name;
}

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
 * Returns the primary screen.
 *
 * (In System Preferences > Displays > Arrangement, the primary screen is the one with the menu bar.)
 */
NSScreen *getPrimaryNSScreen(void)
{
	return [[NSScreen screens] objectAtIndex:0];
}

/**
 * Returns a secondary (non-primary) screen if present, or the primary screen otherwise.
 */
NSScreen *getSecondaryNSScreen(void)
{
	NSArray *screens = [NSScreen screens];
	if ([screens count] > 1)
		return [screens objectAtIndex:1];
	else
		return [screens objectAtIndex:0];
}

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
 * Fills in a VuoScreen structure describing an NSScreen.
 */
static VuoScreen VuoScreen_makeFromNSScreen(NSScreen *screen)
{
	NSRect frame = [screen frame];

	NSDictionary *deviceDescription = [screen deviceDescription];
	NSSize resolution = [[deviceDescription objectForKey:NSDeviceResolution] sizeValue];

	return (VuoScreen){
		VuoScreenType_MatchId,
		[screen deviceId],
		VuoText_make([[screen deviceName] UTF8String]),
		true,
		VuoPoint2d_make(frame.origin.x, frame.origin.y),
		frame.size.width,
		frame.size.height,
		resolution.width,
		resolution.height
	};
}

/**
 * Returns a list of the currently-active screens.
 */
VuoList_VuoScreen VuoScreen_getList(void)
{
	VuoList_VuoScreen screens = VuoListCreate_VuoScreen();

	for (NSScreen *screen in [NSScreen screens])
		VuoListAppendValue_VuoScreen(screens, VuoScreen_makeFromNSScreen(screen));

	return screens;
}

/**
 * Returns the screen with the window that has keyboard focus.
 */
VuoScreen VuoScreen_getActive(void)
{
	return VuoScreen_makeFromNSScreen([NSScreen mainScreen]);
}

/**
 * Returns the primary screen.
 *
 * (In System Preferences > Displays > Arrangement, the primary screen is the one with the menu bar.)
 */
VuoScreen VuoScreen_getPrimary(void)
{
	return VuoScreen_makeFromNSScreen(getPrimaryNSScreen());
}

/**
 * Returns a secondary (non-primary) screen if present, or the primary screen otherwise.
 */
VuoScreen VuoScreen_getSecondary(void)
{
	return VuoScreen_makeFromNSScreen(getSecondaryNSScreen());
}
