/**
 * @file
 * VuoScreenCommon implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"
#include "VuoScreenCommon.h"

#include <IOKit/graphics/IOGraphicsLib.h>
#include <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoScreenCommon",
					  "dependencies" : [
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
	[deviceInfo autorelease];
	NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];
	if ([localizedNames count] > 0)
		return [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];

	return @"";
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
 * Returns a platform screen (`NSScreen`) matching `screen`.
 */
void *VuoScreen_getNSScreen(VuoScreen screen)
{
	if (screen.id == -1 && strlen(screen.name) == 0)
		// Default screen
		return nil;
	else if (screen.id == -1)
	{
		// The first screen whose name contains screen.name
		for (NSScreen *nsscreen in [NSScreen screens])
			if ([[nsscreen deviceName] rangeOfString:[NSString stringWithUTF8String:screen.name]].location != NSNotFound)
				return nsscreen;
	}
	else
	{
		// The screen whose ID matches screen.id
		for (NSScreen *nsscreen in [NSScreen screens])
			if ([nsscreen deviceId] == screen.id)
				return nsscreen;
	}

	return nil;
}

/**
 * Returns a list of the currently-active screens.
 */
VuoList_VuoScreen VuoScreen_getList(void)
{
	VuoList_VuoScreen screens = VuoListCreate_VuoScreen();

	for (NSScreen *screen in [NSScreen screens])
	{
		NSRect frame = [screen frame];

		NSDictionary *deviceDescription = [screen deviceDescription];
		NSSize resolution = [[deviceDescription objectForKey:NSDeviceResolution] sizeValue];

		VuoScreen s = {
			[screen deviceId],
			VuoText_make([[screen deviceName] UTF8String]),
			VuoPoint2d_make(frame.origin.x, frame.origin.y),
			frame.size.width,
			frame.size.height,
			resolution.width,
			resolution.height
		};
		VuoListAppendValue_VuoScreen(screens, s);
	}

	return screens;
}
