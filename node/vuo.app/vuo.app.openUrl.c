/**
 * @file
 * vuo.app.openUrl node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOsStatus.h"

#include <CoreFoundation/CFBundle.h>
#include <ApplicationServices/ApplicationServices.h>

VuoModuleMetadata({
					 "title" : "Open URL in Browser",
					 "keywords" : [
						 "web", "link", "https", "html",
						 "browser", "Firefox", "Chrome", "Safari",
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoOsStatus",
						 "VuoUrl",
						 "ApplicationServices.framework",
						 "CoreFoundation.framework",
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL", "default":"http://vuo.org"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent
)
{
	if (urlEvent)
	{
		VuoText normalizedUrl = VuoUrl_normalize(url, VuoUrlNormalize_assumeHttp);
		VuoLocal(normalizedUrl);

		CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *)normalizedUrl, VuoText_byteCount(normalizedUrl), kCFStringEncodingUTF8, NULL);
		if (!cfurl)
		{
			VUserLog("Couldn't open '%s': Invalid URL.", normalizedUrl);
			return;
		}

		CFStringRef browserBundleId = LSCopyDefaultHandlerForURLScheme(CFSTR("http"));

		CFURLRef browserAppUrl;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// Can't use LSCopyApplicationURLsForBundleIdentifier yet since it's only available on Mac OS X v10.10+.
		OSStatus ret = LSFindApplicationForInfo(kLSUnknownCreator, browserBundleId, NULL, NULL, &browserAppUrl);
#pragma clang diagnostic pop
		if (ret != noErr)
		{
			char *errStr = VuoOsStatus_getText(ret);
			VUserLog("Couldn't open '%s': Couldn't find web browser '%s': %s", normalizedUrl, VuoText_makeFromCFString(browserBundleId), errStr);
			free(errStr);
		}

		CFArrayRef itemsToOpen = CFArrayCreate(NULL, (const void **)&cfurl, 1, &kCFTypeArrayCallBacks);
		LSLaunchURLSpec lus = {browserAppUrl, itemsToOpen, NULL, kLSLaunchDefaults, NULL};
		ret = LSOpenFromURLSpec(&lus, NULL);
		if (ret != noErr)
		{
			char *errStr = VuoOsStatus_getText(ret);
			VUserLog("Couldn't open '%s': %s", normalizedUrl, errStr);
			free(errStr);
		}

		CFRelease(cfurl);
	}
}
