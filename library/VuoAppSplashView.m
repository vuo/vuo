/**
 * @file
 * VuoAppSplashView implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

#import "VuoAppSplashView.h"
#import "VuoAppSplashWindow.h"
#import "module.h"

#import "vuo-wordmark-teal-cropped.h"

/// The URL to open when the user clicks on the splash view, in VuoCompositionLoader and Vuo Pro mode.
NSString *windowURL = @"https://vuo.org/";
/// The URL to open when the user clicks on the splash view, in Vuo Community Edition mode.
NSString *windowURLCE = @"https://vuo.org/community-edition";
/// The simplified URL label to show on the splash view, in VuoCompositionLoader and Vuo Pro mode.
NSString *windowURLLabel = @"vuo.org";
/// The simplified URL label to show on the splash view, in Vuo Community Edition mode.
NSString *windowURLCELabel = @"vuo.org/community-edition";

@implementation VuoAppSplashView

- (instancetype)init
{
	if (self = [super init])
	{
		_borderVisible = YES;
	}
	return self;
}

- (void)drawRect:(NSRect)rect
{
	double windowWidth  = self.bounds.size.width;
	double windowHeight = self.bounds.size.height;
	bool large = windowWidth > 320;

	if (large)
		[NSColor.controlBackgroundColor setFill];
	else
		[NSColor.clearColor setFill];
	[NSColor.windowBackgroundColor setStroke];
	NSBezierPath *path = [NSBezierPath bezierPathWithRect:rect];
	path.lineWidth = 2;
	[path fill];
	if (_borderVisible)
		[path stroke];


	NSData *logoData = [NSData dataWithBytesNoCopy:vuo_wordmark_teal_cropped_pdf
											length:vuo_wordmark_teal_cropped_pdf_len
									  freeWhenDone:NO];
	NSImage *logoImage = [[NSImage alloc] initWithData:logoData];

	NSMutableParagraphStyle *textStyle = NSMutableParagraphStyle.defaultParagraphStyle.mutableCopy;
	textStyle.alignment = NSTextAlignmentCenter;
	NSMutableDictionary *textAttributes = [NSMutableDictionary dictionaryWithObjectsAndKeys:
		[NSFont fontWithName:@"Helvetica Neue Light" size:(large ? 18 : 13)], NSFontAttributeName,
		NSColor.secondaryLabelColor, NSForegroundColorAttributeName,
		textStyle, NSParagraphStyleAttributeName,
		@2.5, NSKernAttributeName,
		nil
	];

	[@"Powered by" drawInRect:NSMakeRect(0, windowHeight*5/8, windowWidth, windowHeight/4)
			   withAttributes:textAttributes];

	// Draw the logo centered in the window.
	{
		double imageAspect = logoImage.size.width / logoImage.size.height;
		double imageScale = .5;
		[logoImage drawInRect:NSMakeRect(windowWidth/2 - windowWidth*imageScale/2,
										 windowHeight/2 - windowWidth*imageScale/imageAspect/2,
										 windowWidth*imageScale,
										 windowWidth*imageScale/imageAspect)];
	}

	NSString *urlLabel = VuoShouldShowSplashWindow() ? windowURLCELabel : windowURLLabel;
	textAttributes[NSKernAttributeName] = @0;
	textAttributes[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
	textAttributes[NSUnderlineColorAttributeName] = NSColor.tertiaryLabelColor;
	[urlLabel drawInRect:NSMakeRect(0, windowHeight*-1/16, windowWidth, windowHeight/4)
		  withAttributes:textAttributes];

	[logoImage release];
	[textStyle release];
}

- (void)mouseDown:(NSEvent *)event
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:(VuoShouldShowSplashWindow() ? windowURLCE : windowURL)]];
}

- (void)resetCursorRects
{
	[self addCursorRect:self.visibleRect cursor:NSCursor.pointingHandCursor];
}

@end
