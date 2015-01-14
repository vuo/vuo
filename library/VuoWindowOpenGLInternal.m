/**
 * @file
 * VuoWindowOpenGLInternal implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (id)initWithFrame:(NSRect)frame
	   initCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
	 resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
	   drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			   drawContext:(void *)_drawContext
{
	if (self = [super initWithFrame:frame pixelFormat:[NSOpenGLView defaultPixelFormat]])
	{
		initCallback = _initCallback;
		resizeCallback = _resizeCallback;
		drawCallback = _drawCallback;
		drawContext = _drawContext;

		drawQueue = dispatch_queue_create("vuo.window.opengl.internal.draw", 0);

		displayRefresh = VuoDisplayRefresh_make(self);
		VuoRetain(displayRefresh);

		pendingClickCount = 0;
		clickQueue = dispatch_queue_create("vuo.window.opengl.internal.click", 0);
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

	dispatch_sync(drawQueue, ^{
					  NSOpenGLContext *glContext = [self openGLContext];
					  initCallback([glContext CGLContextObj], drawContext);
					  [glContext flushBuffer];
				  });
}

/**
 * Redraws this view by calling @c drawCallback.
 *
 * @threadAny
 */
void VuoWindowOpenGLView_draw(VuoFrameRequest frameRequest, void *context)
{
	VuoWindowOpenGLView *glView = (VuoWindowOpenGLView *)context;
	if (glView->callerRequestedRedraw)
	{
		glView->callerRequestedRedraw = false;

		if (glView->togglingFullScreen)
			return;

		dispatch_sync(glView->drawQueue, ^{
						  NSOpenGLContext *glContext = [glView openGLContext];
						  glView->drawCallback([glContext CGLContextObj], glView->drawContext);
	//					  glFlush();
	//					  CGLFlushDrawable(CGLGetCurrentContext());
						  [glContext flushBuffer];
					  });

	}
}

/**
 * Sets up the view to call trigger functions.
 *
 * @threadAny
 */
- (void)enableTriggersWithMovedMouseTo:(void (*)(VuoPoint2d))_movedMouseTo
						 scrolledMouse:(void (*)(VuoPoint2d))_scrolledMouse
					   usedMouseButton:(void (*)(VuoMouseButtonAction))_usedMouseButton
{
	movedMouseTo = _movedMouseTo;
	scrolledMouse = _scrolledMouse;
	usedMouseButton = _usedMouseButton;
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
	movedMouseTo = NULL;
	scrolledMouse = NULL;
	usedMouseButton = NULL;
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

- (void)processClicksForEvent:(NSEvent *)event atPosition:(VuoPoint2d)position withButton:(VuoMouseButton)button
{
	double clickIntervalSeconds = [NSEvent doubleClickInterval];
	dispatch_time_t clickInterval = dispatch_time(DISPATCH_TIME_NOW, clickIntervalSeconds * NSEC_PER_SEC);

	int clickCount = [event clickCount];
	pendingClickCount = clickCount;

	if (clickCount == 1)
	{
		VuoMouseButtonAction buttonAction = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_SingleClick, position);
		dispatch_after(clickInterval, clickQueue, ^{
						   if (pendingClickCount == clickCount)
						   {
							   usedMouseButton(buttonAction);
							   pendingClickCount = 0;
						   }
					   });
	}
	else if (clickCount == 2)
	{
		VuoMouseButtonAction buttonAction = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_DoubleClick, position);
		dispatch_after(clickInterval, clickQueue, ^{
						   if (pendingClickCount == clickCount)
						   {
							   usedMouseButton(buttonAction);
							   pendingClickCount = 0;
						   }
					   });
	}
	else if (clickCount == 3)
	{
		VuoMouseButtonAction buttonAction = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_TripleClick, position);
		dispatch_sync(clickQueue, ^{
						  if (pendingClickCount == clickCount)
						  {
							  usedMouseButton(buttonAction);
							  pendingClickCount = 0;
						  }
					  });
	}
}

- (void)mouseDown:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Left, VuoMouseButtonActionType_Press,
											  VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self)));
}

- (void)mouseUp:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	VuoPoint2d position = VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self);
	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Left, VuoMouseButtonActionType_Release, position));
	[self processClicksForEvent:event atPosition:position withButton:VuoMouseButton_Left];
}

- (void)rightMouseDown:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Right, VuoMouseButtonActionType_Press,
											  VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self)));
}

- (void)rightMouseUp:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	VuoPoint2d position = VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self);
	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Right, VuoMouseButtonActionType_Release, position));
	[self processClicksForEvent:event atPosition:position withButton:VuoMouseButton_Right];
}

- (void)otherMouseDown:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Middle, VuoMouseButtonActionType_Press,
											  VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self)));
}

- (void)otherMouseUp:(NSEvent *)event
{
	if (!usedMouseButton)
		return;

	VuoPoint2d position = VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self);
	usedMouseButton(VuoMouseButtonAction_make(VuoMouseButton_Middle, VuoMouseButtonActionType_Release, position));
	[self processClicksForEvent:event atPosition:position withButton:VuoMouseButton_Middle];
}

/**
 * Handles the mouse moving by calling the trigger function.
 *
 * @threadMain
 */
- (void)mouseMoved:(NSEvent *)event
{
	if (!movedMouseTo)
		return;

	movedMouseTo(VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self));
}

/**
 * Handles the mouse scrolling by calling the trigger function.
 *
 * @threadMain
 */
- (void)scrollWheel:(NSEvent *)event
{
	if (!scrolledMouse)
		return;

	// On 10.8, [event deltaX] emits a console warning saying it's deprecated,
	// yet scrollingDeltaX doesn't exist before 10.7,
	// so do this dance to try to satisfy both OS versions.
	if ([event respondsToSelector:@selector(scrollingDeltaX)])
	{
		typedef CGFloat (*ScrollingDelta)(id receiver, SEL selector);
		ScrollingDelta scrollingDeltaXFunction = (ScrollingDelta)[NSEvent instanceMethodForSelector:@selector(scrollingDeltaX)];
		ScrollingDelta scrollingDeltaYFunction = (ScrollingDelta)[NSEvent instanceMethodForSelector:@selector(scrollingDeltaY)];
		float x = scrollingDeltaXFunction(event, @selector(scrollingDeltaX));
		float y = scrollingDeltaYFunction(event, @selector(scrollingDeltaY));
		scrolledMouse(VuoPoint2d_make(x,y));
	}
	else
		scrolledMouse(VuoPoint2d_make([event deltaX], [event deltaY]));
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
					  NSRect frame = [self frame];
					  glViewport(0, 0, frame.size.width, frame.size.height);
					  resizeCallback([glContext CGLContextObj], drawContext, frame.size.width, frame.size.height);
					  drawCallback([glContext CGLContextObj], drawContext);
					  [glContext flushBuffer];
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
			kCGLPFADepthSize, (CGLPixelFormatAttribute) 16,
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
						  error = CGLCreateContext(fullScreenPixelFormat, windowedGlContextCGL, &fullScreenContextCGL);
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
		[self setOpenGLContext:fullScreenContext];
		[fullScreenContext setView:self];

		NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
								 [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens,  // Don't turn other screens black.
								 nil];

		[glWindow setAlphaValue:0];  // Hide the window on other screens. Unlike -[self orderOut:nil], this preserves the position.

		dispatch_sync(drawQueue, ^{
							   [self enterFullScreenMode:fullScreenScreen withOptions:options];
							   togglingFullScreen = false;
					  });

		// Since the setup and draw were omitted during -enterFullScreenMode above (to prevent deadlock), invoke them manually now.
		[self viewDidMoveToWindow];
		[self reshape];
	}
	else
	{
		dispatch_sync(drawQueue, ^{
						  [self setOpenGLContext:windowedGlContext];
						  togglingFullScreen = true;
						  [windowedGlContext setView:self];
						  [self exitFullScreenModeWithOptions:nil];
						  togglingFullScreen = false;
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

/**
 * Creates a window containing an OpenGL view.
 *
 * @threadMain
 */
- (id)initWithInitCallback:(void (*)(VuoGlContext glContext, void *))_initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
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
		self.delegate = self;

		callbacksEnabled = NO;

		[self setAcceptsMouseMovedEvents:YES];

		userResizedWindow = NO;
		programmaticallyResizingWindow = NO;

		[self setTitle:@"Vuo Scene"];

		VuoWindowOpenGLView *_glView = [[VuoWindowOpenGLView alloc] initWithFrame:[[self contentView] frame]
																	 initCallback:_initCallback
																   resizeCallback:_resizeCallback
																	 drawCallback:_drawCallback
																	  drawContext:_drawContext];
		self.glView = _glView;
		[_glView release];

		// 1. Set the GL context for the view.
		VuoGlContext vuoGlContext = VuoGlContext_use();
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
- (void)enableTriggersWithMovedMouseTo:(void (*)(VuoPoint2d))_movedMouseTo
						 scrolledMouse:(void (*)(VuoPoint2d))_scrolledMouse
					   usedMouseButton:(void (*)(VuoMouseButtonAction))_usedMouseButton
{
	callbacksEnabled = YES;


	[glView enableTriggersWithMovedMouseTo:_movedMouseTo
							 scrolledMouse:_scrolledMouse
						   usedMouseButton:_usedMouseButton];
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
