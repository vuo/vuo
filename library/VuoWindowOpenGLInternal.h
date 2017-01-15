/**
 * @file
 * VuoWindowOpenGLInternal interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "node.h"
#include "VuoGlContext.h"
#include "VuoDisplayRefresh.h"
#include "VuoWindowRecorder.h"
#include "VuoDragEvent.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>


@class VuoWindowOpenGLInternal;

/**
 * Helper for @c VuoWindowOpenGLInternal.
 */
@interface VuoWindowOpenGLView : NSOpenGLView
{
	void (*initCallback)(VuoGlContext glContext, float backingScaleFactor, void *);  ///< Initializes the OpenGL context.
	bool initCallbackCalled;	///< Has the init callback already been called?
	void (*updateBackingCallback)(VuoGlContext glContext, void *, float backingScaleFactor);  ///< Called when the screen changes.
	void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int width, unsigned int height);  ///< Updates the OpenGL context when the view is resized.
	void (*drawCallback)(VuoGlContext glContext, void *);  ///< Draws onto the OpenGL context.
	void *drawContext;  ///< Argument to pass to callbacks (e.g. node instance data).

	bool callerRequestedRedraw;	///< True if an external caller (i.e., not resize or setFullScreen) requested that the GL view be redrawn.

	NSRect viewport;

	NSImage *circleImage;	///< The touch-circle mouse cursor.
	NSRect circleRect;		///< The bounding box of `circleImage`.
}

- (id)initWithFrame:(NSRect)frame
		 initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
updateBackingCallback:(void (*)(VuoGlContext glContext, void *, float backingScaleFactor))_updateBackingCallback
	   resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
		 drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
		  drawContext:(void *)_drawContext;

- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (BOOL)isFullScreen;
- (void)scheduleRedraw;

@property(assign) VuoWindowOpenGLInternal *glWindow;  ///< The parent window; allows the view to access it while full-screen.
@property(retain) NSOpenGLContext *windowedGlContext;  ///< The OpenGL context from Vuo's context pool; allows the windw to access it while the view is full-screen.
@property(assign) dispatch_queue_t drawQueue;	///< Queue to ensure that multiple threads don't attempt to draw to the same window simultaneously.
@property NSRect viewport;	///< The viewport in which we're rendering (it might not match the view's dimensions), relative to the parent view.  In points (not pixels).
@property bool reshapeNeeded;	///< True if the view needs to recalculate the glViewport before drawing
@property bool skipDrawRect;	///< True if drawRect should be temporarily ignored (e.g., during resizing).

@end


/**
 * A graphics window for use by Vuo node classes.
 *
 * The graphics window renders onto an OpenGL context shared with Vuo nodes, so that it can render the
 * scenes created by those nodes. A scene is rendered by calling callbacks provided by the Vuo nodes.
 */
@interface VuoWindowOpenGLInternal : NSWindow <NSWindowDelegate>
{
	BOOL callbacksEnabled;  ///< True if the window can call its trigger callbacks.

	BOOL userResizedWindow;  ///< True if the user has manually resized the window.
	BOOL programmaticallyResizingWindow;  ///< True if a programmatic resize of the window is in progress.

	NSRect contentRectWhenWindowed;
	NSUInteger styleMaskWhenWindowed;

	NSRect contentRectCached;
	float backingScaleFactorCached;
	dispatch_queue_t contentRectQueue;  ///< Synchronizes access to contentRectCached.

	void (*showedWindow)(VuoWindowReference window);  ///< Callback to invoke when the window is shown, moved, and resized.

	VuoDisplayRefresh displayRefresh;	///< Handles redrawing at the display refresh rate.  Only draws if @c callerRequestedRedraw.

	void *dragEntered;		///< A VuoTriggerSet of callbacks invoked when a Finder drag first moves into the window.
	void *dragMovedTo;		///< A VuoTriggerSet of callbacks invoked when a Finder drag moves around within the window.
	void *dragCompleted;	///< A VuoTriggerSet of callbacks invoked when a Finder drag is released over the window.
	void *dragExited;		///< A VuoTriggerSet of callbacks invoked when a Finder drag leaves the window without being released.

	NSMenu *oldMenu;	///< The host app's menu, before the window was activated.
}

@property(retain) VuoWindowOpenGLView *glView;  ///< The OpenGL view inside this window.
@property BOOL depthBuffer;  ///< Was a depth buffer requested?
@property BOOL isClosed;  ///< Has the window been closed?
@property BOOL isInMacFullScreenMode;  ///< Is Mac OS X's fullscreen mode (green arrow button in titlebar) currently active?
@property BOOL maintainsPixelSizeWhenBackingChanges;  ///< When the window is dragged between a retina and non-retina screen, should it maintain its size in points or pixels?
@property NSRect contentRectCached;  ///< The position and size of the window's content area.  In points (not pixels).
@property float backingScaleFactorCached;  ///< The number of pixels per point in the window's content area.
@property NSRect contentRectWhenWindowed;  ///< The position and size of the window's content area, prior to switching to full-screen.  In points (not pixels).
@property NSUInteger styleMaskWhenWindowed;  ///< The window's style mask, prior to switching to full-screen.
@property VuoCursor cursor;  ///< The current mouse cursor for this window.
@property(retain) NSString *titleBackup;	///< The window's title (stored since it gets cleared when switching to fullscreen mode).
@property(retain) VuoWindowRecorder *recorder;	///< nil if recording is inactive.
@property(retain) NSMenuItem *recordMenuItem;	///< The record/stop menu item.
@property(retain) NSURL *temporaryMovieURL;	///< The temporary file to which the movie is being saved.

- (id)initWithDepthBuffer:(BOOL)depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))initCallback
	 updateBackingCallback:(void (*)(VuoGlContext glContext, void *, float backingScaleFactor))updateBackingCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))drawCallback
			   drawContext:(void *)drawContext;
- (void)enableShowedWindowTrigger:(void (*)(VuoWindowReference))showedWindow requestedFrameTrigger:(void (*)(VuoReal))requestedFrame;
- (void)disableTriggers;
- (void)scheduleRedraw;
- (void)setProperties:(VuoList_VuoWindowProperty)properties;
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute;
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)unlockAspectRatio;
- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (void)stopRecording;
@end

/**
 * Methods related to dragging files from Finder.
 */
@interface VuoWindowOpenGLInternal(Drag)
- (void)initDrag;
- (void)addDragEnteredCallback:(void (*)(VuoDragEvent e))dragEnteredCallback
	dragMovedToCallback:(void (*)(VuoDragEvent e))dragMovedCallback
	dragCompletedCallback:(void (*)(VuoDragEvent e))dragCompletedCallback
	dragExitedCallback:(void (*)(VuoDragEvent e))dragExitedCallback;
- (void)removeDragEnteredCallback:(void (*)(VuoDragEvent e))dragEnteredCallback
	dragMovedToCallback:(void (*)(VuoDragEvent e))dragMovedCallback
	dragCompletedCallback:(void (*)(VuoDragEvent e))dragCompletedCallback
	dragExitedCallback:(void (*)(VuoDragEvent e))dragExitedCallback;
@end

#ifdef __cplusplus
}
#endif
