/**
 * @file
 * VuoGraphicsWindowDelegate implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "module.h"
#import "VuoGraphicsWindowDelegate.h"
#import "VuoWindow.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsWindow",
	"dependencies" : [
		"VuoGraphicsWindow",
		"VuoWindow",
		"VuoWindowReference"
	]
});
#endif

/**
 * Private VuoGraphicsWindowDelegate data.
 */
@interface VuoGraphicsWindowDelegate ()
@property(assign) VuoGraphicsWindow *window; ///< The window to which this delegate belongs.
@end

@implementation VuoGraphicsWindowDelegate

/**
 * Creates a delegate.
 */
- (instancetype)initWithWindow:(VuoGraphicsWindow *)window
{
	if (self = [super init])
	{
		_window = window;
	}
	return self;
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)windowWillClose:(NSNotification *)notification
{
	if (_window.oldMenu)
	{
		VuoApp_setMenu(_window.oldMenu);
		_window.oldMenu = nil;
	}
	_window.isClosed = YES;
}

/**
 * Keeps track of whether the user has manually resized the window,
 * and fires the window-properties-changed callback.
 *
 * @threadMain
 */
- (void)windowDidResize:(NSNotification *)notification
{
	if (!_window.programmaticallyResizingWindow)
	{
		_window.userResizedWindow = YES;
		if (_window.showedWindow)
			_window.showedWindow(VuoWindowReference_make(_window));
	}

	NSView *gv = _window.contentView;
	_window.contentRectCached = gv.frame;
}

/**
 * Keep track of when the window enters OS X's now-mandatory 10.7+ fullscreen mode,
 * so we can apply reshape constraints, and prevent entering Vuo's custom fullscreen mode.
 */
- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
	_window.isInMacFullScreenMode = YES;
}

/**
 * If programmatically transitioning, signals the semaphore so the next transition can start.
 */
- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	if (_window.programmaticallyTransitioningFullScreen)
	{
		dispatch_semaphore_signal(VuoGraphicsWindow_fullScreenTransitionSemaphore);
		_window.programmaticallyTransitioningFullScreen = false;
		[_window finishFullScreenTransition];
	}
}

/**
 * Keep track of when the window exits OS X's now-mandatory 10.7+ fullscreen mode,
 * so we can apply reshape constraints, and prevent entering Vuo's custom fullscreen mode.
 */
- (void)windowWillExitFullScreen:(NSNotification *)notification
{
	_window.isInMacFullScreenMode = NO;
}

/**
 * If programmatically transitioning, signals the semaphore so the next transition can start.
 */
- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	if (_window.programmaticallyTransitioningFullScreen)
	{
		dispatch_semaphore_signal(VuoGraphicsWindow_fullScreenTransitionSemaphore);
		_window.programmaticallyTransitioningFullScreen = false;
		[_window finishFullScreenTransition];
	}
}

/**
 * Fires the window-properties-changed callback.
 *
 * @threadMain
 */
- (void)windowDidMove:(NSNotification *)notification
{
	if (_window.showedWindow && !_window.programmaticallyResizingWindow)
		_window.showedWindow(VuoWindowReference_make(_window));
}

@end
