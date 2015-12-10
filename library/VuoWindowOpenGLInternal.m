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
#import "VuoScreenCommon.h"

#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowOpenGLInternal",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoDisplayRefresh",
						 "VuoGLContext",
						 "VuoScreenCommon",
						 "VuoWindowProperty",
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

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (id)initWithFrame:(NSRect)frame
		   initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
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

		viewport = NSMakeRect(0, 0, frame.size.width, frame.size.height);

		if ([self respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
		{
			typedef void (*funcType)(id receiver, SEL selector, BOOL wants);
			funcType setWantsBestResolutionOpenGLSurface = (funcType)[[self class] instanceMethodForSelector:@selector(setWantsBestResolutionOpenGLSurface:)];
			setWantsBestResolutionOpenGLSurface(self, @selector(setWantsBestResolutionOpenGLSurface:), YES);
		}

		// Prepare the circle mouse cursor.
		circleRect = NSMakeRect(0,0,48,48);
		circleImage = [[NSImage alloc] initWithSize:circleRect.size];
		[circleImage lockFocus];
		{
			[[NSColor colorWithDeviceWhite:1 alpha:0.75] setFill];
			NSBezierPath *circlePath = [NSBezierPath bezierPathWithOvalInRect:circleRect];
			[circlePath fill];
		}
		[circleImage unlockFocus];
	}
	return self;
}

/**
 * Performs cleanup prior to closing the window (since -dealloc isn't guaranteed to be called).
 */
- (void)windowWillClose
{
	VuoRelease(displayRefresh);
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
		dispatch_sync(drawQueue, ^{
						  NSOpenGLContext *glContext = [self openGLContext];
						  CGLContextObj cgl_ctx = [glContext CGLContextObj];
						  CGLLockContext(cgl_ctx);

						  float backingScaleFactor = 1;
						  if ([[self window] respondsToSelector:@selector(backingScaleFactor)])
						  {
							  typedef double (*backingFuncType)(id receiver, SEL selector);
							  backingFuncType backingFunc = (backingFuncType)[[[self window] class] instanceMethodForSelector:@selector(backingScaleFactor)];
							  backingScaleFactor = backingFunc([self window], @selector(backingScaleFactor));
						  }

						  initCallback(cgl_ctx, backingScaleFactor, drawContext);
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

/**
 * Returns YES if this window is currently fullscreen.
 *
 * Use this instead of -[NSView isInFullScreenMode], since that method doesn't exist on 10.6.
 */
- (BOOL)isFullScreen
{
	return [self.window styleMask] == 0;
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
	// On pre-retina operating systems, we can assume points=pixels.
	if (![view respondsToSelector:@selector(convertRectToBacking:)])
		return rect;

	typedef NSRect (*funcType)(id, SEL, NSRect);
	funcType convertRectToBacking = (funcType)[[view class] instanceMethodForSelector:@selector(convertRectToBacking:)];
	return convertRectToBacking(view, @selector(convertRectToBacking:), rect);
}

/**
 * Converts a rectangle (in pixels) to points, depending on the backing type of the specified `view`.
 */
static NSRect convertPixelsToPoints(NSView *view, NSRect rect)
{
	// On pre-retina operating systems, we can assume points=pixels.
	if (![view respondsToSelector:@selector(convertRectFromBacking:)])
		return rect;

	typedef NSRect (*funcType)(id, SEL, NSRect);
	funcType convertRectFromBacking = (funcType)[[view class] instanceMethodForSelector:@selector(convertRectFromBacking:)];
	return convertRectFromBacking(view, @selector(convertRectFromBacking:), rect);
}

/**
 * Handles resizing of this view by calling @c resizeCallback.
 *
 * @threadMain
 */
- (void)reshape
{
	if (initCallbackCalled)
	dispatch_sync(drawQueue, ^{
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
	[self setFullScreen:NO onScreen:nil];
}

/**
 * Switches between full-screen and windowed mode.
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
		[self.window setStyleMask:0];
		if (!NSEqualSizes(car, NSMakeSize(0,0)))
			[self.window setContentAspectRatio:car];

		[self.window setFrame:[self.window screen].frame display:YES];

		[self reshape];
	}
	else if (!wantsFullScreen && [self isFullScreen])
	{
		// Switch out of fullscreen mode.

		[self.window setLevel:NSNormalWindowLevel];

		NSSize car = [self.window contentAspectRatio];
		[self.window setStyleMask:((VuoWindowOpenGLInternal *)self.window).styleMaskWhenWindowed];
		[self.window setTitle:((VuoWindowOpenGLInternal *)self.window).titleBackup];
		if (!NSEqualSizes(car, NSMakeSize(0,0)))
			[self.window setContentAspectRatio:car];

		[self.window setFrame:[self.window frameRectForContentRect:((VuoWindowOpenGLInternal *)self.window).contentRectWhenWindowed] display:YES];

		[self reshape];
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
		nsCursor = [[[NSCursor alloc] initWithImage:[[NSImage alloc] initWithSize:NSMakeSize(1,1)] hotSpot:NSMakePoint(0,0)] autorelease];
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



@implementation VuoWindowOpenGLInternal

@synthesize glView;
@synthesize depthBuffer;
@synthesize contentRectWhenWindowed;
@synthesize styleMaskWhenWindowed;
@synthesize cursor;
@synthesize titleBackup;

/**
 * Creates a window containing an OpenGL view.
 *
 * @threadMain
 */
- (id)initWithDepthBuffer:(BOOL)_depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
			   drawContext:(void *)_drawContext
{
	contentRectWhenWindowed = NSMakeRect(0, 0, 1024, 768);
	styleMaskWhenWindowed = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	if (self = [super initWithContentRect:contentRectWhenWindowed
								styleMask:styleMaskWhenWindowed
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.depthBuffer = _depthBuffer;
		self.delegate = self;

		cursor = VuoCursor_Pointer;

		callbacksEnabled = NO;

		[self setContentMinSize:NSMakeSize(16,16)];

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
		CGLContextObj vuoGlContext;
		{
			VuoGlContext vuoSharedGlContext = VuoGlContext_use();
			CGLPixelFormatObj pixelFormat = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(depthBuffer);
			CGLError error = CGLCreateContext(pixelFormat, vuoSharedGlContext, &vuoGlContext);
			if (error != kCGLNoError)
			{
				VLog("Error: %s", CGLErrorString(error));
				return NULL;
			}
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
- (void)windowWillClose:(NSNotification *)notification
{
	[glView windowWillClose];
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
- (void)enableTriggers:(void (*)(VuoWindowReference))_showedWindow
{
	callbacksEnabled = YES;

	showedWindow = _showedWindow;
	showedWindow(VuoWindowReference_make(self));
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

	showedWindow = NULL;
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
			[self setFullScreen:property.fullScreen onScreen:VuoScreen_getNSScreen(property.screen)];
		else if (property.type == VuoWindowProperty_Position)
		{
			NSRect mainScreenRect = [[[NSScreen screens] objectAtIndex:0] frame];
			if ([glView isFullScreen])
				contentRectWhenWindowed.origin = NSMakePoint(property.left, mainScreenRect.size.height - contentRectWhenWindowed.size.height - property.top);
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];
				[self setFrameOrigin:NSMakePoint(property.left, mainScreenRect.size.height - contentRect.size.height - property.top)];
			}
		}
		else if (property.type == VuoWindowProperty_Size)
		{
			if ([glView isFullScreen])
				contentRectWhenWindowed.size = NSMakeSize(property.width, property.height);
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];

				// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
				// (instead of its bottom-left corner as the system does by default).
				contentRect.origin.y += contentRect.size.height - property.height;

				contentRect.size = NSMakeSize(property.width, property.height);
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
 * Outputs the window's current width and height.
 */
- (void)getWidth:(unsigned int *)pixelsWide height:(unsigned int *)pixelsHigh
{
	NSSize size = [self frame].size;
	*pixelsWide = size.width;
	*pixelsHigh = size.height;
}

@end
