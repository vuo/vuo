/**
 * @file
 * VuoWindowOpenGLInternal interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoDisplayRefresh.h"

#import <AppKit/AppKit.h>


@class VuoWindowOpenGLInternal;

/**
 * Helper for @c VuoWindowOpenGLInternal.
 */
@interface VuoWindowOpenGLView : NSOpenGLView
{
	void (*initCallback)(VuoGlContext glContext, void *);  ///< Initializes the OpenGL context.
	void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int width, unsigned int height);  ///< Updates the OpenGL context when the view is resized.
	void (*drawCallback)(VuoGlContext glContext, void *);  ///< Draws onto the OpenGL context.
	void *drawContext;  ///< Argument to pass to callbacks (e.g. node instance data).

	bool callerRequestedRedraw;	///< True if an external caller (i.e., not resize or toggleFullScreen) requested that the GL view be redrawn.
	VuoDisplayRefresh displayRefresh;	///< Handles redrawing at the display refresh rate.  Only draws if @c callerRequestedRedraw.

	dispatch_queue_t drawQueue;	///< Queue to ensure that multiple threads don't attempt to draw to the same window simultaneously.
	bool togglingFullScreen;	///< If true, code that requires drawQueue will be skipped (to avoid deadlock).

	void (*movedMouseTo)(VuoPoint2d);	///< Trigger function, fired when the mouse moves.
	void (*scrolledMouse)(VuoPoint2d);	///< Trigger function, fired when the mouse is scrolled.
	void (*usedMouseButton)(VuoMouseButtonAction);	///< Trigger function, fired when a mouse button is used.
	dispatch_queue_t clickQueue;	///< Queue for processing mouse button clicks.
	int pendingClickCount;	///< The number of clicks elapsed so far.
}

- (id)initWithFrame:(NSRect)frame
	   initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
	 resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
	   drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
		drawContext:(void *)_drawContext;

- (void)enableTriggersWithMovedMouseTo:(void (*)(VuoPoint2d))movedMouseTo
						 scrolledMouse:(void (*)(VuoPoint2d))scrolledMouse
					   usedMouseButton:(void (*)(VuoMouseButtonAction))usedMouseButton;
- (void)disableTriggers;

- (void)toggleFullScreen;
- (void)scheduleRedraw;

@property(retain) VuoWindowOpenGLInternal *glWindow;  ///< The parent window; allows the view to access it while full-screen.
@property(retain) NSOpenGLContext *windowedGlContext;  ///< The OpenGL context from Vuo's context pool; allows the windw to access it while the view is full-screen.

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
}

@property(retain) VuoWindowOpenGLView *glView;  ///< The OpenGL view inside this window.

- (id)initWithInitCallback:(void (*)(VuoGlContext glContext, void *))initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))drawCallback
			   drawContext:(void *)drawContext;
- (void)enableTriggersWithMovedMouseTo:(void (*)(VuoPoint2d))movedMouseTo
						 scrolledMouse:(void (*)(VuoPoint2d))scrolledMouse
					   usedMouseButton:(void (*)(VuoMouseButtonAction))usedMouseButton;
- (void)disableTriggers;
- (void)scheduleRedraw;
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)toggleFullScreen;

@end
