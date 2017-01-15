/**
 * @file
 * VuoApp implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>
#include "VuoApp.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoApp",
					"dependencies" : [
						"VuoUrl",
						"AppKit.framework"
					]
				 });
#endif

/**
 * Launches the `.app` at `rawUrl`, or brings it to the foreground if it's already running.
 */
void VuoApp_launch(VuoText rawUrl)
{
	if (!rawUrl || rawUrl[0] == 0)
		return;

	VuoUrl normalizedUrl = VuoUrl_normalize(rawUrl, false);
	NSString *urlAsString = [[NSString alloc] initWithUTF8String:normalizedUrl];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
	[urlAsString release];

	NSError *error = nil;
	NSRunningApplication *app = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:url options:0 configuration:nil error:&error];
	if (! app)
		VUserLog("Couldn't launch app '%s': %s", rawUrl, [[error localizedDescription] UTF8String]);

	VuoRetain(normalizedUrl);
	VuoRelease(normalizedUrl);
	[url release];
}
