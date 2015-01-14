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

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowOpenGLInternal",
					 "dependencies" : [
						 "AppKit.framework",
						 "CoreVideo.framework",
						 "VuoGLContext"
					 ]
				 });
#endif


@implementation VuoWindowOpenGLView

@synthesize glWindow;

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 */
- (id)initWithFrame:(NSRect)frame
	   initCallback:(void (*)(void *))_initCallback
	 resizeCallback:(void (*)(void *, unsigned int width, unsigned int height))_resizeCallback
	   drawCallback:(void (*)(void *))_drawCallback
			   drawContext:(void *)_drawContext
{
	if (self = [super initWithFrame:frame pixelFormat:[NSOpenGLView defaultPixelFormat]])
	{
		initCallback = _initCallback;
		resizeCallback = _resizeCallback;
		drawCallback = _drawCallback;
		drawContext = _drawContext;

		pendingClickCount = 0;
		clickQueue = dispatch_queue_create("vuo.window.opengl.internal.click", 0);
	}
	return self;
}

/**
 * Handles this view being added to the @c VuoWindowOpenGLInternal, or switched between that
 * and full-screen, by calling @initCallback.
 */
- (void)viewDidMoveToWindow
{
	if ([[self window] isKindOfClass:[VuoWindowOpenGLInternal class]])
		self.glWindow = (VuoWindowOpenGLInternal *)[self window];

	NSOpenGLContext *glContext = [self openGLContext];
	[glContext makeCurrentContext];

	initCallback(drawContext);

	[glContext flushBuffer];

	[NSOpenGLContext clearCurrentContext];
}

/**
 * Sets up the view to call trigger functions.
 */
- (void)enableTriggersWithMovedMouseTo:(void (*)(VuoPoint2d))_movedMouseTo
						 scrolledMouse:(void (*)(VuoPoint2d))_scrolledMouse
					   usedMouseButton:(void (*)(VuoMouseButtonAction))_usedMouseButton
{
	movedMouseTo = _movedMouseTo;
	scrolledMouse = _scrolledMouse;
	usedMouseButton = _usedMouseButton;
}

/**
 * Stops the view from calling trigger functions.
 */
- (void)disableTriggers
{
	movedMouseTo = NULL;
	scrolledMouse = NULL;
	usedMouseButton = NULL;
}

/**
 * Specifies that this view should accept mouseMoved and key events.
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
 */
- (void)mouseMoved:(NSEvent *)event
{
	if (!movedMouseTo)
		return;

	movedMouseTo(VuoWindowOpenGLInternal_mapEventToSceneCoordinates([event locationInWindow], self));
}

/**
 * Handles the mouse scrolling by calling the trigger function.
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
 */
- (void)reshape
{
	NSOpenGLContext *glContext = [self openGLContext];
	[glContext makeCurrentContext];
	NSRect frame = [self frame];
	glViewport(0, 0, frame.size.width, frame.size.height);
	resizeCallback(drawContext, frame.size.width, frame.size.height);
	[NSOpenGLContext clearCurrentContext];
}

/**
 * Redraws this view by calling @c drawCallback.
 */
- (void)drawRect:(NSRect)bounds
{
	NSOpenGLContext *glContext = [self openGLContext];
	[glContext makeCurrentContext];

	drawCallback(drawContext);

	glFlush();

//	CGLFlushDrawable(CGLGetCurrentContext());
	[glContext flushBuffer];

	[NSOpenGLContext clearCurrentContext];
}

/**
 * Makes Escape and Command-. exit full-screen mode.
 */
- (void)cancelOperation:(id)sender
{
	if ([self isInFullScreenMode])
		[[self glWindow] toggleFullScreen];
}

@end



/**
 * Context data for @c VuoWindowOpenGLInternal_displayLinkCallback.
 */
struct VuoWindowOpenGLInternal_displayLinkContext
{
	void (*requestedFrameTrigger)(VuoFrameRequest);	///< The trigger function, to be fired when the display link wants the application to output a frame.
	bool firstRequest;	///< Is this the first time the display link callback has been invoked?
	int64_t initialTime;	///< The output time for which the display link was first invoked.
	VuoInteger frameCount;	///< The nubmer of frames requested so far.
};

/**
 * Called by CoreVideo whenever the display link wants the application to output a frame.
 */
static CVReturn VuoWindowOpenGLInternal_displayLinkCallback(CVDisplayLinkRef displayLink,
															const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime,
															CVOptionFlags flagsIn, CVOptionFlags *flagsOut,
															void *ctx)
{
	struct VuoWindowOpenGLInternal_displayLinkContext *context = (struct VuoWindowOpenGLInternal_displayLinkContext *)ctx;
	if (context->firstRequest)
	{
		context->initialTime = inOutputTime->videoTime;
		context->firstRequest = false;
	}

	if (context->requestedFrameTrigger)
	{
		double timestamp = (double)(inOutputTime->videoTime - context->initialTime)/(double)inOutputTime->videoTimeScale;
		context->requestedFrameTrigger(VuoFrameRequest_make(timestamp, context->frameCount++));
	}

	return kCVReturnSuccess;
}



@implementation VuoWindowOpenGLInternal

@synthesize glView;
@synthesize windowedGlContext;

/**
 * Creates a window containing an OpenGL view.
 *
 * Must be called on the main thread.
 */
- (id)initWithInitCallback:(void (*)(void *))_initCallback
			resizeCallback:(void (*)(void *, unsigned int width, unsigned int height))_resizeCallback
			  drawCallback:(void (*)(void *))_drawCallback
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

		displayLinkContext = (struct VuoWindowOpenGLInternal_displayLinkContext *)malloc(sizeof(struct VuoWindowOpenGLInternal_displayLinkContext));
		displayLinkContext->firstRequest = true;
		displayLinkContext->frameCount = 0;

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
		self.windowedGlContext = [[NSOpenGLContext alloc] initWithCGLContextObj:vuoGlContext];
		[glView setOpenGLContext:windowedGlContext];

		// 2. Add the view to the window. Do after (1) to have the desired GL context in -viewDidMoveToWindow.
		[self setContentView:glView];

		// 3. Set the view for the GL context. Do after (2) to avoid an "invalid drawable" warning.
		[windowedGlContext setView:glView];
	}
	return self;
}

/**
 * Updates the menu bar with this window's menus (View > Full Screen).
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
 */
- (void)enableTriggersWithRequestedFrame:(void (*)(VuoFrameRequest))_requestedFrame
							movedMouseTo:(void (*)(VuoPoint2d))_movedMouseTo
						   scrolledMouse:(void (*)(VuoPoint2d))_scrolledMouse
						 usedMouseButton:(void (*)(VuoMouseButtonAction))_usedMouseButton
{
	callbacksEnabled = YES;

	displayLinkContext->requestedFrameTrigger = _requestedFrame;

	if(CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &displayLink) == kCVReturnSuccess)
		CVDisplayLinkSetOutputCallback(displayLink, VuoWindowOpenGLInternal_displayLinkCallback, displayLinkContext);

	CVDisplayLinkStart(displayLink);

	[glView enableTriggersWithMovedMouseTo:_movedMouseTo
							 scrolledMouse:_scrolledMouse
						   usedMouseButton:_usedMouseButton];
}

/**
 * Stops the window from calling trigger functions.
 */
- (void)disableTriggers
{
	callbacksEnabled = NO;

	CVDisplayLinkStop(displayLink);
	displayLinkContext->requestedFrameTrigger = NULL;

	[glView disableTriggers];
}

/**
 * Schedules the OpenGL view to be redrawn. This can be used in both windowed and full-screen mode.
 */
- (void)scheduleRedraw
{
	[glView setNeedsDisplay:YES];
}

/**
 * Constrains the window's aspect ratio to @c pixelsWide/pixelsHigh. Also resizes the
 * window to @c pixelsWide by @c pixelsHigh, unless the user has manually resized the
 * window (in which case the width is preserved) or the requested size is larger than the
 * window's screen (in which case the window is scaled to fit the screen).
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
 */
- (void)windowDidResize:(NSNotification *)notification
{
	if (! programmaticallyResizingWindow)
		userResizedWindow = YES;
}

/**
 * Switches between full-screen and windowed mode.
 */
- (void)toggleFullScreen
{
	if (! [glView isInFullScreenMode])
	{
		// Pick the screen that shows the most of the the window's area.
		NSScreen *fullScreenScreen = [self screen];

		// Create a GL context that can be used in full-screen mode.
		CGLError error;
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
		CGLContextObj windowedGlContextCGL = [windowedGlContext CGLContextObj];
		CGLContextObj fullScreenContextCGL;
		error = CGLCreateContext(fullScreenPixelFormat, windowedGlContextCGL, &fullScreenContextCGL);
		CGLDestroyPixelFormat(fullScreenPixelFormat);
		if (error != kCGLNoError)
		{
			fprintf(stderr, "Error: Couldn't create full-screen context: %s\n", CGLErrorString(error));
			return;
		}
		NSOpenGLContext *fullScreenContext = [[NSOpenGLContext alloc] initWithCGLContextObj:fullScreenContextCGL];
		[glView setOpenGLContext:fullScreenContext];
		[fullScreenContext setView:glView];

		NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
								 [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens,  // Don't turn other screens black.
								 nil];

		[self setAlphaValue:0];  // Hide the window on other screens. Unlike -[self orderOut:nil], this preserves the position.

		[glView enterFullScreenMode:fullScreenScreen withOptions:options];
	}
	else
	{
		[glView setOpenGLContext:windowedGlContext];
		[windowedGlContext setView:glView];

		[glView exitFullScreenModeWithOptions:nil];

		[self setAlphaValue:1];  // Un-hide the window on other screens.
	}
}

@end
