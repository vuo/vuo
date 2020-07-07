/**
 * @file
 * VuoGraphicsWindow implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "module.h"
#import "VuoApp.h"
#import "VuoGraphicsLayer.h"
#import "VuoGraphicsView.h"
#import "VuoGraphicsWindow.h"
#import "VuoGraphicsWindowDelegate.h"
#import "VuoGraphicsWindowDrag.h"
#import "VuoScreenCommon.h"
#import <Carbon/Carbon.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsWindow",
	"dependencies" : [
		"VuoGraphicsLayer",
		"VuoGraphicsView",
		"VuoGraphicsWindowDelegate",
		"VuoRenderedLayers",
		"VuoScreenCommon",
		"VuoWindowProperty",
		"VuoWindowRecorder",
		"VuoWindowReference",
		"VuoList_VuoWindowProperty"
	]
});
#endif

/// Mac OS 10.8's Cocoa crashes below 5.
/// macOS 10.12's Cocoa crashes below 4.
const int VuoGraphicsWindowMinSize = 5;

const NSInteger VuoViewMenuItemTag = 1000; ///< The "View" menu.
const NSInteger VuoFullScreenMenuItemTag = 1001; ///< The menu item for toggling full screen mode.

/// Serialize making windows fullscreen, since Mac OS X beeps at you (!) if you try to fullscreen two windows too quickly.
dispatch_semaphore_t VuoGraphicsWindow_fullScreenTransitionSemaphore;

/**
 * Initialize VuoGraphicsWindow_fullScreenTransitionSemaphore.
 */
static void __attribute__((constructor)) VuoGraphicsWindow_init()
{
	 VuoGraphicsWindow_fullScreenTransitionSemaphore = dispatch_semaphore_create(1);
}


/**
 * Private VuoGraphicsWindow data.
 */
@interface VuoGraphicsWindow ()
@property(retain) NSMenuItem *recordMenuItem;  ///< The record/stop menu item.
@property(retain) id<NSWindowDelegate> privateDelegate;  ///< Maintain our own reference-counted delegate (since NSWindow's delegate property is a weak reference).
@property bool constrainToScreen;  ///< Allow compositions to programmatically make the window size greater than or equal to the screen.
@property(retain) NSScreen *shouldGoFullscreen;  ///< If non-NULL, specifies the screen on which the window should go fullscreen after completing the previous exit-fullscreen transition.

@property bool leftMouseDown;    ///< Whether the left mouse button is currently pressed (causing window resizing to be deferred).
@property id mouseMonitor;       ///< Updates `leftMouseDown`.
@property NSSize pendingResize;  ///< If nonzero, the size (in points) the window's content area should change to, once the window drag has concluded.
@end

@implementation VuoGraphicsWindow

/**
 * Creates a window with the default size and position.
 */
- (instancetype)initWithInitCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData
{
	NSRect mainScreenFrame = [[NSScreen mainScreen] frame];
	_contentRectWhenWindowed = NSMakeRect(mainScreenFrame.origin.x, mainScreenFrame.origin.y, VuoGraphicsWindowDefaultWidth, 768);
	_styleMaskWhenWindowed = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	if (self = [super initWithContentRect:_contentRectWhenWindowed
								styleMask:_styleMaskWhenWindowed
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.colorSpace = NSColorSpace.sRGBColorSpace;

		self.privateDelegate = [[[VuoGraphicsWindowDelegate alloc] initWithWindow:self] autorelease];
		self.delegate = self.privateDelegate;
		self.releasedWhenClosed = NO;

		_cursor = VuoCursor_Pointer;

		self.contentMinSize = NSMakeSize(VuoGraphicsWindowMinSize, VuoGraphicsWindowMinSize);

		self.acceptsMouseMovedEvents = YES;

		self.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;

		[self initDrag];
		[self registerForDraggedTypes:@[NSFilenamesPboardType]];

		_userResizedWindow = NO;
		_programmaticallyResizingWindow = NO;
		_constrainToScreen = YES;

		char *title = VuoApp_getName();
		self.title = [NSString stringWithUTF8String:title];
		free(title);

		_backingScaleFactorCached = self.backingScaleFactor;

		VuoGraphicsLayer *l = [[VuoGraphicsLayer alloc] initWithWindow:self
																initCallback:initCallback
													   updateBackingCallback:updateBackingCallback
														  backingScaleFactor:_backingScaleFactorCached
															  resizeCallback:resizeCallback
																drawCallback:drawCallback
																	userData:userData];
		l.contentsScale = _backingScaleFactorCached;

		VuoGraphicsView *v = [[VuoGraphicsView alloc] init];
		v.layer = l;
		[l release];

		self.contentView = v;
		[v release];

		_contentRectCached = l.frame;

		_mouseMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseDownMask|NSLeftMouseUpMask handler:^(NSEvent *event) {
			if (event.type == NSLeftMouseDown)
				_leftMouseDown = true;
			else if (event.type == NSLeftMouseUp)
			{
				_leftMouseDown = false;
				if (!NSEqualSizes(_pendingResize, NSZeroSize))
				{
					VDebugLog("The window drag has concluded; performing the deferred resize.");
					self.contentSize = _pendingResize;
					_pendingResize = NSZeroSize;
				}
			}
			return event;
		}];
	}

	return self;
}

/**
 * Schedules the OpenGL view to be repainted.
 * This can be used in both windowed and full-screen mode.
 *
 * @threadAny
 */
- (void)draw
{
	__block VuoGraphicsLayer *l;
	VuoApp_executeOnMainThread(^{
		NSView *v = self.contentView;
		l = (VuoGraphicsLayer *)v.layer;
	});
	[l draw];
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

- (void)updateFullScreenMenu
{
	NSMenuItem *fullScreenMenuItem = [[[[[NSApplication sharedApplication] mainMenu] itemWithTag:VuoViewMenuItemTag] submenu] itemWithTag:VuoFullScreenMenuItemTag];
	fullScreenMenuItem.title = self.isFullScreen ? @"Exit Full Screen" :  @"Enter Full Screen";
}

/**
 * Updates the menu bar with this window's menus (View > Full Screen).
 *
 * @threadMain
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	if (!_isInMacFullScreenMode)
		[self setFullScreenPresentation:self.isFullScreen];

	NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
	_recordMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:@selector(toggleRecording) keyEquivalent:@"e"];
	[_recordMenuItem setKeyEquivalentModifierMask:NSCommandKeyMask|NSAlternateKeyMask];
	[fileMenu addItem:_recordMenuItem];
	NSMenuItem *fileMenuItem = [[NSMenuItem new] autorelease];
	[fileMenuItem setSubmenu:fileMenu];

	NSMenu *viewMenu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];
	NSMenuItem *fullScreenMenuItem = [[[NSMenuItem alloc] initWithTitle:@"" action:@selector(toggleFullScreen) keyEquivalent:@"f"] autorelease];
	fullScreenMenuItem.tag = VuoFullScreenMenuItemTag;
	[viewMenu addItem:fullScreenMenuItem];
	NSMenuItem *viewMenuItem = [[NSMenuItem new] autorelease];
	viewMenuItem.tag = VuoViewMenuItemTag;
	[viewMenuItem setSubmenu:viewMenu];

	NSMenu *windowMenu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];
	NSMenuItem *minimizeMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"] autorelease];
	NSMenuItem *zoomMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""] autorelease];
	NSMenuItem *cycleMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Cycle Through Windows" action:@selector(_cycleWindows:) keyEquivalent:@"`"] autorelease];
	[windowMenu addItem:minimizeMenuItem];
	[windowMenu addItem:zoomMenuItem];
	[windowMenu addItem:cycleMenuItem];
	NSMenuItem *windowMenuItem = [[NSMenuItem new] autorelease];
	[windowMenuItem setSubmenu:windowMenu];

	NSMutableArray *windowMenuItems = [NSMutableArray arrayWithCapacity:3];
	[windowMenuItems addObject:fileMenuItem];
	[windowMenuItems addObject:viewMenuItem];
	[windowMenuItems addObject:windowMenuItem];
	self.oldMenu = (NSMenu *)VuoApp_setMenuItems(windowMenuItems);

	[self updateFullScreenMenu];

	[self updateUI];
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)resignMainWindow
{
	[super resignMainWindow];

	// No need to change presentation when resigning, since:
	//    - other VuoGraphicsWindows change presentation on becomeMainWindow
	//    - Mac OS X changes presentation when another app becomes active
	//
	// Also, if two VuoGraphicsWindows are fullscreen on separate displays,
	// switching out of fullscreen presentation will cause the menubar and dock to flicker
	// when clicking from one display to another.
//	if (!_isInMacFullScreenMode)
//		[self setFullScreenPresentation:NO];

	VuoApp_setMenu(_oldMenu);
	self.oldMenu = nil;
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
 * Updates various UI elements to match current internal state.
 */
- (void)updateUI
{
	if (_recorder)
		_recordMenuItem.title = @"Stop Recording…";
	else
		_recordMenuItem.title = @"Start Recording";
}

/**
 * Sets up the window to call trigger functions.
 *
 * @threadAny
 */
- (void)enableUpdatedWindowTrigger:(VuoGraphicsWindowUpdatedWindowCallback)updatedWindow
{
	_updatedWindow = updatedWindow;

	VuoRenderedLayers rl = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_setWindow(rl, VuoWindowReference_make(self));
	_updatedWindow(rl);

	__block VuoGraphicsLayer *l;
	VuoApp_executeOnMainThread(^{
		NSView *v = self.contentView;
		l = (VuoGraphicsLayer *)v.layer;
	});
	[l enableTriggers];
}

/**
 * Sets up the window to call trigger functions.
 *
 * @threadAny
 * @deprecated
 */
- (void)enableShowedWindowTrigger:(VuoGraphicsWindowShowedWindowCallback)showedWindow requestedFrameTrigger:(VuoGraphicsWindowRequestedFrameCallback)requestedFrame
{
	_showedWindow = showedWindow;
	_showedWindow(VuoWindowReference_make(self));

	_requestedFrame = requestedFrame;

	__block VuoGraphicsLayer *l;
	VuoApp_executeOnMainThread(^{
		NSView *v = self.contentView;
		l = (VuoGraphicsLayer *)v.layer;
	});
	[l enableTriggers];
}

/**
 * Stops the window from calling trigger functions.
 *
 * @threadAny
 */
- (void)disableTriggers
{
	__block VuoGraphicsLayer *l;
	VuoApp_executeOnMainThread(^{
		NSView *v = self.contentView;
		l = (VuoGraphicsLayer *)v.layer;
	});
	[l disableTriggers];

	_updatedWindow = NULL;
	_showedWindow = NULL;
	_requestedFrame = NULL;
}

/**
 * Returns YES if this window is currently fullscreen.
 *
 * @threadAny
 */
- (bool)isFullScreen
{
	__block NSUInteger styleMask;
	VuoApp_executeOnMainThread(^{
		styleMask = self.styleMask;
	});
	return _isInMacFullScreenMode || styleMask == 0;
}


/**
 * Store the title, so we can re-apply it after returning from fullscreen mode.
 */
- (void)setTitle:(NSString *)title
{
	self.titleBackup = title;
	super.title = title;
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
		VDebugLog("%s", VuoWindowProperty_getSummary(property));

		if (property.type == VuoWindowProperty_Title)
		{
			self.title = property.title ? [NSString stringWithUTF8String:property.title] : @"";
			if (_updatedWindow)
			{
				VuoRenderedLayers rl = VuoRenderedLayers_makeEmpty();
				VuoRenderedLayers_setWindow(rl, VuoWindowReference_make(self));
				_updatedWindow(rl);
			}
		}
		else if (property.type == VuoWindowProperty_FullScreen)
		{
			NSScreen *requestedScreen = VuoScreen_getNSScreen(property.screen);

			bool isFullScreen = self.isFullScreen;
			bool wantsFullScreen = property.fullScreen;

			// Only go fullscreen if the specific requested screen exists.
			// https://b33p.net/kosada/node/14658
			if (wantsFullScreen && !requestedScreen)
				continue;

			NSInteger requestedDeviceId = [[[requestedScreen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
			NSInteger currentDeviceId = [[[self.screen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
			bool changingScreen = requestedDeviceId != currentDeviceId;

			if (isFullScreen && wantsFullScreen && changingScreen)
			{
				// Temporarily switch back to windowed mode so that we can switch to fullscreen on the new screen
				// (since macOS doesn't let us directly move from one fullscreen to another).
				[self setFullScreen:NO onScreen:nil];

				// If `System Preferences > Mission Control > Displays have separate Spaces` is enabled…
				if (NSScreen.screensHaveSeparateSpaces)
					// Give macOS a chance to complete its exit-fullscreen transition.
					self.shouldGoFullscreen = requestedScreen;
				else
					// If not, we can immediately go fullscreen on the new screen.
					[self setFullScreen:YES onScreen:requestedScreen];
			}
			else if (isFullScreen != wantsFullScreen)
				[self setFullScreen:property.fullScreen onScreen:requestedScreen];
			else if (requestedScreen && !isFullScreen)
				// Move the non-fullscreen window to the center of the specified screen.
				[self setFrameOrigin:(NSPoint){ NSMidX(requestedScreen.visibleFrame) - self.frame.size.width / 2,
												NSMidY(requestedScreen.visibleFrame) - self.frame.size.height /2 }];
		}
		else if (property.type == VuoWindowProperty_Position)
		{
			NSRect propertyInPoints = NSMakeRect(property.left, property.top, 0, 0);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = [self.contentView convertRectFromBacking:propertyInPoints];

			NSRect mainScreenRect = [[[NSScreen screens] objectAtIndex:0] frame];
			if ([self isFullScreen])
				_contentRectWhenWindowed.origin = NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - _contentRectWhenWindowed.size.height - propertyInPoints.origin.y);
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];
				self.frameOrigin = NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - contentRect.size.height - propertyInPoints.origin.y);
			}
		}
		else if (property.type == VuoWindowProperty_Size)
		{
			NSRect propertyInPoints = NSMakeRect(0, 0, property.width, property.height);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = [self.contentView convertRectFromBacking:propertyInPoints];
			_maintainsPixelSizeWhenBackingChanges = (property.unit == VuoCoordinateUnit_Pixels);

			if ([self isFullScreen])
				_contentRectWhenWindowed.size = propertyInPoints.size;
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];

				// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
				// (instead of its bottom-left corner as the system does by default).
				contentRect.origin.y += contentRect.size.height - propertyInPoints.size.height;

				contentRect.size = NSMakeSize(propertyInPoints.size.width, propertyInPoints.size.height);
				@try
				{
					_constrainToScreen = NO;
					[self setFrame:[self frameRectForContentRect:contentRect] display:YES animate:NO];
					_constrainToScreen = YES;
				}
				@catch (NSException *e)
				{
					VuoText description = VuoText_makeFromCFString(e.description);
					VuoLocal(description);
					VUserLog("Error: Couldn't change window size to %lldx%lld: %s", property.width, property.height, description);
				}
			}
		}
		else if (property.type == VuoWindowProperty_AspectRatio)
		{
			if (property.aspectRatio < 1./10000
			 || property.aspectRatio > 10000)
			{
				VUserLog("Error: Couldn't change window aspect ratio to %g since it's unrealistically narrow.", property.aspectRatio);
				continue;
			}

			NSRect contentRect = [self contentRectForFrameRect:[self frame]];
			[self setAspectRatioToWidth:contentRect.size.width height:contentRect.size.width/property.aspectRatio];
		}
		else if (property.type == VuoWindowProperty_AspectRatioReset)
			[self unlockAspectRatio];
		else if (property.type == VuoWindowProperty_Resizable)
		{
			[[self standardWindowButton:NSWindowZoomButton] setEnabled:property.resizable];
			if ([self isFullScreen])
				_styleMaskWhenWindowed = property.resizable ? (_styleMaskWhenWindowed | NSResizableWindowMask) : (_styleMaskWhenWindowed & ~NSResizableWindowMask);
			else
				self.styleMask =         property.resizable ? ([self styleMask]       | NSResizableWindowMask) : ([self styleMask]       & ~NSResizableWindowMask);
		}
		else if (property.type == VuoWindowProperty_Cursor)
		{
			_cursor = property.cursor;
			[self invalidateCursorRectsForView:self.contentView];
		}
		else if (property.type == VuoWindowProperty_Level)
		{
			if (property.level == VuoWindowLevel_Background)
				self.level = CGWindowLevelForKey(kCGBackstopMenuLevelKey);
			else if (property.level == VuoWindowLevel_Normal)
				self.level = CGWindowLevelForKey(kCGNormalWindowLevelKey);
			else if (property.level == VuoWindowLevel_Floating)
				self.level = CGWindowLevelForKey(kCGFloatingWindowLevelKey);
		}
	}
}

/**
 * -[NSWindow frame] isn't updated continually during a drag.
 * Apparently the old Carbon API is the only way to get its current size/position.
 *
 * Via Jonathan 'Wolf' Rentzsch http://rentzsch.com .
 */
- (NSRect)liveFrame
{
	Rect qdRect;
	extern OSStatus GetWindowBounds(WindowRef window, WindowRegionCode regionCode, Rect *globalBounds);

	GetWindowBounds([self windowRef], kWindowStructureRgn, &qdRect);

	return NSMakeRect(qdRect.left,
					  (float)CGDisplayPixelsHigh(kCGDirectMainDisplay) - qdRect.bottom,
					  qdRect.right - qdRect.left,
					  qdRect.bottom - qdRect.top);
}

/**
 * Constrains the window's aspect ratio to `pixelsWide/pixelsHigh`. Also resizes the
 * window to `pixelsWide` by `pixelsHigh`, unless:
 *
 * - the user has manually resized the window, in which case the user-set width is preserved
 * - the requested size is larger than the window's screen, in which case the window is scaled to fit the screen
 * - the requested size is smaller than Cocoa can properly render, in which case the size is clamped
 *
 * @threadMain
 */
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh
{
	pixelsWide = MAX(VuoGraphicsWindowMinSize, pixelsWide);
	pixelsHigh = MAX(VuoGraphicsWindowMinSize, pixelsHigh);

	NSSize newAspect = NSMakeSize(pixelsWide, pixelsHigh);
	if (NSEqualSizes([self contentAspectRatio], newAspect))
		return;

	// Sets the constraint when the user resizes the window (but doesn't affect the window's current size).
	self.contentAspectRatio = newAspect;

	if ([self isFullScreen])
	{
		if (_updatedWindow)
		{
			VuoRenderedLayers rl = VuoRenderedLayers_makeEmpty();
			VuoRenderedLayers_setWindow(rl, VuoWindowReference_make(self));
			_updatedWindow(rl);
		}
		if (_showedWindow)
			_showedWindow(VuoWindowReference_make(self));
		return;
	}

	CGFloat desiredWidth = pixelsWide;
	CGFloat desiredHeight = pixelsHigh;
	CGRect windowFrame = [self liveFrame];
	CGFloat aspectRatio = (CGFloat)pixelsWide / (CGFloat)pixelsHigh;

	// Adjust the width and height if the user manually resized the window.
	if (_userResizedWindow)
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

	_programmaticallyResizingWindow = YES;
	[self setFrame:newWindowFrame display:YES];
	_programmaticallyResizingWindow = NO;

	if (_updatedWindow)
	{
		VuoRenderedLayers rl = VuoRenderedLayers_makeEmpty();
		VuoRenderedLayers_setWindow(rl, VuoWindowReference_make(self));
		_updatedWindow(rl);
	}
	if (_showedWindow)
		_showedWindow(VuoWindowReference_make(self));
}

/**
 * Removes the aspect ratio constraint set by @ref VuoWindowOpenGl_setAspectRatio.
 *
 * @threadMain
 */
- (void)unlockAspectRatio
{
	self.resizeIncrements = NSMakeSize(1,1);
}

/**
 * Override NSWindow's default implementation,
 * so compositions can programmatically make the window size greater than or equal to the screen.
 */
- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen
{
	if (_constrainToScreen)
		return [super constrainFrameRect:frameRect toScreen:screen];
	else
		return frameRect;
}

/**
 * Enables/disables Vuo's custom fullscreen behavior (@see -setFullScreen:onScreen:).
 */
- (void)setFullScreenPresentation:(bool)enabled
{
	if (enabled)
	{
		// Don't raise the window level, since the window still covers the screen even when it's not focused.
//		self.level = NSScreenSaverWindowLevel;

		// Instead, just hide the dock and menu bar.
		((NSApplication *)NSApp).presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
	}
	else
	{
//		self.level = NSNormalWindowLevel;
		((NSApplication *)NSApp).presentationOptions = NSApplicationPresentationDefault;
	}
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
 *    - achieved by hiding the titlebar, menubar, and dock, and resizing the window
 *    - can go fullscreen on any user-specified VuoScreen
 *    - can have multiple windows fullscreen simultaneously (regardless of screensHaveSeparateSpaces setting)
 *    - if `screensHaveSeparateSpaces=NO`, menu bar reappears when focusing a non-fullscreen window (bad when doing a performance)
 *    - should not change the window level, since that prevents app switching and Mission Control from working properly
 * 3. `-[NSWindow toggleFullScreen:]`
 *    - a.k.a. Mac-fullscreen mode
 *    - a.k.a. the arrows in the top-right of the window on 10.9 and prior
 *    - a.k.a. the arrows inside the green button in the top-left of the window on 10.10 and later
 *    - optional in 10.7 – 10.10, mandatory in 10.11+
 *    - creates a space for every fullscreen window (some users consider this a feature)
 *    - painfully slow transition animation
 *    - if `screensHaveSeparateSpaces=NO`, can only have one window fullscreen (other screen is blank)
 *
 * `screensHaveSeparateSpaces` is `System Preferences > Mission Control > Displays have separate Spaces`.
 *
 * Vuo uses #3 if it allows simultaneous fullscreen windows on separate displays
 * (i.e., when both running on 10.9+ and screensHaveSeparateSpaces=YES),
 * and otherwise uses #2 (which allows simultaneous fullscreen windows on separate displays regardless of OS version and settings).
 *
 * https://b33p.net/kosada/node/8729
 * https://b33p.net/kosada/node/8730
 * https://b33p.net/kosada/node/11899
 *
 * @threadMain
 */
- (void)setFullScreen:(BOOL)wantsFullScreen onScreen:(NSScreen *)screen
{
	_programmaticallyResizingWindow = true;

	if (wantsFullScreen && ![self isFullScreen])
	{
		// Switch into fullscreen mode.


		// Save the position, size, and style, to be restored when switching back to windowed mode.
		_contentRectWhenWindowed = [self contentRectForFrameRect:self.frame];
		_styleMaskWhenWindowed = [self styleMask];

		// Move the window to the specified screen.
		// (Necessary even for Mac-fullscreen mode, since it fullscreens on whichever screen the window is currently on.)
		if (screen && ![[self screen] isEqualTo:screen])
		{
			NSRect windowFrame = self.frame;
			NSRect screenFrame = screen.frame;
			self.frameOrigin = NSMakePoint(screenFrame.origin.x + screenFrame.size.width/2  - windowFrame.size.width/2,
										   screenFrame.origin.y + screenFrame.size.height/2 - windowFrame.size.height/2);
		}


		// Use Mac-fullscreen mode by default, since it's compatible with Mission Control,
		// and since (unlike Kiosk Mode) the menu bar stays hidden on the fullscreen display
		// when you focus a window on a different display (such as when livecoding during a performance).
		bool useMacFullScreenMode = true;

		// If `System Preferences > Mission Control > Displays have separate Spaces` is unchecked,
		// only a single window could go Mac-fullscreen at once, so don't use Mac-fullscreen mode in that case.
		if (!NSScreen.screensHaveSeparateSpaces)
			useMacFullScreenMode = false;


		if (useMacFullScreenMode)
		{
			_isInMacFullScreenMode = YES;

			// If a size-locked window enters Mac-fullscreen mode,
			// its height increases upon exiting Mac-fullscreen mode.
			// Mark it resizeable while fullscreen, to keep its size from changing (!).
			self.styleMask |= NSResizableWindowMask;

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_semaphore_wait(VuoGraphicsWindow_fullScreenTransitionSemaphore, DISPATCH_TIME_FOREVER);
				dispatch_async(dispatch_get_main_queue(), ^{
					_programmaticallyTransitioningFullScreen = true;
					[self toggleFullScreen:nil];
					[self updateFullScreenMenu];
				});
			});
		}
		else
		{
			[self setFullScreenPresentation:YES];

			NSSize car = self.contentAspectRatio;
			self.styleMask = 0;
			if (!NSEqualSizes(car, NSMakeSize(0,0)))
				self.contentAspectRatio = car;

			// Make the window take up the whole screen.
			[self setFrame:self.screen.frame display:YES];

			[self finishFullScreenTransition];
			[self updateFullScreenMenu];
		}
	}
	else if (!wantsFullScreen && [self isFullScreen])
	{
		// Switch out of fullscreen mode.

		if (_isInMacFullScreenMode)
		{
			_isInMacFullScreenMode = NO;

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_semaphore_wait(VuoGraphicsWindow_fullScreenTransitionSemaphore, DISPATCH_TIME_FOREVER);
				dispatch_async(dispatch_get_main_queue(), ^{
					_programmaticallyTransitioningFullScreen = true;
					[self toggleFullScreen:nil];
					self.styleMask = _styleMaskWhenWindowed;
					[self updateFullScreenMenu];
				});
			});
		}
		else
		{
			[self setFullScreenPresentation:NO];

			NSSize car = self.contentAspectRatio;
			self.styleMask = _styleMaskWhenWindowed;
			self.title = _titleBackup;
			if (!NSEqualSizes(car, NSMakeSize(0,0)))
				self.contentAspectRatio = car;

			[self setFrame:[self frameRectForContentRect:_contentRectWhenWindowed] display:YES];

			[self finishFullScreenTransition];
			[self updateFullScreenMenu];
		}
	}
}

/**
 * Ends the programmatic-resizing event-block, and explicitly fires the resize handler once.
 *
 * To be called when the delegate receives `-windowDidEnter/ExitFullScreen:`.
 */
- (void)finishFullScreenTransition
{
	_programmaticallyResizingWindow = false;

	[self.delegate windowDidResize:nil];

	[self makeFirstResponder:self];

	if (self.shouldGoFullscreen)
		dispatch_async(dispatch_get_main_queue(), ^{
			NSScreen *s = [self.shouldGoFullscreen retain];
			self.shouldGoFullscreen = nil;
			[self setFullScreen:true onScreen:s];
			[s release];
		});
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)toggleFullScreen
{
	[self setFullScreen:![self isFullScreen] onScreen:nil];
}

/**
 * Resizes the window's content area to `newSize` (in points)
 * when it's safe to do so.
 */
- (void)scheduleResize:(NSSize)newSize
{
	VDebugLog("Requested to resize to %gx%g points.",newSize.width,newSize.height);
	if (_leftMouseDown)
	{
		VDebugLog("    The window is being dragged; deferring the resize.");
		_pendingResize = newSize;
	}
	else
	{
		VDebugLog("    The window is not being dragged; resizing now.");
		self.contentSize = newSize;
	}
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
		_recorder = [[VuoWindowRecorder alloc] initWithWindow:self url:_temporaryMovieURL];

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

	char *title = VuoApp_getName();
	sp.nameFieldStringValue = [NSString stringWithUTF8String:title];
	free(title);

	if ([sp runModal] == NSFileHandlingPanelCancelButton)
		goto done;

	NSError *error;
	if (![[NSFileManager defaultManager] moveItemAtURL:_temporaryMovieURL toURL:[sp URL] error:&error])
	{
		if ([error code] == NSFileWriteFileExistsError)
		{
			// File exists.  Since, in the NSSavePanel, the user said to Replace, try replacing it.
			if (![[NSFileManager defaultManager] replaceItemAtURL:[sp URL]
					  withItemAtURL:_temporaryMovieURL
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

	[self setFullScreen:NO onScreen:nil];

	[self promptToSaveMovie];
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
 * Called when the window is being closed (and thus rendering should stop).
 */
- (void)close
{
	NSView *v = self.contentView;
	VuoGraphicsLayer *gv = (VuoGraphicsLayer *)v.layer;
	[gv close];

	[super close];
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	[NSEvent removeMonitor:_mouseMonitor];

	// https://b33p.net/kosada/node/12123
	// Cocoa leaks the contentView unless we force it to resign firstResponder status.
	[self makeFirstResponder:nil];

	[super dealloc];
}

@end
