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
	void (*switchContextCallback)(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *);  ///< Allows the caller to free resources on the old GL context and initialize the new GL context.
	void (*drawCallback)(VuoGlContext glContext, void *);  ///< Draws onto the OpenGL context.
	void *drawContext;  ///< Argument to pass to callbacks (e.g. node instance data).

	bool callerRequestedRedraw;	///< True if an external caller (i.e., not resize or toggleFullScreen) requested that the GL view be redrawn.
	VuoDisplayRefresh displayRefresh;	///< Handles redrawing at the display refresh rate.  Only draws if @c callerRequestedRedraw.

	bool togglingFullScreen;	///< If true, code that requires drawQueue will be skipped (to avoid deadlock).
}

- (id)initWithFrame:(NSRect)frame
		 initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
	   resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
switchContextCallback:(void (*)(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *))switchContextCallback
		 drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
		  drawContext:(void *)_drawContext;

- (void)enableTriggers;
- (void)disableTriggers;

- (void)toggleFullScreen;
- (void)scheduleRedraw;

@property(retain) VuoWindowOpenGLInternal *glWindow;  ///< The parent window; allows the view to access it while full-screen.
@property(retain) NSOpenGLContext *windowedGlContext;  ///< The OpenGL context from Vuo's context pool; allows the windw to access it while the view is full-screen.
@property dispatch_queue_t drawQueue;	///< Queue to ensure that multiple threads don't attempt to draw to the same window simultaneously.

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
@property BOOL depthBuffer;  ///< Was a depth buffer requested?

- (id)initWithDepthBuffer:(BOOL)depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, void *))initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))resizeCallback
	 switchContextCallback:(void (*)(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *))switchContextCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))drawCallback
			   drawContext:(void *)drawContext;
- (void)enableTriggers;
- (void)disableTriggers;
- (void)scheduleRedraw;
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute;
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)toggleFullScreen;
- (void)getWidth:(unsigned int *)pixelsWide height:(unsigned int *)pixelsHigh;

@end
