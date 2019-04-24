/**
 * @file
 * vuo.app.openDocument node implementation.
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
					 "title" : "Open Document",
					 "keywords" : [
						 "file", "document", "read", "edit", "view",
						 "launch", "execute", "run", "start", "application",
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
		VuoInputData(VuoText, {"name":"URL", "default":"/Library/Documentation/Acknowledgements.rtf"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
		VuoInputData(VuoBoolean, {"default":true}) activate
)
{
	if (urlEvent)
	{
		VuoText normalizedUrl = VuoUrl_normalize(url, VuoUrlNormalize_default);
		VuoLocal(normalizedUrl);

		CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *)normalizedUrl, VuoText_byteCount(normalizedUrl), kCFStringEncodingUTF8, NULL);
		if (!cfurl)
		{
			VUserLog("Couldn't open '%s': Invalid URL.", normalizedUrl);
			return;
		}

		CFArrayRef itemsToOpen = CFArrayCreate(NULL, (const void **)&cfurl, 1, &kCFTypeArrayCallBacks);

		LSLaunchURLSpec us = { NULL, itemsToOpen, NULL,
							   activate ? kLSLaunchDefaults : kLSLaunchDontSwitch,
							   NULL
							 };

		OSStatus ret = LSOpenFromURLSpec(&us, NULL);
		if (ret != noErr)
		{
			char *errStr = VuoOsStatus_getText(ret);
			VUserLog("Couldn't open '%s': %s", normalizedUrl, errStr);
			free(errStr);
		}

		CFRelease(cfurl);
		CFRelease(itemsToOpen);
	}
}
