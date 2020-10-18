/**
 * @file
 * vuo.ui.open node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "node.h"

#import "VuoApp.h"
#import "VuoFileType.h"

#include <json-c/json.h>

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <AppKit/AppKit.h>

VuoModuleMetadata({
	"title": "Display \"Open\" Window",
	"keywords": [
		"dialog", "dialogue", "popup", "picker", "chooser", "Finder",
		"load", "read", "get", "input",
	],
	"version": "1.0.0",
	"dependencies": [
		"AppKit.framework",
		"VuoApp",
	],
	"node": {
		"exampleCompositions": [ "OpenImage.vuo" ],
	},
});

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	return context;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputEvent({"eventBlocking":"door"}) show,
	VuoInputData(VuoText, {"default":"Open"}) title,
	VuoInputEvent({"eventBlocking":"wall","data":"title"}) titleEvent,
	VuoInputData(VuoText, {"default":"Open"}) buttonLabel,
	VuoInputEvent({"eventBlocking":"wall","data":"buttonLabel"}) buttonLabelEvent,
	VuoInputData(VuoFileType, {"default":"image"}) fileType,
	VuoInputEvent({"eventBlocking":"wall","data":"fileType"}) fileTypeEvent,
	VuoInputData(VuoText, {"name":"Default URL", "default":"~/Desktop"}) defaultURL,
	VuoInputEvent({"eventBlocking":"wall","data":"defaultURL"}) defaultURLEvent,
	VuoInputData(VuoBoolean, {"default":false}) selectMultiple,
	VuoInputEvent({"eventBlocking":"wall","data":"selectMultiple"}) selectMultipleEvent,
	VuoInputData(VuoRenderedLayers) parentWindow,
	VuoInputEvent({"eventBlocking":"wall","data":"parentWindow"}) parentWindowEvent,
	VuoOutputData(VuoList_VuoText, {"name":"Selected URLs"}) selectedURLs,
	VuoOutputEvent({"data":"selectedURLs"}) selectedURLsEvent,
	VuoOutputEvent() canceled)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, parentWindow, &renderingDimensionsChanged);

	if (!show)
		return;

	VuoApp_init(false);

	VuoWindowReference w = NULL;
	(void)VuoRenderedLayers_getWindow((*context)->renderedLayers, &w);

	__block NSOpenPanel *op;
	__block NSModalResponse response;
	__block dispatch_semaphore_t dismissed;
	VuoApp_executeOnMainThread(^{
		op = [NSOpenPanel openPanel];
		[op retain];
		op.title   = @"";
		op.message = title       ? [NSString stringWithUTF8String:title]       : @"";
		op.prompt  = buttonLabel ? [NSString stringWithUTF8String:buttonLabel] : @"";

		if (fileType == VuoFileType_Folder)
		{
			op.canChooseFiles = NO;
			op.canChooseDirectories = YES;
		}
		else
		{
			op.canChooseFiles = YES;
			op.canChooseDirectories = NO;

			if (fileType == VuoFileType_AnyFile)
				op.allowedFileTypes = nil;
			else
			{
				NSMutableArray *types = [NSMutableArray new];
				struct json_object *extensions = VuoFileType_getExtensions(fileType);
				int extensionCount = json_object_array_length(extensions);
				for (int i = 0; i < extensionCount; ++i)
					[types addObject:[NSString stringWithUTF8String:json_object_get_string(json_object_array_get_idx(extensions, i))]];
				op.allowedFileTypes = types;
				[types release];
			}
		}

		op.allowsMultipleSelection = selectMultiple;

		if (VuoText_isPopulated(defaultURL))
		{
			VuoText normalizedURL = VuoUrl_normalize(defaultURL, VuoUrlNormalize_default);
			VuoLocal(normalizedURL);
			NSString *s = [NSString stringWithUTF8String:normalizedURL];
			if (s)
				op.directoryURL = [NSURL URLWithString:s];
		}

		if (w)
		{
			dismissed = dispatch_semaphore_create(0);
			[op beginSheetModalForWindow:(NSWindow *)w completionHandler:^(NSModalResponse result){
				response = result;
				dispatch_semaphore_signal(dismissed);
			}];
		}
		else
			response = [op runModal];
	});

	if (w)
	{
		dispatch_semaphore_wait(dismissed, DISPATCH_TIME_FOREVER);
		dispatch_release(dismissed);
	}

	VuoApp_executeOnMainThread(^{
		if (response == NSFileHandlingPanelOKButton)
		{
			*selectedURLs = VuoListCreate_VuoText();
			for (NSURL *u in op.URLs)
				VuoListAppendValue_VuoText(*selectedURLs, VuoText_make(u.absoluteString.UTF8String));
			*selectedURLsEvent = true;
			*canceled = false;
		}
		else
		{
			*selectedURLs = NULL;
			*selectedURLsEvent = false;
			*canceled = true;
		}

		[op release];
	});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
