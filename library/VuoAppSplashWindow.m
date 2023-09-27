/**
 * @file
 * VuoAppSplashWindow implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

#import "VuoAppSplashWindow.h"

#import "VuoApp.h"
#import "VuoAppSplashView.h"

/// The splash window instance.  Non-nil when fading in, holding, and fading out;  nil when invisible.
NSWindow *VuoApp_splashWindow = nil;
/// The width of the splash window.
const double VuoApp_splashWindowWidth  = 640;
/// The height of the splash window.
const double VuoApp_splashWindowHeight = 384;

/**
 * Shows the "Powered by Vuo" splash window.
 */
void VuoApp_showSplashWindow(void)
{
	VuoApp_splashWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, VuoApp_splashWindowWidth, VuoApp_splashWindowHeight)
																styleMask:NSWindowStyleMaskBorderless
																  backing:NSBackingStoreBuffered
																	defer:NO];
	VuoApp_splashWindow.animationBehavior = NSWindowAnimationBehaviorNone;
	VuoApp_splashWindow.hasShadow = YES;

	NSView *view = [VuoAppSplashView new];
	VuoApp_splashWindow.contentView = view;
	[view release];

	[VuoApp_splashWindow center];
	VuoApp_splashWindow.alphaValue = 0;
	[VuoApp_splashWindow makeKeyAndOrderFront:nil];

	[NSCursor.pointingHandCursor push];


	// Fade in.
	// Can't simply use `+[NSAnimationContext runAnimationGroup:]` here,
	// since it has no effect on OS X 10.10 and macOS 10.12,
	// possibly because we're processing events ourselves below
	// rather than using the main event loop.
	// (But `-[NSRunLoop.mainRunLoop runUntilDate:…]` and `-[NSRunLoop.currentRunLoop runUntilDate:…]` don't work either.)

	double splashFadeSeconds = VuoApp_windowFadeSeconds * 2;
//	[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context){
//		context.duration = splashFadeSeconds;
//		VuoApp_splashWindow.animator.alphaValue = 0;
//	} completionHandler:^{
//		VLog("finished fading in: %g",VuoApp_splashWindow.alphaValue);
//	}];
	double t0 = VuoLogGetElapsedTime();
	do
	{
		NSEvent *event = [VuoApp_splashWindow nextEventMatchingMask:NSEventMaskAny
								untilDate:[NSDate dateWithTimeIntervalSinceNow:1./60]
								inMode:NSDefaultRunLoopMode
								dequeue:YES];
		[VuoApp_splashWindow sendEvent:event];
		VuoApp_splashWindow.alphaValue = (VuoLogGetElapsedTime() - t0) / splashFadeSeconds;
	} while (t0 + splashFadeSeconds > VuoLogGetElapsedTime());


	// Hold.
	// Can't simply use `-[NSRunLoop runUntilDate:]` here,
	// since that doesn't immediately process the `-[NSWorkspace openURL:]` message.
	double splashHoldSeconds = 1;
	t0 = VuoLogGetElapsedTime();
	do
	{
		NSEvent *event = [VuoApp_splashWindow nextEventMatchingMask:NSEventMaskAny
								untilDate:[NSDate dateWithTimeIntervalSinceNow:splashHoldSeconds]
								inMode:NSDefaultRunLoopMode
								dequeue:YES];
		[VuoApp_splashWindow sendEvent:event];
	} while (t0 + splashHoldSeconds > VuoLogGetElapsedTime());


	// Fade out.
	// `+[NSAnimationContext runAnimationGroup:]` does work here on OS X 10.10,
	// possibly because we're using the main event loop (see below).
	[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context){
		context.duration = splashFadeSeconds;
		VuoApp_splashWindow.animator.alphaValue = 0;
	} completionHandler:^{
		[VuoApp_splashWindow close];
		VuoApp_splashWindow = nil;
	}];

	[NSCursor.pointingHandCursor pop];

	// Return immediately and let the main event loop handle the fadeout
	// (so the composition's windows can simultaneously fade in).
}
