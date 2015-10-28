/**
 * @file
 * VuoWindowOpenGLInternal interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoDisplayRefresh.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>


@class VuoWindowOpenGLInternal;

/**
 * Helper for @c VuoWindowOpenGLInternal.
 */
@interface VuoWindowOpenGLView : NSOpenGLView
{
	void (*initCallback)(VuoGlContext glContext, void *);  ///< Initializes the OpenGL context.
	bool initCallbackCalled;	///< Has the init callback already been called?
	void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int width, unsigned int height);  ///< Updates the OpenGL context when the view is resized.
	void (*drawCallback)(VuoGlContext glContext, void *);  ///< Draws onto the OpenGL context.
	void *drawContext;  ///< Argument to pass to callbacks (e.g. node instance data).

	bool callerRequestedRedraw;	///< True if an external caller (i.e., not resize or setFullScreen) requested that the GL view be redrawn.
	VuoDisplayRefresh displayRefresh;	///< Handles redrawing at the display refresh rate.  Only draws if @c callerRequestedRedraw.

	NSRect viewport;

	NSImage *circleImage;	///< The touch-circle mouse cursor.
	NSRect circleRect;		///< The bounding box of `circleImage`.
}

- (id)initWithFrame:(NSRect)frame
		 initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
	   resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
		 drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
		  drawContext:(void *)_drawContext;

- (void)enableTriggers;
- (void)disableTriggers;

- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (BOOL)isFullScreen;
- (void)scheduleRedraw;

@property(retain) VuoWindowOpenGLInternal *glWindow;  ///< The parent window; allows the view to access it while full-screen.
@property(retain) NSOpenGLContext *windowedGlContext;  ///< The OpenGL context from Vuo's context pool; allows the windw to access it while the view is full-screen.
@property dispatch_queue_t drawQueue;	///< Queue to ensure that multiple threads don't attempt to draw to the same window simultaneously.
@property NSRect viewport;	///< The viewport in which we're rendering (it might not match the view's dimensions), relative to the parent view.  In points (not pixels).

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
}

@property(retain) VuoWindowOpenGLView *glView;  ///< The OpenGL view inside this window.
@property BOOL depthBuffer;  ///< Was a depth buffer requested?
@property NSRect contentRectWhenWindowed;  ///< The position and size of the window's content area, prior to switching to full-screen.  In points (not pixels).
@property NSUInteger styleMaskWhenWindowed;  ///< The window's style mask, prior to switching to full-screen.
@property VuoCursor cursor;  ///< The current mouse cursor for this window.

- (id)initWithDepthBuffer:(BOOL)depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, void *))initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))drawCallback
			   drawContext:(void *)drawContext;
- (void)enableTriggers;
- (void)disableTriggers;
- (void)scheduleRedraw;
- (void)setProperties:(VuoList_VuoWindowProperty)properties;
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute;
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)unlockAspectRatio;
- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (void)getWidth:(unsigned int *)pixelsWide height:(unsigned int *)pixelsHigh;

@end
