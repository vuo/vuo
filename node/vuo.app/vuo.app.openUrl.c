/**
 * @file
 * vuo.app.openUrl node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
		VuoInputData(VuoText, {"name":"URL", "default":"https://vuo.org"}) url,
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
			VUserLog("Error: Couldn't open '%s': Invalid URL.", normalizedUrl);
			return;
		}

		CFErrorRef error = NULL;
		CFURLRef browserAppUrl = LSCopyDefaultApplicationURLForURL(cfurl, kLSRolesAll, &error);
		if (!browserAppUrl)
		{
			CFStringRef errorCFS = CFErrorCopyDescription(error);
			CFRelease(error);
			VuoText errorText = VuoText_makeFromCFString(errorCFS);
			CFRelease(errorCFS);
			VuoRetain(errorText);
			VUserLog("Error: Couldn't open '%s': %s", normalizedUrl, errorText);
			VuoRelease(errorText);
			CFRelease(cfurl);
			return;
		}

		CFArrayRef itemsToOpen = CFArrayCreate(NULL, (const void **)&cfurl, 1, &kCFTypeArrayCallBacks);
		LSLaunchURLSpec lus = {browserAppUrl, itemsToOpen, NULL, kLSLaunchDefaults, NULL};
		OSStatus ret = LSOpenFromURLSpec(&lus, NULL);
		if (ret != noErr)
		{
			char *errStr = VuoOsStatus_getText(ret);
			VUserLog("Error: Couldn't open '%s': %s", normalizedUrl, errStr);
			free(errStr);
		}

		CFRelease(browserAppUrl);
		CFRelease(cfurl);
	}
}
