/**
 * @file
 * VuoWindowOpenGLInternal implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowOpenGLInternal.h"
#import "VuoWindow.h"
#import "VuoScreenCommon.h"
#import "VuoCglPixelFormat.h"

#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowOpenGLInternal",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoCglPixelFormat",
						 "VuoDisplayRefresh",
						 "VuoGLContext",
						 "VuoScreenCommon",
						 "VuoWindow",
						 "VuoWindowDrag",
						 "VuoWindowProperty",
						 "VuoWindowRecorder",
						 "VuoWindowReference",
						 "VuoList_VuoWindowProperty",
						 "OpenGL.framework"
					 ]
				 });
#endif



/**
 * NSWindow category by Jonathan 'Wolf' Rentzsch http://rentzsch.com
 */
@interface NSWindow (liveFrame)
- (NSRect)liveFrame;
@end
@implementation NSWindow (liveFrame)
/**
 * This method is because -[NSWindow frame] isn't updated continually during a drag.
 */
- (NSRect)liveFrame {
	Rect qdRect;
	extern OSStatus
	GetWindowBounds(
					WindowRef          window,
					WindowRegionCode   regionCode,
					Rect *             globalBounds);

	GetWindowBounds([self windowRef], kWindowStructureRgn, &qdRect);

	return NSMakeRect(qdRect.left,
					  (float)CGDisplayPixelsHigh(kCGDirectMainDisplay) - qdRect.bottom,
					  qdRect.right - qdRect.left,
					  qdRect.bottom - qdRect.top);
}
@end



@implementation VuoWindowOpenGLView

@synthesize glWindow;
@synthesize windowedGlContext;
@synthesize drawQueue;
@synthesize viewport;
@synthesize reshapeNeeded;
@synthesize skipDrawRect;

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (id)initWithFrame:(NSRect)frame
		   initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
  updateBackingCallback:(void (*)(VuoGlContext glContext, void *, float backingScaleFactor))_updateBackingCallback
		 resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
		   drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			drawContext:(void *)_drawContext
{
	if (self = [super initWithFrame:frame pixelFormat:[NSOpenGLView defaultPixelFormat]])
	{
		initCallback = _initCallback;
		updateBackingCallback = _updateBackingCallback;
		resizeCallback = _resizeCallback;
		drawCallback = _drawCallback;
		drawContext = _drawContext;

		drawQueue = dispatch_queue_create("vuo.window.opengl.internal.draw", 0);

		viewport = NSMakeRect(0, 0, frame.size.width, frame.size.height);

		[self setWantsBestResolutionOpenGLSurface:YES];

		// Prepare the circle mouse cursor.
		circleRect = NSMakeRect(0,0,48,48);
		circleImage = [[NSImage alloc] initWithSize:circleRect.size];
		[circleImage lockFocus];
		{
			[[NSColor colorWithDeviceWhite:1 alpha:0.75] setFill];
			[[NSColor colorWithDeviceWhite:0 alpha:0.15] setStroke];
			NSBezierPath *circlePath = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(circleRect, 1, 1)];
			[circlePath fill];
			[circlePath stroke];
		}
		[circleImage unlockFocus];
	}
	return self;
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	[circleImage release];
	[windowedGlContext release];
	[super dealloc];
}

/**
 * Handles this view being added to the @c VuoWindowOpenGLInternal by calling @initCallback.
 *
 * @threadMain
 */
- (void)viewDidMoveToWindow
{
	if ([[self window] isKindOfClass:[VuoWindowOpenGLInternal class]])
		self.glWindow = (VuoWindowOpenGLInternal *)[self window];

	if (!initCallbackCalled)
	{
		VUOLOG_PROFILE_BEGIN(drawQueue);
		dispatch_sync(drawQueue, ^{
						  VUOLOG_PROFILE_END(drawQueue);
						  NSOpenGLContext *glContext = [self openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);

						  float backingScaleFactor = [[self window] backingScaleFactor];
						  [self.glWindow setBackingScaleFactorCached:backingScaleFactor];

						  initCallback(cgl_ctx, backingScaleFactor, drawContext);
						  [glContext flushBuffer];
						  CGLUnlockContext(cgl_ctx);
					  });
		initCallbackCalled = true;
	}
}

/**
 * This delegate method fires after the window has changed screens (and many other times).
 * We need to catch it just when changing screens (reshapeNeeded), and recalculate the glViewport.
 */
- (void)viewWillDraw
{
	if (reshapeNeeded)
	{
		[self reshape];
		reshapeNeeded = false;
	}
}

/**
 * Called when the view changes screens (and thus maybe the backingScaleFactor changed).
 *
 * @threadMain
 */
- (void)viewDidChangeBackingProperties
{
	__block bool backingScaleFactorChanged = false;
	__block NSSize oldContentRectPixelSize;
	VUOLOG_PROFILE_BEGIN(drawQueue);
	dispatch_sync(drawQueue, ^{
					  VUOLOG_PROFILE_END(drawQueue);
					  float backingScaleFactor = [[self window] backingScaleFactor];
					  backingScaleFactorChanged = (backingScaleFactor != self.glWindow.backingScaleFactorCached);
					  if (backingScaleFactorChanged)
					  {
						  // Get the window's current size in pixels.
						  NSRect contentRect = [self.window contentRectForFrameRect:[self.window frame]];
						  oldContentRectPixelSize = NSMakeSize(contentRect.size.width*self.glWindow.backingScaleFactorCached, contentRect.size.height*self.glWindow.backingScaleFactorCached);

						  [self.glWindow setBackingScaleFactorCached:backingScaleFactor];

						  NSOpenGLContext *glContext = [self openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);

						  updateBackingCallback(cgl_ctx, drawContext, backingScaleFactor);

						CGLUnlockContext(cgl_ctx);
					  }
				  });

	if (backingScaleFactorChanged)
	{
		if (self.glWindow.maintainsPixelSizeWhenBackingChanges)
		{
			// Resize the window to maintain its pixel size.

			NSSize newContentRectPointSize = NSMakeSize(oldContentRectPixelSize.width  / self.glWindow.backingScaleFactorCached,
														oldContentRectPixelSize.height / self.glWindow.backingScaleFactorCached);


			NSRect contentRect = [self.window contentRectForFrameRect:[self.window frame]];

			// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
			// (instead of its bottom-left corner as the system does by default).
			contentRect.origin.y += contentRect.size.height - newContentRectPointSize.height;

			contentRect.size = newContentRectPointSize;
			[self.window setFrame:[self.window frameRectForContentRect:contentRect] display:YES animate:NO];
		}

		// Give the caller a chance to recompute the projection matrix.
		[self reshape];
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

		VUOLOG_PROFILE_BEGIN(drawQueue);
		dispatch_sync(glView->drawQueue, ^{
						  VUOLOG_PROFILE_END(drawQueue);
						  NSAutoreleasePool *pool = [NSAutoreleasePool new];
						  NSOpenGLContext *glContext = [glView openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);
						  glView->drawCallback(cgl_ctx, glView->drawContext);
	//					  glFlush();
	//					  CGLFlushDrawable(CGLGetCurrentContext());

#ifdef PROFILE
						  static double lastFrameTime = 0;
						  double start = VuoLogGetTime();
#endif

						  [glContext flushBuffer];

#ifdef PROFILE
						  double end = VuoLogGetTime();
						  if (lastFrameTime > 0 && start-lastFrameTime > 1.5 * 1./60.)
						  {
							  double dropTime = (start-lastFrameTime)*60.;
							  fprintf(stderr, "Dropped frame; draw took %f frame%s\n", dropTime, fabs(dropTime-1)<.00001 ? "" : "s");
						  }
						  lastFrameTime = end;
#endif

						  [[[glView glWindow] recorder] captureImageOfContext:cgl_ctx];
						  CGLUnlockContext(cgl_ctx);
						  [pool drain];
					  });

	}
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

/**
 * Returns YES if this window is currently fullscreen.
 */
- (BOOL)isFullScreen
{
	return [self.window styleMask] == 0
		|| [(VuoWindowOpenGLInternal *)self.window isInMacFullScreenMode];
}

/**
 * This method is implemented to avoid the system beep for keyboard events, which may be handled by vuo.keyboard nodes.
 */
- (void)keyDown:(NSEvent *)event
{
	if ([event keyCode] == kVK_Escape && [self isFullScreen])
		[super keyDown:event];
}

/**
 * Converts a rectangle (in points) to pixels, depending on the backing type of the specified `view`.
 */
static NSRect convertPointsToPixels(NSView *view, NSRect rect)
{
	return [view convertRectToBacking:rect];
}

/**
 * Converts a rectangle (in pixels) to points, depending on the backing type of the specified `view`.
 */
static NSRect convertPixelsToPoints(NSView *view, NSRect rect)
{
	return [view convertRectFromBacking:rect];
}

/**
 * Cocoa calls this method when it needs us to repaint the view.
 *
 * @threadMain
 */
- (void)drawRect:(NSRect)bounds
{
	if (skipDrawRect)
		return;

	callerRequestedRedraw = true;
	VuoWindowOpenGLView_draw(0, self);
}

/**
 * Handles resizing of this view by calling @c resizeCallback.
 *
 * @threadMain
 */
- (void)reshape
{
	if (!initCallbackCalled)
		return;

	if ([self.glWindow isClosed])
		return;

	VUOLOG_PROFILE_BEGIN(drawQueue);
	dispatch_sync(drawQueue, ^{
					  VUOLOG_PROFILE_END(drawQueue);
					  NSOpenGLContext *glContext = [self openGLContext];
					  CGLContextObj cgl_ctx = [glContext CGLContextObj];
					  CGLLockContext(cgl_ctx);
					  NSRect frame = convertPointsToPixels(self, [self bounds]);

					  // Dimensions, in pixels (not points).
					  int x = 0;
					  int y = 0;
					  int width = frame.size.width;
					  int height = frame.size.height;

					  // When fullscreen and aspect-locked, letterbox the scene (render to a centered rectangle that fits the size of the screen and matches the aspect of the now-hidden window).
					  NSSize desiredAspectSize = [glWindow contentAspectRatio];
					  bool isAspectLocked = !NSEqualSizes(desiredAspectSize, NSMakeSize(0,0));
					  if ([self isFullScreen] && isAspectLocked)
					  {
						  float desiredAspect = desiredAspectSize.width / desiredAspectSize.height;
						  float screenAspect = frame.size.width / frame.size.height;

						  if (desiredAspect > screenAspect)
						  {
							  // Match the screen's width.
							  width = frame.size.width;
							  height = frame.size.width / desiredAspect;
							  x = 0;
							  y = frame.size.height/2 - height/2;
						  }
						  else
						  {
							  // Match the screen's height.
							  width = frame.size.height * desiredAspect;
							  height = frame.size.height;
							  x = frame.size.width/2 - width/2;
							  y = 0;
						  }
					  }

					  // When fullscreen and non-resizable, windowbox the scene (render to a centered rectangle matching the size of the now-hidden window).
					  bool isWindowResizable = ([self isFullScreen] ? glWindow.styleMaskWhenWindowed : [glWindow styleMask]) & NSResizableWindowMask;
					  if ([self isFullScreen] && !isWindowResizable)
					  {
						  NSRect contentRectWhenWindowed = convertPointsToPixels(self, ((VuoWindowOpenGLInternal *)glWindow).contentRectWhenWindowed);
						  width = contentRectWhenWindowed.size.width;
						  height = contentRectWhenWindowed.size.height;
						  x = frame.size.width/2 - width/2;
						  y = frame.size.height/2 - height/2;
					  }

					  viewport = convertPixelsToPoints(self, NSMakeRect(x, y, width, height));

					  glViewport(x, y, width, height);
					  resizeCallback(cgl_ctx, drawContext, width, height);
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
	[self setFullScreen:NO onScreen:nil];
}

/**
 * Switches between full-screen and windowed mode.
 *
 * "Fullscreen" can mean 3 different things:
 *
 * 1. `-[NSView enterFullScreenMode:withOptions:]`
 *    - 10.5+
 *    - we don't use this at all
 * 2. Vuo's custom fullscreen mode —
 *    - instant (no transition animation)
 *    - achieved by removing the titlebar and resizing the window
 *    - can go fullscreen on any user-specified VuoScreen
 *    - can have multiple windows fullscreen simultaneously
 * 3. `-[NSWindow toggleFullScreen:]`
 *    - optional in 10.7 – 10.10, mandatory in 10.11+
 *    - painfully slow transition animation
 *    - can only have one window fullscreen when
 *      `System Preferences > Mission Control > Displays have separate Spaces`
 *      is unchecked
 *
 * Vuo uses #2 whenever possible (when triggered by ⌘F or a node),
 * and since #3 is now mandatory, Vuo supports it where needed —
 * when the user presses the green button in the titlebar:
 *
 *    - the window goes fullscreen (#3)
 *    - Vuo applies aspect-locking to the interior GL viewport
 *    - pressing ⌘F or ESC, or using the `Change Fullscreen Status` node,
 *      initiates the painfully-slow transition back to windowed mode,
 *      symmetric to how the user entered fullscreen mode
 *
 * @threadMain
 */
- (void)setFullScreen:(BOOL)wantsFullScreen onScreen:(NSScreen *)screen
{
	if (wantsFullScreen && ![self isFullScreen])
	{
		// Save the position, size, and style, to be restored when switching back to windowed mode.
		((VuoWindowOpenGLInternal *)self.window).contentRectWhenWindowed = [self.window contentRectForFrameRect:[self.window frame]];
		((VuoWindowOpenGLInternal *)self.window).styleMaskWhenWindowed = [self.window styleMask];


		// Switch into fullscreen mode:

		// Move the window to the specified screen.
		if (screen && ![[self.window screen] isEqualTo:screen])
		{
			NSRect windowFrame = [self.window frame];
			NSRect screenFrame = [screen frame];
			[self.window setFrameOrigin:NSMakePoint(screenFrame.origin.x + screenFrame.size.width/2  - windowFrame.size.width/2,
													screenFrame.origin.y + screenFrame.size.height/2 - windowFrame.size.height/2)];
		}

		// Make the window take up the whole screen.

		[self.window setLevel:NSScreenSaverWindowLevel];

		NSSize car = [self.window contentAspectRatio];
		skipDrawRect = true;
		[self.window setStyleMask:0];
		skipDrawRect = false;
		if (!NSEqualSizes(car, NSMakeSize(0,0)))
			[self.window setContentAspectRatio:car];

		[self.window setFrame:[self.window screen].frame display:YES];

//		[self reshape];

		[self.window makeFirstResponder:self];
	}
	else if (!wantsFullScreen && [self isFullScreen])
	{
		// Switch out of fullscreen mode.

		if ([(VuoWindowOpenGLInternal *)self.window isInMacFullScreenMode])
			[self.window toggleFullScreen:nil];
		else
		{
			[self.window setLevel:NSNormalWindowLevel];

			NSSize car = [self.window contentAspectRatio];
			skipDrawRect = true;
			[self.window setStyleMask:((VuoWindowOpenGLInternal *)self.window).styleMaskWhenWindowed];
			skipDrawRect = false;
			[self.window setTitle:((VuoWindowOpenGLInternal *)self.window).titleBackup];
			if (!NSEqualSizes(car, NSMakeSize(0,0)))
				[self.window setContentAspectRatio:car];

			[self.window setFrame:[self.window frameRectForContentRect:((VuoWindowOpenGLInternal *)self.window).contentRectWhenWindowed] display:YES];

//			[self reshape];
		}

		[self.window makeFirstResponder:self];
	}
}

- (void)update
{
	NSOpenGLContext *glContext = [self openGLContext];
	CGLContextObj cgl_ctx = [glContext CGLContextObj];
	CGLLockContext(cgl_ctx);
	[self.openGLContext update];
	CGLUnlockContext(cgl_ctx);
}

- (void)resetCursorRects
{
	VuoCursor cursor = ((VuoWindowOpenGLInternal *)self.window).cursor;
	NSCursor *nsCursor = nil;

	if (cursor == VuoCursor_None)
	{
		NSImage *im = [[NSImage alloc] initWithSize:NSMakeSize(1,1)];
		nsCursor = [[[NSCursor alloc] initWithImage:im hotSpot:NSMakePoint(0,0)] autorelease];
		[im release];
	}
	else if (cursor == VuoCursor_Pointer)
		nsCursor = [NSCursor arrowCursor];
	else if (cursor == VuoCursor_Crosshair)
		nsCursor = [NSCursor crosshairCursor];
	else if (cursor == VuoCursor_HandOpen)
		nsCursor = [NSCursor openHandCursor];
	else if (cursor == VuoCursor_HandClosed)
		nsCursor = [NSCursor closedHandCursor];
	else if (cursor == VuoCursor_IBeam)
		nsCursor = [NSCursor IBeamCursor];
	else if (cursor == VuoCursor_Circle)
		nsCursor = [[[NSCursor alloc] initWithImage:circleImage hotSpot:NSMakePoint(NSMidX(circleRect),NSMidY(circleRect))] autorelease];

	if (nsCursor)
		[self addCursorRect:[self visibleRect] cursor:nsCursor];
}

@end


/**
 * Workaround for Radar #19509497 (see below).
 */
@interface NSWindow (Accessibility)
/**
 * Workaround for Radar #19509497 (see below).
 */
- (void)accessibilitySetSizeAttribute:(NSValue *)size;
@end

@implementation VuoWindowOpenGLInternal

@synthesize glView;
@synthesize depthBuffer;
@synthesize backingScaleFactorCached;
@synthesize contentRectWhenWindowed;
@synthesize styleMaskWhenWindowed;
@synthesize cursor;
@synthesize titleBackup;
@synthesize recordMenuItem;
@synthesize temporaryMovieURL;

/**
 * On some GPUs, when the window is resized via the Accessiblity API, the GPU crashes when calling `glReadPixels()` the next time (Radar #19509497).
 *
 * Skip the Accessiblity resize operation on those GPUs.
 *
 * This is a private NSWindow API.
 * I first tried `AXObserverAddNotification(,,kAXWindowResizedNotification,)`, but that notification happens too late to prevent the crash.
 */
- (void)accessibilitySetSizeAttribute:(NSValue *)size
{
	CGLContextObj cgl_ctx = [[glView openGLContext] CGLContextObj];
	if (_recorder && strcmp((const char *)glGetString(GL_RENDERER), "NVIDIA GeForce GT 650M OpenGL Engine") == 0)
		return;

	[super accessibilitySetSizeAttribute:size];
}

/**
 * Creates a window containing an OpenGL view.
 *
 * @threadMain
 */
- (id)initWithDepthBuffer:(BOOL)_depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
	 updateBackingCallback:(void (*)(VuoGlContext glContext, void *, float backingScaleFactor))_updateBackingCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			   drawContext:(void *)_drawContext
{
	NSRect mainScreenFrame = [[NSScreen mainScreen] frame];
	contentRectWhenWindowed = NSMakeRect(mainScreenFrame.origin.x, mainScreenFrame.origin.y, 1024, 768);
	styleMaskWhenWindowed = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	if (self = [super initWithContentRect:contentRectWhenWindowed
								styleMask:styleMaskWhenWindowed
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.depthBuffer = _depthBuffer;
		self.delegate = self;
		self.releasedWhenClosed = NO;

		cursor = VuoCursor_Pointer;

		callbacksEnabled = NO;

		[self setContentMinSize:NSMakeSize(16,16)];

		[self setAcceptsMouseMovedEvents:YES];

//		self.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary;

		[self initDrag];
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

		userResizedWindow = NO;
		programmaticallyResizingWindow = NO;

		[self setTitle:@"Vuo Scene"];

		contentRectQueue = dispatch_queue_create("vuo.window.opengl.internal.contentrect", 0);

		VuoWindowOpenGLView *_glView = [[VuoWindowOpenGLView alloc] initWithFrame:[[self contentView] frame]
																	 initCallback:_initCallback
															updateBackingCallback:_updateBackingCallback
																   resizeCallback:_resizeCallback
																	 drawCallback:_drawCallback
																	  drawContext:_drawContext];

		displayRefresh = VuoDisplayRefresh_make(_glView);
		VuoRetain(displayRefresh);

		self.glView = _glView;
		[_glView release];

		// 1. Set the GL context for the view.
		CGLContextObj vuoGlContext;
		{
			VuoGlContext vuoSharedGlContext = VuoGlContext_use();
			CGLPixelFormatObj pixelFormat = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(depthBuffer);
			CGLError error = CGLCreateContext(pixelFormat, vuoSharedGlContext, &vuoGlContext);
			if (error != kCGLNoError)
			{
				VUserLog("Error: %s", CGLErrorString(error));

				CGLPixelFormatObj basePF = CGLGetPixelFormat(vuoSharedGlContext);
				VuoCglPixelFormat_logDiff(basePF, pixelFormat);

				return NULL;
			}
			VuoGlContext_disuse(vuoSharedGlContext);
			CGLDestroyPixelFormat(pixelFormat);
		}

		NSOpenGLContext *glc = [[NSOpenGLContext alloc] initWithCGLContextObj:vuoGlContext];
		int swapInterval=1;
		[glc setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
		[glView setOpenGLContext:glc];
		glView.windowedGlContext = glc;
		[glc release];

		// 2. Add the view to the window. Do after (1) to have the desired GL context in -viewDidMoveToWindow.
		[self setContentView:glView];

		// -[NSOpenGLContext setView] changes the TLS context.
		// Store it so we can change it back afterward, to be friendly to third-party host apps that use the TLS context.
		CGLContextObj priorMainThreadContext = CGLGetCurrentContext();

		// 3. Set the view for the GL context. Do after (2) to avoid an "invalid drawable" warning.
		[glView.windowedGlContext setView:glView];

		CGLSetCurrentContext(priorMainThreadContext);
	}

	[self setContentRectCached:[[self glView] viewport]];

	return self;
}

/**
 * Store the title, so we can re-apply it after returning from fullscreen mode.
 */
- (void)setTitle:(NSString *)title
{
	self.titleBackup = title;
	[super setTitle:title];
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
 * Allow this window to become main, even when it's borderless.
 */
- (BOOL)canBecomeMainWindow
{
	return YES;
}

/**
 * Allow this window to become key, even when it's borderless.
 */
- (BOOL)canBecomeKeyWindow
{
	return YES;
}

/**
 * Updates the menu bar with this window's menus (View > Full Screen).
 *
 * @threadMain
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
	self.recordMenuItem = [[[NSMenuItem alloc] initWithTitle:@"" action:@selector(toggleRecording) keyEquivalent:@"e"] autorelease];
	[self.recordMenuItem setKeyEquivalentModifierMask:NSCommandKeyMask|NSAlternateKeyMask];
	[fileMenu addItem:self.recordMenuItem];
	NSMenuItem *fileMenuItem = [[NSMenuItem new] autorelease];
	[fileMenuItem setSubmenu:fileMenu];

	NSMenu *viewMenu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];
	NSMenuItem *fullScreenMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Full Screen" action:@selector(toggleFullScreen) keyEquivalent:@"f"] autorelease];
	[viewMenu addItem:fullScreenMenuItem];
	NSMenuItem *viewMenuItem = [[NSMenuItem new] autorelease];
	[viewMenuItem setSubmenu:viewMenu];

	NSMutableArray *windowMenuItems = [NSMutableArray arrayWithCapacity:3];
	[windowMenuItems addObject:fileMenuItem];
	[windowMenuItems addObject:viewMenuItem];
	oldMenu = [(NSMenu *)VuoApp_setMenuItems(windowMenuItems) retain];

	[self updateUI];
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)resignMainWindow
{
	[super resignMainWindow];

	VuoApp_setMenu(oldMenu);
	[oldMenu release];
	oldMenu = nil;
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)windowWillClose:(NSNotification *)notification
{
	VuoApp_setMenu(oldMenu);
	[oldMenu release];
	oldMenu = nil;
	_isClosed = YES;
}

/**
 * NSOpenGLView changes the TLS context before rendering.
 * Store it so we can change it back afterward, to be friendly to third-party host apps that use the TLS context.
 */
- (void)displayIfNeeded
{
	CGLContextObj priorMainThreadContext = CGLGetCurrentContext();
	[super displayIfNeeded];
	CGLSetCurrentContext(priorMainThreadContext);
}

/**
 * NSOpenGLView changes the TLS context before rendering.
 * Store it so we can change it back afterward, to be friendly to third-party host apps that use the TLS context.
 */
- (void)setFrame:(NSRect)windowFrame display:(BOOL)displayViews
{
	CGLContextObj priorMainThreadContext = CGLGetCurrentContext();
	[super setFrame:windowFrame display:displayViews];
	CGLSetCurrentContext(priorMainThreadContext);
}

/**
 * Sets up the window to call trigger functions.
 *
 * @threadAny
 */
- (void)enableShowedWindowTrigger:(void (*)(VuoWindowReference))_showedWindow requestedFrameTrigger:(void (*)(VuoReal))_requestedFrame
{
	callbacksEnabled = YES;

	showedWindow = _showedWindow;
	showedWindow(VuoWindowReference_make(self));

	VuoDisplayRefresh_enableTriggers(displayRefresh, _requestedFrame, VuoWindowOpenGLView_draw);
}

/**
 * Stops the window from calling trigger functions.
 *
 * @threadAny
 */
- (void)disableTriggers
{
	callbacksEnabled = NO;

	showedWindow = NULL;

	VuoDisplayRefresh_disableTriggers(displayRefresh);
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
 * Applies a list of properties to this window.
 *
 * @threadMain
 */
- (void)setProperties:(VuoList_VuoWindowProperty)properties
{
	unsigned int propertyCount = VuoListGetCount_VuoWindowProperty(properties);
	for (unsigned int i = 1; i <= propertyCount; ++i)
	{
		VuoWindowProperty property = VuoListGetValue_VuoWindowProperty(properties, i);
//		VLog("%s",VuoWindowProperty_getSummary(property));

		if (property.type == VuoWindowProperty_Title)
			[self setTitle:[NSString stringWithUTF8String:property.title]];
		else if (property.type == VuoWindowProperty_FullScreen)
		{
			NSScreen *requestedScreen = VuoScreen_getNSScreen(property.screen);

			// If we're already fullscreen, and the property tells us to switch to a different screen,
			// temporarily switch back to windowed mode, so that we can cleanly switch to fullscreen on the new screen.
			if ([glView isFullScreen] && property.fullScreen)
			{
				NSInteger requestedDeviceId = [[[requestedScreen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
				NSInteger currentDeviceId = [[[self.screen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
				if (requestedDeviceId != currentDeviceId)
					[self setFullScreen:NO onScreen:nil];
			}

			[self setFullScreen:property.fullScreen onScreen:requestedScreen];
		}
		else if (property.type == VuoWindowProperty_Position)
		{
			NSRect propertyInPoints = NSMakeRect(property.left, property.top, 0, 0);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = convertPixelsToPoints(glView, propertyInPoints);

			NSRect mainScreenRect = [[[NSScreen screens] objectAtIndex:0] frame];
			if ([glView isFullScreen])
				contentRectWhenWindowed.origin = NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - contentRectWhenWindowed.size.height - propertyInPoints.origin.y);
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];
				[self setFrameOrigin:NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - contentRect.size.height - propertyInPoints.origin.y)];
			}
		}
		else if (property.type == VuoWindowProperty_Size)
		{
			NSRect propertyInPoints = NSMakeRect(0, 0, property.width, property.height);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = convertPixelsToPoints(glView, propertyInPoints);
			_maintainsPixelSizeWhenBackingChanges = (property.unit == VuoCoordinateUnit_Pixels);

			if ([glView isFullScreen])
				contentRectWhenWindowed.size = propertyInPoints.size;
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];

				// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
				// (instead of its bottom-left corner as the system does by default).
				contentRect.origin.y += contentRect.size.height - propertyInPoints.size.height;

				contentRect.size = NSMakeSize(propertyInPoints.size.width, propertyInPoints.size.height);
				[self setFrame:[self frameRectForContentRect:contentRect] display:YES animate:NO];
			}
		}
		else if (property.type == VuoWindowProperty_AspectRatio)
		{
			NSRect contentRect = [self contentRectForFrameRect:[self frame]];
			[self setAspectRatioToWidth:contentRect.size.width height:contentRect.size.width/property.aspectRatio];
		}
		else if (property.type == VuoWindowProperty_AspectRatioReset)
			[self unlockAspectRatio];
		else if (property.type == VuoWindowProperty_Resizable)
		{
			[[self standardWindowButton:NSWindowZoomButton] setEnabled:property.resizable];
			if ([glView isFullScreen])
				styleMaskWhenWindowed = property.resizable ? (styleMaskWhenWindowed | NSResizableWindowMask) : (styleMaskWhenWindowed & ~NSResizableWindowMask);
			else
				[self setStyleMask:     property.resizable ? ([self styleMask]      | NSResizableWindowMask) : ([self styleMask]      & ~NSResizableWindowMask) ];
		}
		else if (property.type == VuoWindowProperty_Cursor)
		{
			cursor = property.cursor;
			[self invalidateCursorRectsForView:glView];
		}
	}
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
	VUOLOG_PROFILE_BEGIN(drawQueue);
	dispatch_sync([glView drawQueue], ^{
					  VUOLOG_PROFILE_END(drawQueue);
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
 * @threadMain
 */
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh
{
	NSSize newAspect = NSMakeSize(pixelsWide, pixelsHigh);
	if (NSEqualSizes([self contentAspectRatio], newAspect))
		return;

	// Sets the constraint when the user resizes the window (but doesn't affect the window's current size).
	[self setContentAspectRatio:newAspect];

	if ([glView isFullScreen])
	{
		if (showedWindow)
			showedWindow(VuoWindowReference_make(self));
		[glView reshape];
		return;
	}

	CGFloat desiredWidth = pixelsWide;
	CGFloat desiredHeight = pixelsHigh;
	CGRect windowFrame = [self liveFrame];
	CGFloat aspectRatio = (CGFloat)pixelsWide / (CGFloat)pixelsHigh;

	// Adjust the width and height if the user manually resized the window.
	if (userResizedWindow)
	{
		// Preserve the width, scale the height.
		desiredWidth = CGRectGetWidth(windowFrame);
		desiredHeight = CGRectGetWidth(windowFrame) / aspectRatio;
	}

	// Adjust the width and height if they don't fit the screen.
	NSRect screenFrame = [[self screen] visibleFrame];
	NSRect maxContentRect = [self contentRectForFrameRect:screenFrame];
	if (desiredWidth > maxContentRect.size.width || desiredHeight > maxContentRect.size.height)
	{
		CGFloat maxContentAspectRatio = maxContentRect.size.width / maxContentRect.size.height;
		if (aspectRatio >= maxContentAspectRatio)
		{
			// Too wide, so scale it to the maximum horizontal screen space.
			desiredWidth = maxContentRect.size.width;
			desiredHeight = maxContentRect.size.width / aspectRatio;
		}
		else
		{
			// Too tall, so scale it to the maximum vertical screen space.
			desiredWidth = maxContentRect.size.height * aspectRatio;
			desiredHeight = maxContentRect.size.height;
		}
	}

	NSSize newContentSize = NSMakeSize(desiredWidth, desiredHeight);
	NSSize newWindowSize = [self frameRectForContentRect:NSMakeRect(0, 0, newContentSize.width, newContentSize.height)].size;

	// Preserve the window's top left corner position. (If you just resize, it preserves the bottom left corner position.)
	CGFloat topY = CGRectGetMinY(windowFrame) + CGRectGetHeight(windowFrame);
	NSRect newWindowFrame = NSMakeRect(CGRectGetMinX(windowFrame), topY - newWindowSize.height, newWindowSize.width, newWindowSize.height);

	programmaticallyResizingWindow = YES;
	[self setFrame:newWindowFrame display:YES];
	programmaticallyResizingWindow = NO;

	if (showedWindow)
		showedWindow(VuoWindowReference_make(self));
}

/**
 * Removes the aspect ratio constraint set by @ref VuoWindowOpenGl_setAspectRatio.
 *
 * @threadMain
 */
- (void)unlockAspectRatio
{
	[self setResizeIncrements:NSMakeSize(1,1)];
}

/**
 * Keeps track of whether the user has manually resized the window,
 * and fires the window-properties-changed callback.
 *
 * @threadMain
 */
- (void)windowDidResize:(NSNotification *)notification
{
	if (! programmaticallyResizingWindow)
	{
		userResizedWindow = YES;
		if (showedWindow)
			showedWindow(VuoWindowReference_make(self));
	}

	[self setContentRectCached:[[self glView] viewport]];
}

/**
 * Keep track of when the window enters OS X's now-mandatory 10.7+ fullscreen mode,
 * so we can apply reshape constraints, and prevent entering Vuo's custom fullscreen mode.
 */
- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
	self.isInMacFullScreenMode = YES;
}

/**
 * Keep track of when the window exits OS X's now-mandatory 10.7+ fullscreen mode,
 * so we can apply reshape constraints, and prevent entering Vuo's custom fullscreen mode.
 */
- (void)windowWillExitFullScreen:(NSNotification *)notification
{
	self.isInMacFullScreenMode = NO;
}

/**
 * Fires the window-properties-changed callback.
 *
 * @threadMain
 */
- (void)windowDidMove:(NSNotification *)notification
{
	if (showedWindow && !programmaticallyResizingWindow)
		showedWindow(VuoWindowReference_make(self));
}

/**
 * This delegate method fires after the window has changed screens,
 * later in the process than -[NSView viewDidChangeBackingProperties] fires.
 * We need to catch this since Cocoa automatically resets the glViewport to the wrong value
 * sometime in between these two delegate methods.
 *
 * Also, when fullscreen on a screen that disappears,
 * we need to adjust the window to fit the screen on which the window next appears.
 */
- (void)windowDidChangeScreen:(NSNotification *)notification
{
	glView.reshapeNeeded = true;

	if ([glView isFullScreen])
		[self setFrame:[self screen].frame display:YES];
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen
{
	[glView setFullScreen:fullScreen onScreen:screen];
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)toggleFullScreen
{
	[glView setFullScreen:![glView isFullScreen] onScreen:nil];
}

/**
 * Starts or stops recording a movie of the currently-selected window.
 *
 * @threadMain
 */
- (void)toggleRecording
{
	if (!self.recorder)
	{
		// Start recording to a temporary file (rather than first asking for the filename, to make it quicker to start recording).
		self.temporaryMovieURL = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]]];
		VuoWindowRecorder *r = [[VuoWindowRecorder alloc] initWithWindow:self url:self.temporaryMovieURL];
		self.recorder = r;
		[r release];

		[self updateUI];
	}
	else
		[self stopRecording];
}

/**
 * Ask the user for a permanent location to move the temporary movie file to.
 */
- (void)promptToSaveMovie
{
	NSSavePanel *sp = [NSSavePanel savePanel];
	[sp setTitle:@"Save Movie"];
	[sp setNameFieldLabel:@"Save Movie To:"];
	[sp setPrompt:@"Save"];
	[sp setAllowedFileTypes:@[@"mov"]];
	if ([sp runModal] == NSFileHandlingPanelCancelButton)
		goto done;

	NSError *error;
	if (![[NSFileManager defaultManager] moveItemAtURL:self.temporaryMovieURL toURL:[sp URL] error:&error])
	{
		if ([error code] == NSFileWriteFileExistsError)
		{
			// File exists.  Since, in the NSSavePanel, the user said to Replace, try replacing it.
			if (![[NSFileManager defaultManager] replaceItemAtURL:[sp URL]
					  withItemAtURL:self.temporaryMovieURL
					 backupItemName:nil
							options:0
				   resultingItemURL:nil
							  error:&error])
			{
				// Replacement failed; show error…
				NSAlert *alert = [NSAlert alertWithError:error];
				[alert runModal];

				// …and give the user another chance.
				[self promptToSaveMovie];
			}
			goto done;
		}

		NSAlert *alert = [NSAlert alertWithError:error];
		[alert runModal];
	}

done:
	self.temporaryMovieURL = nil;
}

/**
 * Stops recording a movie of the currently-selected window.
 *
 * Does nothing if recording is not currently enabled.
 *
 * @threadMain
 */
- (void)stopRecording
{
	if (!_recorder)
		return;

	[_recorder finish];
	[_recorder release];
	_recorder = nil;

	[self updateUI];

	[glView setFullScreen:NO onScreen:nil];

	[self promptToSaveMovie];
}

/**
 * Updates various UI elements to match current internal state.
 */
- (void)updateUI
{
	if (self.recorder)
		self.recordMenuItem.title = @"Stop Recording…";
	else
		self.recordMenuItem.title = @"Start Recording";
}

/**
 * Thread-safe accessor.
 *
 * @threadAny
 */
- (NSRect)contentRectCached
{
	__block NSRect c;
	dispatch_sync(contentRectQueue, ^{
					  c = contentRectCached;
				  });
	return c;
}

/**
 * Thread-safe mutator.
 *
 * @threadAny
 */
- (void)setContentRectCached:(NSRect)c
{
	dispatch_sync(contentRectQueue, ^{
					  contentRectCached = c;
				  });
}

@end
