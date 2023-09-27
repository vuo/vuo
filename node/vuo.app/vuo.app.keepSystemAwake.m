/**
 * @file
 * vuo.app.keepSystemAwake node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#import <Foundation/Foundation.h>

#import "VuoApp.h"

VuoModuleMetadata({
	"title": "Keep System Awake",
	"keywords": [
		"disable", "prevent", "sleep",
		"stay", "wake",
		"idle", "busy",
		"display", "screen", "computer",
		"deactivate", "screensaver",
		"energy saver", "poweroff", "turn off",
		"caffeine", "caffeinated",
	],
	"version": "1.0.0",
	"dependencies": [
		"Foundation.framework",
	],
	"node": {
		"exampleCompositions": [ ],
	},
});

id *nodeInstanceInit()
{
	id *activityToken = (id *)calloc(1, sizeof(id));
	VuoRegister(activityToken, free);
	return activityToken;
}

void nodeInstanceEvent(
	VuoInstanceData(id *) activityToken,
	VuoInputData(VuoBoolean, {"default":true}) keepSystemAwake,
	VuoInputEvent({"eventBlocking":"none","data":"keepSystemAwake"}) keepSystemAwakeEvent)
{
	if (keepSystemAwake && !**activityToken)
		VuoApp_executeOnMainThread(^{
			**activityToken = [NSProcessInfo.processInfo beginActivityWithOptions:NSActivityIdleDisplaySleepDisabled
				reason: @"This Vuo composition uses the \"Keep System Awake\" node."];
			[**activityToken retain];
		});
	else if (!keepSystemAwake && **activityToken)
		VuoApp_executeOnMainThread(^{
			[**activityToken release];
			**activityToken = nil;
		});
}

void nodeInstanceFini(VuoInstanceData(id *) activityToken)
{
	if (**activityToken)
		VuoApp_executeOnMainThread(^{
			[**activityToken release];
		});
}
