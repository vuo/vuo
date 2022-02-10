/**
 * @file
 * vuo.ui.save node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "node.h"

#import "VuoApp.h"
#import "VuoFileType.h"

#include <json-c/json.h>
#include <sys/stat.h>

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

VuoModuleMetadata({
	"title": "Display \"Save\" Window",
	"keywords": [
		"dialog", "dialogue", "popup", "picker", "chooser", "Finder",
		"write", "export", "put", "output",
	],
	"version": "1.0.0",
	"dependencies": [
		"AppKit.framework",
		"VuoApp",
	],
	"node": {
		"exampleCompositions": [ "SaveImage.vuo" ],
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
	VuoInputData(VuoText, {"default":"Save"}) title,
	VuoInputEvent({"eventBlocking":"wall","data":"title"}) titleEvent,
	VuoInputData(VuoText, {"default":"Save As:"}) fieldLabel,
	VuoInputEvent({"eventBlocking":"wall","data":"fieldLabel"}) fieldLabelEvent,
	VuoInputData(VuoText, {"default":"Untitled"}) defaultFileName,
	VuoInputEvent({"eventBlocking":"wall","data":"defaultFileName"}) defaultFileNameEvent,
	VuoInputData(VuoText, {"default":"Save"}) buttonLabel,
	VuoInputEvent({"eventBlocking":"wall","data":"buttonLabel"}) buttonLabelEvent,
	VuoInputData(VuoFileType, {"default":"image","includeValues":[
		"any","image","json","movie","table","xml"]}) fileType,
	VuoInputEvent({"eventBlocking":"wall","data":"fileType"}) fileTypeEvent,
	VuoInputData(VuoText, {"name":"Default URL", "default":"~/Desktop"}) defaultURL,
	VuoInputEvent({"eventBlocking":"wall","data":"defaultURL"}) defaultURLEvent,
	VuoInputData(VuoRenderedLayers) parentWindow,
	VuoInputEvent({"eventBlocking":"wall","data":"parentWindow"}) parentWindowEvent,
	VuoOutputData(VuoText, {"name":"Selected URL"}) selectedURL,
	VuoOutputEvent({"data":"selectedURL"}) selectedURLEvent,
	VuoOutputEvent() canceled)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, parentWindow, &renderingDimensionsChanged);

	if (!show)
		return;

	VuoApp_init(false);

	VuoWindowReference w = NULL;
	(void)VuoRenderedLayers_getWindow((*context)->renderedLayers, &w);

	__block NSSavePanel *sp;
	__block NSModalResponse response;
	__block dispatch_semaphore_t dismissed;
	VuoApp_executeOnMainThread(^{
		sp = [NSSavePanel savePanel];
		[sp retain];
		sp.title                = @"";
		sp.message              = title           ? [NSString stringWithUTF8String:title]           : @"";
		sp.nameFieldLabel       = fieldLabel      ? [NSString stringWithUTF8String:fieldLabel]      : @"";
		sp.nameFieldStringValue = defaultFileName ? [NSString stringWithUTF8String:defaultFileName] : @"";
		sp.prompt               = buttonLabel     ? [NSString stringWithUTF8String:buttonLabel]     : @"";

		if (fileType == VuoFileType_AnyFile)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			// The replacement, allowedContentTypes, isn't available until macOS 11.
			sp.allowedFileTypes = nil;
#pragma clang diagnostic pop
		else
		{
			NSMutableArray *types = [NSMutableArray new];
			struct json_object *extensions = VuoFileType_getExtensions(fileType);
			int extensionCount = json_object_array_length(extensions);
			for (int i = 0; i < extensionCount; ++i)
				[types addObject:[NSString stringWithUTF8String:json_object_get_string(json_object_array_get_idx(extensions, i))]];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			// The replacement, allowedContentTypes, isn't available until macOS 11.
			sp.allowedFileTypes = types;
#pragma clang diagnostic pop
			[types release];
		}

		if (VuoText_isPopulated(defaultURL))
		{
			VuoText normalizedURL = VuoUrl_normalize(defaultURL, VuoUrlNormalize_default);
			VuoLocal(normalizedURL);
			NSString *s = [NSString stringWithUTF8String:normalizedURL];
			if (s)
			{
				sp.directoryURL = [NSURL URLWithString:s];

				// If `defaultURL` specifies a file that exists, the above line will highlight it in the dialog.
				// Make the dialog's filename field match (overriding `defaultFileName`).
				VuoText posixPath = VuoUrl_getPosixPath(normalizedURL);
				VuoLocal(posixPath);
				struct stat s;
				bool exists = (stat(posixPath, &s) == 0);
				if (exists && S_ISREG(s.st_mode))
				{
					VuoText path;
					VuoText folder;
					VuoText fileName;
					VuoText extension;
					if (VuoUrl_getFileParts(normalizedURL, &path, &folder, &fileName, &extension))
					{
						VuoLocal(path);
						VuoLocal(folder);
						VuoLocal(fileName);

						if (extension)
						{
							VuoLocal(extension);
							sp.nameFieldStringValue = [NSString stringWithFormat:@"%s.%s", fileName, extension];
						}
						else
							sp.nameFieldStringValue = [NSString stringWithUTF8String:fileName];
					}
				}
			}
		}

		if (w)
		{
			dismissed = dispatch_semaphore_create(0);
			[sp beginSheetModalForWindow:(NSWindow *)w completionHandler:^(NSModalResponse result){
				response = result;
				dispatch_semaphore_signal(dismissed);
			}];
		}
		else
			response = [sp runModal];
	});

	if (w)
	{
		dispatch_semaphore_wait(dismissed, DISPATCH_TIME_FOREVER);
		dispatch_release(dismissed);
	}

	VuoApp_executeOnMainThread(^{
		if (response == NSModalResponseOK && sp.URL)
		{
			*selectedURL = VuoText_make(sp.URL.absoluteString.UTF8String);
			*selectedURLEvent = true;
			*canceled = false;
		}
		else
		{
			*selectedURL = NULL;
			*selectedURLEvent = false;
			*canceled = true;
		}

		[sp release];
	});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
