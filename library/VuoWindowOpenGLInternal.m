/**
 * @file
 * VuoWindowOpenGLInternal implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowOpenGLInternal.h"
#import "VuoWindowApplication.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowOpenGLInternal",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoDisplayRefresh",
						 "VuoGLContext"
					 ]
				 });
#endif


@implementation VuoWindowOpenGLView

@synthesize glWindow;
@synthesize windowedGlContext;
@synthesize drawQueue;

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (id)initWithFrame:(NSRect)frame
		   initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
		 resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
  switchContextCallback:(void (*)(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *))_switchContextCallback
		   drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			drawContext:(void *)_drawContext
{
	if (self = [super initWithFrame:frame pixelFormat:[NSOpenGLView defaultPixelFormat]])
	{
		initCallback = _initCallback;
		resizeCallback = _resizeCallback;
		switchContextCallback = _switchContextCallback;
		drawCallback = _drawCallback;
		drawContext = _drawContext;

		drawQueue = dispatch_queue_create("vuo.window.opengl.internal.draw", 0);

		displayRefresh = VuoDisplayRefresh_make(self);
		VuoRetain(displayRefresh);

		if ([self respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
		{
			typedef void (*funcType)(id receiver, SEL selector, BOOL wants);
			funcType setWantsBestResolutionOpenGLSurface = (funcType)[[self class] instanceMethodForSelector:@selector(setWantsBestResolutionOpenGLSurface:)];
			setWantsBestResolutionOpenGLSurface(self, @selector(setWantsBestResolutionOpenGLSurface:), YES);
		}
	}
	return self;
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	VuoRelease(displayRefresh);
	[super dealloc];
}

/**
 * Handles this view being added to the @c VuoWindowOpenGLInternal, or switched between that
 * and full-screen, by calling @initCallback.
 *
 * @threadMain
 */
- (void)viewDidMoveToWindow
{
	if ([[self window] isKindOfClass:[VuoWindowOpenGLInternal class]])
		self.glWindow = (VuoWindowOpenGLInternal *)[self window];

	if (togglingFullScreen)
		return;

	if (!initCallbackCalled)
	{
		dispatch_sync(drawQueue, ^{
						  NSOpenGLContext *glContext = [self openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  initCallback(cgl_ctx, drawContext);
						  [glContext flushBuffer];
						  CGLUnlockContext(cgl_ctx);
					  });
		initCallbackCalled = true;
	}
}

/**
 * Redraws this view by calling @c drawCallback.
 *
 * @threadAny
 */
void VuoWindowOpenGLView_draw(VuoReal frameRequest, void *context)
{
	VuoWindowOpenGLView *glView = (VuoWindowOpenGLView *)context;
	if (glView->callerRequestedRedraw)
	{
		glView->callerRequestedRedraw = false;

		if (glView->togglingFullScreen)
			return;

		dispatch_sync(glView->drawQueue, ^{
						  NSOpenGLContext *glContext = [glView openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  glView->drawCallback(cgl_ctx, glView->drawContext);
	//					  glFlush();
	//					  CGLFlushDrawable(CGLGetCurrentContext());
						  [glContext flushBuffer];
						  CGLUnlockContext(cgl_ctx);
					  });

	}
}

/**
 * Sets up the view to call trigger functions.
 *
 * @threadAny
 */
- (void)enableTriggers
{
	VuoDisplayRefresh_enableTriggers(displayRefresh, NULL, VuoWindowOpenGLView_draw);
}

/**
 * Stops the view from calling trigger functions.
 *
 * @threadAny
 */
- (void)disableTriggers
{
	VuoDisplayRefresh_disableTriggers(displayRefresh);
}

/**
 * Specifies that this view should accept mouseMoved and key events.
 *
 * @threadMain
 */
- (BOOL)acceptsFirstResponder
{
	return YES;
}

//- (void) keyDown: (NSEvent*) event
//{
//	VLogCF(event);
//}

/**
 * Converts @c point (relative to @c view) to default scene coordinates (-1 to 1 in X, proportional in Y).
 *
 * @threadAny
 */
VuoPoint2d VuoWindowOpenGLInternal_mapEventToSceneCoordinates(NSPoint point, NSView *view)
{
	point = [view convertPoint:point fromView:nil];

	NSRect bounds = [view bounds];
	point.x = ((point.x - bounds.size.width/2.) * 2.) / bounds.size.width;
	point.y = ((point.y - bounds.size.height/2.) * 2.) / bounds.size.width;

	return VuoPoint2d_make(point.x,point.y);
}

/**
 * Handles resizing of this view by calling @c resizeCallback.
 *
 * @threadMain
 */
- (void)reshape
{
	if (togglingFullScreen)
		return;

	dispatch_sync(drawQueue, ^{
					  NSOpenGLContext *glContext = [self openGLContext];
					  CGLContextObj cgl_ctx = [glContext CGLContextObj];
					  CGLLockContext(cgl_ctx);
					  NSRect frame = [self bounds];
					  if ([self respondsToSelector:@selector(convertRectToBacking:)])
					  {
						  typedef NSRect (*funcType)(id receiver, SEL selector, NSRect rect);
						  funcType convertRectToBacking = (funcType)[[self class] instanceMethodForSelector:@selector(convertRectToBacking:)];
						  frame = convertRectToBacking(self, @selector(convertRectToBacking:), frame);
					  }
					  glViewport(0, 0, frame.size.width, frame.size.height);
					  resizeCallback(cgl_ctx, drawContext, frame.size.width, frame.size.height);
					  drawCallback(cgl_ctx, drawContext);
					  [glContext flushBuffer];
					  CGLUnlockContext(cgl_ctx);
				  });
}

/**
 * Schedules the OpenGL view to be redrawn. This can be used in both windowed and full-screen mode.
 *
 * @threadAny
 */
- (void)scheduleRedraw
{
	callerRequestedRedraw = true;
}

/**
 * Makes Escape and Command-. exit full-screen mode.
 *
 * @threadMain
 */
- (void)cancelOperation:(id)sender
{
	if ([self isInFullScreenMode])
		[self toggleFullScreen];
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)toggleFullScreen
{
	if (! [self isInFullScreenMode])
	{
		// Pick the screen that shows the most of the the window's area.
		NSScreen *fullScreenScreen = [glWindow screen];

		// Create a GL context that can be used in full-screen mode.
		__block CGLError error;
		CGDirectDisplayID fullScreenDisplay = [[[fullScreenScreen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];  // http://www.cocoabuilder.com/archive/cocoa/133873-how-to-get-the-cgdirectdisplayid.html
		CGLPixelFormatAttribute fullScreenAttributes[] =
		{
			kCGLPFAFullScreen,
			kCGLPFADisplayMask, (CGLPixelFormatAttribute) CGDisplayIDToOpenGLDisplayMask(fullScreenDisplay),

			// copied from VuoGlContext
			kCGLPFAAccelerated,
			kCGLPFANoRecovery,
			kCGLPFADoubleBuffer,
			kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
			kCGLPFADepthSize, (CGLPixelFormatAttribute) glWindow.depthBuffer ? 16 : 0,
			kCGLPFAMultisample,
			kCGLPFASampleBuffers, (CGLPixelFormatAttribute) 1,
			kCGLPFASamples, (CGLPixelFormatAttribute) 4,
			(CGLPixelFormatAttribute) 0
		};
		CGLPixelFormatObj fullScreenPixelFormat;
		GLint numPixelFormats;
		error = CGLChoosePixelFormat(fullScreenAttributes, &fullScreenPixelFormat, &numPixelFormats);
		if (error != kCGLNoError)
		{
			fprintf(stderr, "Error: Couldn't create full-screen pixel format: %s\n", CGLErrorString(error));
			return;
		}

		__block CGLContextObj fullScreenContextCGL;
		dispatch_sync(drawQueue, ^{
						  CGLContextObj windowedGlContextCGL = [windowedGlContext CGLContextObj];
						  CGLLockContext(windowedGlContextCGL);
						  error = CGLCreateContext(fullScreenPixelFormat, windowedGlContextCGL, &fullScreenContextCGL);
						  CGLUnlockContext(windowedGlContextCGL);
					  });

		CGLDestroyPixelFormat(fullScreenPixelFormat);

		if (error != kCGLNoError)
		{
			fprintf(stderr, "Error: Couldn't create full-screen context: %s\n", CGLErrorString(error));
			return;
		}

		NSOpenGLContext *fullScreenContext = [[NSOpenGLContext alloc] initWithCGLContextObj:fullScreenContextCGL];
		int swapInterval=1;
		[fullScreenContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
		togglingFullScreen = true;
		dispatch_sync(drawQueue, ^{
						  CGLContextObj cgl_ctx = [windowedGlContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  switchContextCallback(cgl_ctx, fullScreenContextCGL, drawContext);
						  CGLUnlockContext(cgl_ctx);
					  });
		[self setOpenGLContext:fullScreenContext];
		[fullScreenContext setView:self];

		NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
								 [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens,  // Don't turn other screens black.
								 nil];

		[glWindow setAlphaValue:0];  // Hide the window on other screens. Unlike -[self orderOut:nil], this preserves the position.

		dispatch_sync(drawQueue, ^{
						  CGLContextObj cgl_ctx = [windowedGlContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  [self enterFullScreenMode:fullScreenScreen withOptions:options];
						  CGLUnlockContext(cgl_ctx);
						  togglingFullScreen = false;
					  });

		// Since the setup and draw were omitted during -enterFullScreenMode above (to prevent deadlock), invoke them manually now.
		[self viewDidMoveToWindow];
		[self reshape];
	}
	else
	{
		dispatch_sync(drawQueue, ^{
						  CGLContextObj cgl_ctx = [windowedGlContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  switchContextCallback([[self openGLContext] CGLContextObj], cgl_ctx, drawContext);

						  [self setOpenGLContext:windowedGlContext];
						  togglingFullScreen = true;
						  [windowedGlContext setView:self];
						  [self exitFullScreenModeWithOptions:nil];
						  [self.window makeKeyWindow];
						  togglingFullScreen = false;
						  CGLUnlockContext(cgl_ctx);
					  });

		[glWindow setAlphaValue:1];  // Un-hide the window on other screens.

		// Since the setup and draw were omitted during -exitFullScreenMode above (to prevent deadlock), invoke them manually now.
		[self viewDidMoveToWindow];
		[self reshape];
	}
}

@end



@implementation VuoWindowOpenGLInternal

@synthesize glView;
@synthesize depthBuffer;

/**
 * Creates a window containing an OpenGL view.
 *
 * @threadMain
 */
- (id)initWithDepthBuffer:(BOOL)_depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
	 switchContextCallback:(void (*)(VuoGlContext oldGlContext, VuoGlContext newGlContext, void *))_switchContextCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			   drawContext:(void *)_drawContext
{
	NSRect frame = NSMakeRect(0, 0, 1024, 768);
	NSUInteger styleMask = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	NSRect contentRect = [NSWindow contentRectForFrameRect:frame styleMask:styleMask];

	if (self = [super initWithContentRect:contentRect
								styleMask:styleMask
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.depthBuffer = _depthBuffer;
		self.delegate = self;

		callbacksEnabled = NO;

		[self setAcceptsMouseMovedEvents:YES];

		userResizedWindow = NO;
		programmaticallyResizingWindow = NO;

		[self setTitle:@"Vuo Scene"];

		VuoWindowOpenGLView *_glView = [[VuoWindowOpenGLView alloc] initWithFrame:[[self contentView] frame]
																	 initCallback:_initCallback
																   resizeCallback:_resizeCallback
															switchContextCallback:_switchContextCallback
																	 drawCallback:_drawCallback
																	  drawContext:_drawContext];
		self.glView = _glView;
		[_glView release];

		// 1. Set the GL context for the view.
		CGLContextObj vuoGlContext;
		{
			VuoGlContext vuoSharedGlContext = VuoGlContext_use();

			CGLPixelFormatAttribute pixelFormatAttributes[] =
			{
				kCGLPFAWindow,

				// copied from VuoGlContext
				kCGLPFAAccelerated,
				kCGLPFANoRecovery,
				kCGLPFADoubleBuffer,
				kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
				kCGLPFADepthSize, (CGLPixelFormatAttribute) depthBuffer? 16 : 0,
				kCGLPFAMultisample,
				kCGLPFASampleBuffers, (CGLPixelFormatAttribute) 1,
				kCGLPFASamples, (CGLPixelFormatAttribute) 4,
				(CGLPixelFormatAttribute) 0
			};
			CGLPixelFormatObj pixelFormat;
			GLint numPixelFormats;
			CGLError error = CGLChoosePixelFormat(pixelFormatAttributes, &pixelFormat, &numPixelFormats);
			if (error != kCGLNoError)
			{
				fprintf(stderr, "Error: Couldn't create windowed pixel format: %s\n", CGLErrorString(error));
				return nil;
			}

			error = CGLCreateContext(pixelFormat, vuoSharedGlContext, &vuoGlContext);
			VuoGlContext_disuse(vuoSharedGlContext);
			CGLDestroyPixelFormat(pixelFormat);
		}

		glView.windowedGlContext = [[NSOpenGLContext alloc] initWithCGLContextObj:vuoGlContext];
		int swapInterval=1;
		[glView.windowedGlContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
		[glView setOpenGLContext:glView.windowedGlContext];

		// 2. Add the view to the window. Do after (1) to have the desired GL context in -viewDidMoveToWindow.
		[self setContentView:glView];

		// 3. Set the view for the GL context. Do after (2) to avoid an "invalid drawable" warning.
		[glView.windowedGlContext setView:glView];
	}
	return self;
}

/**
 * Updates the menu bar with this window's menus (View > Full Screen).
 *
 * @threadMain
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	NSMenu *viewMenu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];
	NSMenuItem *fullScreenMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Full Screen" action:@selector(toggleFullScreen) keyEquivalent:@"f"] autorelease];
	[viewMenu addItem:fullScreenMenuItem];
	NSMenuItem *viewMenuItem = [[NSMenuItem new] autorelease];
	[viewMenuItem setSubmenu:viewMenu];
	NSArray *windowMenuItems = [NSArray arrayWithObject:viewMenuItem];
	[(VuoWindowApplication *)NSApp replaceWindowMenu:windowMenuItems];
}

/**
 * Sets up the window to call trigger functions.
 *
 * @threadAny
 */
- (void)enableTriggers
{
	callbacksEnabled = YES;

	[glView enableTriggers];
}

/**
 * Stops the window from calling trigger functions.
 *
 * @threadAny
 */
- (void)disableTriggers
{
	callbacksEnabled = NO;

	[glView disableTriggers];
}

/**
 * Schedules the OpenGL view to be redrawn. This can be used in both windowed and full-screen mode.
 *
 * @threadAny
 */
- (void)scheduleRedraw
{
	[glView scheduleRedraw];
}

/**
 * Executes the specifed block on the window's OpenGL context, then returns.
 * Ensures that nobody else is using the OpenGL context at that time
 * (by synchronizing with the window's @c drawQueue).
 *
 * @threadAny
 */
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute
{
	dispatch_sync([glView drawQueue], ^{
					  CGLContextObj cgl_ctx = [[glView openGLContext] CGLContextObj];
					  CGLLockContext(cgl_ctx);
					  blockToExecute(cgl_ctx);
					  CGLUnlockContext(cgl_ctx);
				  });
}

/**
 * Constrains the window's aspect ratio to @c pixelsWide/pixelsHigh. Also resizes the
 * window to @c pixelsWide by @c pixelsHigh, unless the user has manually resized the
 * window (in which case the width is preserved) or the requested size is larger than the
 * window's screen (in which case the window is scaled to fit the screen).
 *
 * @threadAny
 */
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh
{
	[self setContentAspectRatio:NSMakeSize(pixelsWide, pixelsHigh)];

	if ([glView isInFullScreenMode])
		return;

	CGFloat desiredWidth = pixelsWide;
	CGFloat desiredHeight = pixelsHigh;
	CGRect windowFrame = [self frame];
	CGFloat aspectRatio = (CGFloat)pixelsWide / (CGFloat)pixelsHigh;

	// Adjust the width and height if the user manually resized the window.
	if (userResizedWindow)
	{
		// Preserve the width, scale the height.
		desiredWidth = windowFrame.size.width;
		desiredHeight = windowFrame.size.width / aspectRatio;
	}

	// Adjust the width and height if they don't fit the screen.
	NSRect screenFrame = [[self screen] frame];
	NSRect maxContentRect = [self contentRectForFrameRect:screenFrame];
	if (desiredWidth > maxContentRect.size.width)
	{
		// Too wide, so scale it to the maximum horizontal screen space.
		desiredWidth = maxContentRect.size.width;
		desiredHeight = maxContentRect.size.width / aspectRatio;
	}
	else if (desiredHeight > maxContentRect.size.height)
	{
		// Too tall, so scale it to the maximum vertical screen space.
		desiredWidth = maxContentRect.size.height * aspectRatio;
		desiredHeight = maxContentRect.size.height;
	}

	NSSize newContentSize = NSMakeSize(desiredWidth, desiredHeight);
	NSSize newWindowSize = [self frameRectForContentRect:NSMakeRect(0, 0, newContentSize.width, newContentSize.height)].size;

	// Preserve the window's top left corner position. (If you just resize, it preserves the bottom left corner position.)
	CGFloat topY = windowFrame.origin.y + windowFrame.size.height;
	NSRect newWindowFrame = NSMakeRect(windowFrame.origin.x, topY - newWindowSize.height, newWindowSize.width, newWindowSize.height);

	programmaticallyResizingWindow = YES;
	[self setFrame:newWindowFrame display:YES];
	programmaticallyResizingWindow = NO;
}

/**
 * Keeps track of whether the user has manually resized the window.
 *
 * @threadMain
 */
- (void)windowDidResize:(NSNotification *)notification
{
	if (! programmaticallyResizingWindow)
		userResizedWindow = YES;
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)toggleFullScreen
{
	[glView toggleFullScreen];
}

@end
