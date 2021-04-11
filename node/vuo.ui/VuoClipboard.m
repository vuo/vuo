/**
 * @file
 * VuoClipboard implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoClipboard.h"
#include "module.h"

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoClipboard",
					 "dependencies" : [
						 "AppKit.framework"
					 ]
				 });
#endif

/**
 * Read contents of Mac Pasteboard.
 */
@interface VuoClipboardReader : NSObject
@end

@implementation VuoClipboardReader

/**
 * Returns the text currently stored in the pasteboard.
 */
+ (VuoText) getClipboardContents
{
	NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
	NSData* data = [pasteBoard dataForType:NSPasteboardTypeString];
	NSString* str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	VuoText txt = VuoText_make([str UTF8String]);
	[str release];
	return txt;
}

/**
 * Set the clipboard contents for string type.
 */
+ (void) setClipboardContents:(const char*) text
{
	NSString* str = [[NSString alloc] initWithUTF8String:text];

	if([str length] < 1)
	{
		[str release];
		return;
	}

	[[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
	[str release];
}

@end

/**
 * Get the contents of the system clipboard.  This function
 * only returns text values.
 */
VuoText VuoClipboard_getContents()
{
	return [VuoClipboardReader getClipboardContents];
}

/**
 * Set the clipboard contents for string type.
 */
void VuoClipboard_setText(VuoText text)
{
	[VuoClipboardReader setClipboardContents:text];
}
