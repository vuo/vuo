/**
 * @file
 * VuoGraphicsWindow interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "module.h"
#import "VuoDragEvent.h"
#import "VuoRenderedLayers.h"
#import "VuoWindow.h"
#import "VuoWindowRecorder.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>

extern const int VuoGraphicsWindowMinSize; ///< Smallest window width and height that Cocoa can properly render.

typedef void (*VuoGraphicsWindowInitCallback)(void *userData, float backingScaleFactor);          ///< Called by @ref VuoGraphicsLayer's init method.
typedef void (*VuoGraphicsWindowUpdateBackingCallback)(void *userData, float backingScaleFactor); ///< Called when the window changes screens (including when the window first appears).
typedef void (*VuoGraphicsWindowResizeCallback)(void *userData, unsigned int, unsigned int);      ///< Called when the window is resized (including transitioning between fullscreen and windowed).
typedef VuoIoSurface (*VuoGraphicsWindowDrawCallback)(void *userData);							  ///< Called when content needs to be redrawn.
typedef void (*VuoGraphicsWindowUpdatedWindowCallback)(VuoRenderedLayers window);                 ///< Called when the window is shown, moved, or resized.
typedef void (*VuoGraphicsWindowShowedWindowCallback)(VuoWindowReference window);                 ///< Deprecated. See @ref VuoGraphicsWindowUpdatedWindowCallback.
typedef void (*VuoGraphicsWindowRequestedFrameCallback)(VuoReal time);                            ///< Called when the display is ready for the next frame.
typedef void (*VuoGraphicsWindowDragCallback)(VuoDragEvent e);                                    ///< Called when one or more files enter, exit, or are dropped onto the window.

extern dispatch_semaphore_t VuoGraphicsWindow_fullScreenTransitionSemaphore;

/**
 * A window containing a VuoGraphicsLayer.
 */
@interface VuoGraphicsWindow : NSWindow
@property(atomic) NSRect contentRectCached;           ///< The window's content area.
@property(atomic) float backingScaleFactorCached;     ///< The number of pixels per point in the window's content area.
@property(retain) NSMenu *oldMenu;                    ///< The host app's menu, before the window was activated.
@property BOOL isClosed;                              ///< Has the window been closed?
@property(atomic,assign) VuoWindowRecorder *recorder; ///< nil if recording is inactive.
@property(retain) NSURL *temporaryMovieURL;	          ///< The temporary file to which the movie is being saved.
@property bool programmaticallyResizingWindow;        ///< True if a programmatic resize of the window is in progress.
@property bool programmaticallyTransitioningFullScreen;///< True if a programmatic Mac-fullscreen transition is in progress.
@property bool userResizedWindow;                     ///< True if the user has manually resized the window.
@property BOOL isInMacFullScreenMode;                 ///< Is Mac OS X's fullscreen mode (green arrow button in titlebar) currently active?
@property VuoCursor cursor;                           ///< The current mouse cursor for this window.
@property BOOL maintainsPixelSizeWhenBackingChanges;  ///< When the window is dragged between a retina and non-retina screen, should it maintain its size in points or pixels?
@property(retain) NSString *titleBackup;              ///< The window's title (stored since it gets cleared when switching to fullscreen mode).
@property NSUInteger styleMaskWhenWindowed;           ///< The window's style mask, prior to switching to full-screen.
@property NSRect contentRectWhenWindowed;             ///< The position and size of the window's content area, prior to switching to full-screen.  In points (not pixels).
@property uint64_t compositionUid;                    ///< The composition that contains this window.

@property void *dragEntered;   ///< A VuoTriggerSet of callbacks invoked when a Finder drag first moves into the window.
@property void *dragMovedTo;   ///< A VuoTriggerSet of callbacks invoked when a Finder drag moves around within the window.
@property void *dragCompleted; ///< A VuoTriggerSet of callbacks invoked when a Finder drag is released over the window.
@property void *dragExited;    ///< A VuoTriggerSet of callbacks invoked when a Finder drag leaves the window without being released.

@property(atomic) VuoGraphicsWindowUpdatedWindowCallback updatedWindow;   ///< Callback to invoke when the window is shown, moved, and resized.
@property(atomic) VuoGraphicsWindowShowedWindowCallback showedWindow;     ///< Deprecated. See @ref updatedWindow.
@property(atomic) VuoGraphicsWindowRequestedFrameCallback requestedFrame; ///< Callback to invoke when the display is ready for the next frame.

- (instancetype)initWithInitCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData;
- (void)draw;
- (bool)isFullScreen;
- (void)finishFullScreenTransition;

- (void)enableUpdatedWindowTrigger:(VuoGraphicsWindowUpdatedWindowCallback)updatedWindow;
- (void)enableShowedWindowTrigger:(VuoGraphicsWindowShowedWindowCallback)showedWindow requestedFrameTrigger:(VuoGraphicsWindowRequestedFrameCallback)requestedFrame;
- (void)disableTriggers;

- (void)setProperties:(VuoList_VuoWindowProperty)properties;

- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)unlockAspectRatio;

@end
