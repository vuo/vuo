/**
 * @file
 * VuoAppLaunch implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAppLaunch.h"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoAppLaunch",
					"dependencies" : [
						"VuoUrl",
						"AppKit.framework"
					]
				 });
#endif

/**
 * Launches the `.app` at `rawUrl`, or brings it to the foreground if it's already running.
 */
void VuoAppLaunch_launch(VuoText rawUrl, bool activate)
{
	if (VuoText_isEmpty(rawUrl))
		return;

	VuoUrl normalizedUrl = VuoUrl_normalize(rawUrl, VuoUrlNormalize_forLaunching);
	NSString *urlAsString = [[NSString alloc] initWithUTF8String:normalizedUrl];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
	[urlAsString release];

	NSError *error = nil;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// The replacement, `-[NSWorkspace openApplicationAtURL]`, isn't available until macOS 11.
	NSRunningApplication *app = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:url
																			  options:(activate ? 0 : NSWorkspaceLaunchWithoutActivation)
																		configuration:@{}
																				error:&error];
#pragma clang diagnostic pop
	if (! app)
		VUserLog("Couldn't launch '%s': %s — %s", normalizedUrl,
				 [[error localizedDescription] UTF8String],
				 [[[[error userInfo] objectForKey:NSUnderlyingErrorKey] localizedDescription] UTF8String]);

	VuoRetain(normalizedUrl);
	VuoRelease(normalizedUrl);
	[url release];
}
