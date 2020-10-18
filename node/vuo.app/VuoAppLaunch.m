/**
 * @file
 * VuoAppLaunch implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAppLaunch.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
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
	NSRunningApplication *app = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:url
																			  options:(activate ? 0 : NSWorkspaceLaunchWithoutActivation)
																		configuration:nil
																				error:&error];
	if (! app)
		VUserLog("Couldn't launch '%s': %s — %s", normalizedUrl,
				 [[error localizedDescription] UTF8String],
				 [[[[error userInfo] objectForKey:NSUnderlyingErrorKey] localizedDescription] UTF8String]);

	VuoRetain(normalizedUrl);
	VuoRelease(normalizedUrl);
	[url release];
}
