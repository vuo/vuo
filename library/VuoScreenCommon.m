/**
 * @file
 * VuoScreenCommon implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"
#include "VuoScreenCommon.h"

#include <IOKit/graphics/IOGraphicsLib.h>
#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoScreenCommon",
					  "dependencies" : [
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
		for (NSScreen *nsscreen in [NSScreen screens])
			if ([nsscreen deviceId] == realizedScreen.id)
				return nsscreen;

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
