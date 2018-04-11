/**
 * @file
 * VuoMouse implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <AppKit/AppKit.h>

#include "VuoMouse.h"
#include "VuoGraphicsWindow.h"
#include "VuoGraphicsView.h"
#include "VuoApp.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoMouse",
					"dependencies" : [
						"VuoApp",
						"VuoBoolean",
						"VuoGraphicsView",
						"VuoGraphicsWindow",
						"VuoModifierKey",
						"VuoMouseButton",
						"VuoPoint2d",
						"VuoWindow",
						"VuoWindowReference",
						"AppKit.framework"
					]
				 });
#endif


/**
 *	Get the full screen width and height in pixels.
 *
 * @threadNoMain
 */
void VuoMouse_GetScreenDimensions(int64_t *width, int64_t *height)
{
	__block NSScreen *screen = nil;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  screen = [NSScreen mainScreen];
				  });
	NSDictionary *description = [screen deviceDescription];
	NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];

	*width = (int)displayPixelSize.width;
	*height = (int)displayPixelSize.height;
}

/**
 * Handle for starting and stopping event listeners.
 */
struct VuoMouseContext
{
	id monitor;  ///< The handle returned by NSEvent's method to start monitoring events, to be used to stop monitoring.
	id monitor2;  ///< Like `monitor`, but listens for control-leftclicks (when needed).

	dispatch_queue_t clickQueue;  ///< Synchronizes handling of click events.
	dispatch_group_t clickGroup;  ///< Synchronizes handling of click events.
	int pendingClickCount;  ///< The number of clicks so far in the in-progress series of clicks, or 0 if none in progress.
};

/**
 * Creates a handle for starting and stopping event listeners.
 */
VuoMouse * VuoMouse_make(void)
{
	// https://b33p.net/kosada/node/11966
	// Mouse events are only received if the process is in app mode.
	VuoApp_init();

	struct VuoMouseContext *context = (struct VuoMouseContext *)calloc(1, sizeof(struct VuoMouseContext));
	VuoRegister(context, free);
	return (VuoMouse *)context;
}


/**
 * Converts a position relative to the window into a position relative to the current screen.
 */
static VuoPoint2d VuoMouse_convertWindowToScreenCoordinates(NSPoint pointInWindow, NSWindow *window, bool *shouldFire)
{
	NSRect rectInWindow = NSMakeRect(pointInWindow.x, pointInWindow.y, 0, 0);
	NSPoint pointInScreen = window ? [window convertRectToScreen:rectInWindow].origin : pointInWindow;

	*shouldFire = false;
	NSInteger windowNumberForPoint = [NSWindow windowNumberAtPoint:pointInScreen belowWindowWithWindowNumber:0];
	for (NSWindow *w in [NSApp windows])
		if ([w windowNumber] == windowNumberForPoint
		 && [w isKindOfClass:[VuoGraphicsWindow class]]
		 && NSPointInRect(pointInScreen, [w convertRectToScreen:[[w contentView] frame]]))
		{
			*shouldFire = true;
			break;
		}

	pointInScreen.y = [[NSScreen mainScreen] frame].size.height - pointInScreen.y;

	return VuoPoint2d_make(pointInScreen.x, pointInScreen.y);
}

/**
 * Converts a position relative to a view and in view coordinates into a position in Vuo coordinates.
 */
static VuoPoint2d VuoMouse_convertViewToVuoCoordinates(NSPoint pointInView, NSView *view)
{
	NSRect bounds = [view convertRectFromBacking:((VuoGraphicsView *)view).viewport];
	VuoGraphicsWindow *w = (VuoGraphicsWindow *)[view window];
	if (w.isFullScreen)
	{
		// If the view is fullscreen, its origin might not line up with the screen's origin
		// (such as when the window is aspect-locked or size-locked).
		NSPoint windowOrigin = [w frameRectForContentRect:w.frame].origin;
		NSPoint screenOrigin = w.screen.frame.origin;
		bounds.origin.x += windowOrigin.x - screenOrigin.x;
		bounds.origin.y += windowOrigin.y - screenOrigin.y;
	}

	VuoPoint2d pointInVuo;
	double viewMaxX = NSWidth(bounds) - 1;
	double viewMaxY = NSHeight(bounds) - 1;
	pointInVuo.x = ((pointInView.x - NSMinX(bounds) - viewMaxX /2.) * 2.) / viewMaxX;
	pointInVuo.y = ((pointInView.y - NSMinY(bounds) - viewMaxY/2.) * 2.) / viewMaxX;
	pointInVuo.y /= (NSWidth(bounds)/viewMaxX) * (viewMaxY/NSHeight(bounds));
	return pointInVuo;
}

/**
 * Converts a position in screen coordinates into a position relative to the the window's fullscreen view and in Vuo coordinates.
 */
static VuoPoint2d VuoMouse_convertFullScreenToVuoCoordinates(NSPoint pointInScreen, NSWindow *window, bool *isInScreen)
{
	NSRect fullScreenFrame = [[window screen] frame];

	// [NSEvent mouseLocation] returns points between [0, screenSize] (inclusive!?)
	// (i.e., somehow Cocoa invents an extra row and column of pixels).
	// Scale to [0, screenSize-1],
	// then quantize to pixels to get exact coordinate values when the mouse hits the edges.
	pointInScreen.x = VuoReal_snap(pointInScreen.x * fullScreenFrame.size.width  / (fullScreenFrame.size.width+1),  0, 1);
	pointInScreen.y = VuoReal_snap(pointInScreen.y * fullScreenFrame.size.height / (fullScreenFrame.size.height+1), 0, 1);

	NSView *view = [window contentView];
	NSPoint pointInView = NSMakePoint(pointInScreen.x - fullScreenFrame.origin.x,
									  pointInScreen.y - fullScreenFrame.origin.y);
	*isInScreen = NSPointInRect(pointInScreen, fullScreenFrame);
	return VuoMouse_convertViewToVuoCoordinates(pointInView, view);
}

/**
 * Converts a position relative to the window and in window coordinates into
 * a position relative to the window's content view and in Vuo coordinates.
 */
VuoPoint2d VuoMouse_convertWindowToVuoCoordinates(NSPoint pointInWindow, NSWindow *window, bool *shouldFire)
{
	// https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CocoaDrawingGuide/Transforms/Transforms.html says:
	// "Cocoa event objects return y coordinate values that are 1-based (in window coordinates) instead of 0-based.
	// Thus, a mouse click on the bottom left corner of a window or view would yield the point (0, 1) in Cocoa and not (0, 0).
	// Only y-coordinates are 1-based."
	// …so, compensate for that.
	--pointInWindow.y;

	// Then quantize to pixels to get exact coordinate values when the mouse hits the edges of the window.
	pointInWindow.x = VuoReal_snap(pointInWindow.x, 0, 1);
	pointInWindow.y = VuoReal_snap(pointInWindow.y, 0, 1);

	NSView *view = [window contentView];
	NSPoint pointInView = [view convertPoint:pointInWindow fromView:nil];
	*shouldFire = NSPointInRect(pointInView, [view frame]);
	return VuoMouse_convertViewToVuoCoordinates(pointInView, view);
}

/**
 * Converts a change in position in window coordinates into a change in position in Vuo coordinates.
 */
static VuoPoint2d VuoMouse_convertDeltaToVuoCoordinates(NSPoint delta, NSWindow *window)
{
	NSView *view = [window contentView];
	NSRect bounds = [view bounds];
	VuoPoint2d deltaInVuo;
	deltaInVuo.x = (delta.x * 2.) / bounds.size.width;
	deltaInVuo.y = -(delta.y * 2.) / bounds.size.width;
	return deltaInVuo;
}

/**
 * Returns true if the mouse is potentially involved in a window resize
 * (and thus we should ignore the button).
 */
bool VuoMouse_isResizing(void)
{
	// https://b33p.net/kosada/node/11580
	// We shouldn't report mouse button presses while resizing the window.
	// I went through all the data that NSEvent (and the underlying CGEvent) provides,
	// but couldn't find anything different between normal mouse-down events
	// and mouse-down events that led to resizing the window.
	// Disgusting hack: check whether the system has overridden the application's
	// mouse cursor and turned it into a resize arrow.
	return !NSEqualPoints([[NSCursor currentCursor] hotSpot], [[NSCursor currentSystemCursor] hotSpot]);
}

/**
 * If the mouse was scrolled a non-zero amount, calls the trigger function and passes it the scroll delta.
 */
static void VuoMouse_fireScrollDeltaIfNeeded(NSEvent *event, VuoWindowReference windowRef, VuoModifierKey modifierKey, void (^scrolled)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	VuoPoint2d delta = VuoPoint2d_make(-[event deltaX], [event deltaY]);
	if (fabs(delta.x) < 0.00001 && fabs(delta.y) < 0.00001)
		return;

	bool shouldFire = false;
	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (targetWindow)
	{
		if (targetWindow == [event window])
			shouldFire = true;
	}
	else
		shouldFire = true;

	if (shouldFire)
		scrolled(delta);
}

/**
 * If the mouse event is not ignored, calls the block and passes it the mouse position.
 *
 * If a window is given, the mouse position is in Vuo coordinates relative to the window's content view.
 * The mouse event is ignored if it doesn't correspond to this window.
 *
 * If no window is given, the mouse position is in screen coordinates.
 */
static void VuoMouse_fireMousePositionIfNeeded(NSEvent *event, NSPoint fullscreenPoint, VuoWindowReference windowRef, VuoModifierKey modifierKey, bool fireRegardlessOfPosition, void (^fire)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	bool shouldFire = false;
	VuoPoint2d convertedPoint;
	NSPoint pointInWindowOrScreen = [event locationInWindow];
	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (targetWindow)
	{
		if (((VuoGraphicsWindow *)targetWindow).isFullScreen)
		{
			// https://b33p.net/kosada/node/8489
			// When in fullscreen mode with multiple displays, -[event locationInWindow] produces inconsistent results,
			// so use +[NSEvent mouseLocation] which seems to work with both single and multiple displays.
			pointInWindowOrScreen = fullscreenPoint;
			convertedPoint = VuoMouse_convertFullScreenToVuoCoordinates(pointInWindowOrScreen, targetWindow, &shouldFire);
		}
		else if (targetWindow == [event window])
			convertedPoint = VuoMouse_convertWindowToVuoCoordinates(pointInWindowOrScreen, targetWindow, &shouldFire);
		else if (! [event window] && [targetWindow isKeyWindow])
		{
			// Workaround (https://b33p.net/kosada/node/7545):
			// [event window] becomes nil for some reason after mousing over certain other applications.
			NSRect rectInWindowOrScreen = NSMakeRect(pointInWindowOrScreen.x, pointInWindowOrScreen.y, 0, 0);
			NSPoint pointInWindow = [targetWindow convertRectFromScreen:rectInWindowOrScreen].origin;
			convertedPoint = VuoMouse_convertWindowToVuoCoordinates(pointInWindow, targetWindow, &shouldFire);
		}
		else
			// Our window isn't focused, so ignore this event.
			return;

		shouldFire = shouldFire && !VuoMouse_isResizing();
	}
	else
		convertedPoint = VuoMouse_convertWindowToScreenCoordinates(pointInWindowOrScreen, [event window], &shouldFire);

	if (shouldFire || fireRegardlessOfPosition)
		fire(convertedPoint);
}

/**
 * If the mouse event is not ignored, calls the trigger function and passes it the mouse delta (change in position).
 *
 * If a window is given, the mouse delta is in Vuo coordinates relative to the window's content view.
 * The mouse event is ignored if it doesn't correspond to this window.
 *
 * If no window is given, the mouse delta is in screen coordinates.
 */
static void VuoMouse_fireMouseDeltaIfNeeded(NSEvent *event, VuoWindowReference windowRef, VuoModifierKey modifierKey, void (*fire)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	bool shouldFire = false;
	VuoPoint2d convertedDelta;
	NSPoint deltaInScreen = NSMakePoint([event deltaX], [event deltaY]);
	if (fabs(deltaInScreen.x) < 0.0001
	 && fabs(deltaInScreen.y) < 0.0001)
		return;

	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (targetWindow)
	{
		if (((VuoGraphicsWindow *)targetWindow).isFullScreen)
		{
			// https://b33p.net/kosada/node/8489
			// When in fullscreen mode with multiple displays, -[event locationInWindow] produces inconsistent results,
			// so use +[NSEvent mouseLocation] which seems to work with both single and multiple displays.
			NSPoint pointInWindowOrScreen = [NSEvent mouseLocation];
			VuoMouse_convertFullScreenToVuoCoordinates(pointInWindowOrScreen, targetWindow, &shouldFire);
			convertedDelta = VuoMouse_convertDeltaToVuoCoordinates(deltaInScreen, targetWindow);
		}
		else if (targetWindow == [event window] || (! [event window] && [targetWindow isKeyWindow]))
		{
			// Workaround (https://b33p.net/kosada/node/7545):
			// [event window] becomes nil for some reason after mousing over certain other applications.
			shouldFire = true;
			convertedDelta = VuoMouse_convertDeltaToVuoCoordinates(deltaInScreen, targetWindow);
		}
	}
	else
	{
		shouldFire = true;
		convertedDelta = VuoPoint2d_make(deltaInScreen.x, deltaInScreen.y);
	}

	if (shouldFire)
		fire(convertedDelta);
}

/**
 * If the mouse click event is not ignored, calls the trigger function and passes it the mouse position.
 *
 * If a window is given, the mouse position is in Vuo coordinates relative to the window's content view.
 * The mouse event is ignored if it doesn't correspond to this window.
 *
 * If no window is given, the mouse position is in screen coordinates.
 */
static void VuoMouse_fireMouseClickIfNeeded(struct VuoMouseContext *context, NSEvent *event, VuoWindowReference windowRef, VuoModifierKey modifierKey,
											void (*singleClicked)(VuoPoint2d), void (*doubleClicked)(VuoPoint2d), void (*tripleClicked)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	NSInteger clickCount = [event clickCount];
	NSPoint fullscreenPoint = [NSEvent mouseLocation];
	NSTimeInterval clickIntervalInSeconds = [NSEvent doubleClickInterval];
	dispatch_time_t clickInterval = dispatch_time(DISPATCH_TIME_NOW, clickIntervalInSeconds * NSEC_PER_SEC);
	context->pendingClickCount = clickCount;

	void (*fired)(VuoPoint2d) = NULL;
	if (clickCount == 1)
		fired = singleClicked;
	else if (clickCount == 2)
		fired = doubleClicked;
	else if (clickCount >= 3)
		fired = tripleClicked;
	if (! fired)
		return;

	dispatch_group_enter(context->clickGroup);
	dispatch_after(clickInterval, context->clickQueue, ^{
					   if (context->pendingClickCount == clickCount)
					   {
						   VuoMouse_fireMousePositionIfNeeded(event, fullscreenPoint, windowRef, modifierKey, false, ^(VuoPoint2d point){ fired(point); });
						   context->pendingClickCount = 0;
					   }
					   dispatch_group_leave(context->clickGroup);
				   });
}


/**
 * Starts listening for scroll events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForScrolls(VuoMouse *mouseListener, void (*scrolled)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSScrollWheelMask handler:^(NSEvent *event) {
		VuoMouse_fireScrollDeltaIfNeeded(event, window, modifierKey, ^(VuoPoint2d point){ scrolled(point); });
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * Starts listening for scroll events, and calling the scrolled block for each one.
 */
void VuoMouse_startListeningForScrollsWithCallback(VuoMouse *mouseListener, void (^scrolled)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSScrollWheelMask handler:^(NSEvent *event) {
		VuoMouse_fireScrollDeltaIfNeeded(event, window, modifierKey, scrolled);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * If `window` is non-NULL and `modifierKey` is Any or None, invokes `movedTo` with the mouse's current position.
 */
static void VuoMouse_fireInitialEvent(void (^movedTo)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	if (window && (modifierKey == VuoModifierKey_Any || modifierKey == VuoModifierKey_None))
	{
		VuoMouseStatus_use();

		VuoPoint2d position;
		VuoBoolean isPressed;
		if (VuoMouse_getStatus(&position, &isPressed, VuoMouseButton_Any, window, VuoModifierKey_Any, false))
			movedTo(position);

		VuoMouseStatus_disuse();
	}
}

/**
 * Starts listening for mouse move events, and calling the trigger function (with mouse position) for each one,.
 *
 * If `window` is non-NULL and `modifierKey` is Any or None, immediately invokes `movedTo` with the mouse's current position.
 */
void VuoMouse_startListeningForMoves(VuoMouse *mouseListener, void (*movedTo)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSMouseMovedMask|NSLeftMouseDraggedMask|NSRightMouseDraggedMask|NSOtherMouseDraggedMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, modifierKey, true, ^(VuoPoint2d point){ movedTo(point); });
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;

	VuoMouse_fireInitialEvent(^(VuoPoint2d point){ movedTo(point); }, window, modifierKey);
}

/**
 * Starts listening for mouse move events, and calling the given block for each one.
 *
 * If `window` is non-NULL and `modifierKey` is Any or None, immediately invokes `movedTo` with the mouse's current position.
 */
void VuoMouse_startListeningForMovesWithCallback(VuoMouse *mouseListener, void (^movedTo)(VuoPoint2d),
									 VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSMouseMovedMask|NSLeftMouseDraggedMask|NSRightMouseDraggedMask|NSOtherMouseDraggedMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, modifierKey, true, movedTo);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;

	VuoMouse_fireInitialEvent(movedTo, window, modifierKey);
}

/**
 * Starts listening for mouse move events, and calling the trigger function (with change in mouse position) for each one.
 */
void VuoMouse_startListeningForDeltas(VuoMouse *mouseListener, void (*movedBy)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSMouseMovedMask|NSLeftMouseDraggedMask|NSRightMouseDraggedMask|NSOtherMouseDraggedMask handler:^(NSEvent *event) {
		VuoMouse_fireMouseDeltaIfNeeded(event, window, modifierKey, movedBy);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * Starts listening for mouse drag events, and calling the given block for each one.
 */
void VuoMouse_startListeningForDragsWithCallback(VuoMouse *mouseListener, void (^dragMovedTo)(VuoPoint2d),
												 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;

	NSEventMask eventMask = 0;
	switch (button)
	{
		case VuoMouseButton_Left:
			eventMask = NSLeftMouseDraggedMask;
			break;
		case VuoMouseButton_Middle:
			eventMask = NSOtherMouseDraggedMask;
			break;
		case VuoMouseButton_Right:
			eventMask = NSRightMouseDraggedMask;

			// Also fire events for control-leftclicks.
			context->monitor2 = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseDraggedMask handler:^(NSEvent *event) {
					VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, VuoModifierKey_Control, fireRegardlessOfPosition, dragMovedTo);
					return event;
				}];

			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseDraggedMask | NSOtherMouseDraggedMask | NSRightMouseDraggedMask;
			break;
	}

	context->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
			VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, modifierKey, fireRegardlessOfPosition, dragMovedTo);
			return event;
		}];
}

/**
 * Starts listening for mouse drag events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForDrags(VuoMouse *mouseListener, void (*dragMovedTo)(VuoPoint2d), VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	VuoMouse_startListeningForDragsWithCallback(mouseListener, ^(VuoPoint2d point){ dragMovedTo(point); }, button, window, modifierKey, true);
}

/**
 * Starts listening for mouse press events, and calling the given block for each one.
 */
void VuoMouse_startListeningForPressesWithCallback(VuoMouse *mouseListener, void (^pressed)(VuoPoint2d),
												   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;

	NSEventMask eventMask = 0;
	switch (button)
	{
		case VuoMouseButton_Left:
			eventMask = NSLeftMouseDownMask;
			break;
		case VuoMouseButton_Middle:
			eventMask = NSOtherMouseDownMask;
			break;
		case VuoMouseButton_Right:
			eventMask = NSRightMouseDownMask;

			// Also fire events for control-leftclicks.
			context->monitor2 = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseDownMask handler:^(NSEvent *event) {
					VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, VuoModifierKey_Control, false, pressed);
					return event;
				}];

			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseDownMask | NSOtherMouseDownMask | NSRightMouseDownMask;
			break;
	}

	context->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
			VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, modifierKey, false, pressed);
			return event;
		}];
}

/**
 * Starts listening for mouse press events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForPresses(VuoMouse *mouseListener, void (*pressed)(VuoPoint2d), VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	VuoMouse_startListeningForPressesWithCallback(mouseListener, ^(VuoPoint2d point){ pressed(point); }, button, window, modifierKey);
}

/**
 * Starts listening for mouse release events, and calling the given block for each one.
 */
void VuoMouse_startListeningForReleasesWithCallback(VuoMouse *mouseListener, void (^released)(VuoPoint2d),
													VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;

	NSEventMask eventMask = 0;
	switch (button)
	{
		case VuoMouseButton_Left:
			eventMask = NSLeftMouseUpMask;
			break;
		case VuoMouseButton_Middle:
			eventMask = NSOtherMouseUpMask;
			break;
		case VuoMouseButton_Right:
			eventMask = NSRightMouseUpMask;

			// Also fire events for control-leftclicks.
			context->monitor2 = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseUpMask handler:^(NSEvent *event) {
					VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, VuoModifierKey_Control, fireRegardlessOfPosition, released);
					return event;
				}];

			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseUpMask | NSOtherMouseUpMask | NSRightMouseUpMask;
			break;
	}

	context->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
			VuoMouse_fireMousePositionIfNeeded(event, [NSEvent mouseLocation], window, modifierKey, fireRegardlessOfPosition, released);
			return event;
		}];
}

/**
 * Starts listening for mouse release events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForReleases(VuoMouse *mouseListener, void (*released)(VuoPoint2d), VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition)
{
	VuoMouse_startListeningForReleasesWithCallback(mouseListener, ^(VuoPoint2d point){ released(point); }, button, window, modifierKey, fireRegardlessOfPosition);
}

/**
 * Starts listening for mouse click events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForClicks(VuoMouse *mouseListener, void (*singleClicked)(VuoPoint2d), void (*doubleClicked)(VuoPoint2d), void (*tripleClicked)(VuoPoint2d),
											VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->clickQueue = dispatch_queue_create("vuo.mouse.click", 0);
	context->clickGroup = dispatch_group_create();
	context->pendingClickCount = 0;

	NSEventMask eventMask = 0;
	switch (button)
	{
		case VuoMouseButton_Left:
			eventMask = NSLeftMouseUpMask;
			break;
		case VuoMouseButton_Middle:
			eventMask = NSOtherMouseUpMask;
			break;
		case VuoMouseButton_Right:
			eventMask = NSRightMouseUpMask;

			// Also fire events for control-leftclicks.
			context->monitor2 = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseUpMask handler:^(NSEvent *event) {
					VuoMouse_fireMouseClickIfNeeded(context, event, window, VuoModifierKey_Control, singleClicked, doubleClicked, tripleClicked);
					return event;
				}];

			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseUpMask | NSOtherMouseUpMask | NSRightMouseUpMask;
			break;
	}

	context->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
			VuoMouse_fireMouseClickIfNeeded(context, event, window, modifierKey, singleClicked, doubleClicked, tripleClicked);
			return event;
		}];
}


/**
 * Stops listening for mouse events for the given handle.
 */
void VuoMouse_stopListening(VuoMouse *mouseListener)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{  // wait for any in-progress monitor handlers to complete
						VUOLOG_PROFILE_END(mainQueue);
						if (context->monitor)
						{
							[NSEvent removeMonitor:context->monitor];
							context->monitor = nil;

							if (context->monitor2)
							{
								[NSEvent removeMonitor:context->monitor2];
								context->monitor2 = nil;
							}
						}
				  });

	if (context->clickGroup)
	{
		dispatch_group_wait(context->clickGroup, DISPATCH_TIME_FOREVER);  // wait for any pending click events to complete
		dispatch_release(context->clickQueue);
		dispatch_release(context->clickGroup);
		context->clickQueue = NULL;
		context->clickGroup = NULL;
	}
}

static unsigned int VuoMouseStatus_useCount = 0;		///< Process-wide count of callers (typically node instances) using @ref VuoMouse_getStatus.
static id VuoMouseStatus_monitor            = nil;		///< Process-wide mouse event monitor for @ref VuoMouse_getStatus.
static NSPoint VuoMouseStatus_position;					///< The mouse's position in screen coordinates, as reported by the most recent non-filtered event.
static bool VuoMouseStatus_leftButton       = false;	///< The mouse's current button status, as reported by recent non-filtered events.
static bool VuoMouseStatus_middleButton     = false;	///< The mouse's current button status, as reported by recent non-filtered events.
static bool VuoMouseStatus_rightButton      = false;	///< The mouse's current button status, as reported by recent non-filtered events.

/**
 * Starts tracking mouse events to later be reported by @ref VuoMouse_getStatus.
 */
void VuoMouseStatus_use(void)
{
	if (__sync_add_and_fetch(&VuoMouseStatus_useCount, 1) == 1)
	{
		// https://b33p.net/kosada/node/11966
		// Mouse events are only received if the process is in app mode.
		VuoApp_init();

		// Ensure +[NSEvent mouseLocation] is called for the first time on the main thread, else we get console message
		// "!!! BUG: The current event queue and the main event queue are not the same. Events will not be handled correctly. This is probably because _TSGetMainThread was called for the first time off the main thread."
		VuoApp_executeOnMainThread(^{
			VuoMouseStatus_position = [NSEvent mouseLocation];
		});
		VuoMouseStatus_leftButton   = false;
		VuoMouseStatus_middleButton = false;
		VuoMouseStatus_rightButton  = false;
		VuoMouseStatus_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:
				NSMouseMovedMask
				|NSLeftMouseDraggedMask|NSRightMouseDraggedMask|NSOtherMouseDraggedMask
				|NSLeftMouseDownMask|NSOtherMouseDownMask|NSRightMouseDownMask
				|NSLeftMouseUpMask|NSOtherMouseUpMask|NSRightMouseUpMask
			handler:^(NSEvent *event) {
				// Despite the name, -locationInWindow sometimes returns screen coordinates.
				NSPoint p = [event locationInWindow];
				NSWindow *window = [event window];
				if (window)
				{
					NSRect rectInWindow = NSMakeRect(p.x, p.y, 0, 0);
					VuoMouseStatus_position = [window convertRectToScreen:rectInWindow].origin;
				}
				else
					VuoMouseStatus_position = p;

				bool reportPress = !VuoMouse_isResizing();
				switch ([event type])
				{
					case NSLeftMouseDown:   if (reportPress) VuoMouseStatus_leftButton   = true;  break;
					case NSLeftMouseUp:                      VuoMouseStatus_leftButton   = false; break;
					case NSOtherMouseDown:  if (reportPress) VuoMouseStatus_middleButton = true;  break;
					case NSOtherMouseUp:                     VuoMouseStatus_middleButton = false; break;
					case NSRightMouseDown:  if (reportPress) VuoMouseStatus_rightButton  = true;  break;
					case NSRightMouseUp:                     VuoMouseStatus_rightButton  = false; break;
					default: ;
				}

				return event;
			}];
	}
}

/**
 * Stops tracking mouse events for @ref VuoMouse_getStatus.
 */
void VuoMouseStatus_disuse(void)
{
	if (__sync_sub_and_fetch(&VuoMouseStatus_useCount, 1) == 0)
	{
		[NSEvent removeMonitor:VuoMouseStatus_monitor];
		VuoMouseStatus_monitor = nil;
	}
}

/**
 * Outputs the current mouse position and whether the mouse is currently pressed.
 *
 * If `onlyUpdateWhenActive` is true, and the application is not active, then neither `position` nor `isPressed` is modified.
 *
 * If a window is given, but it's not the key window or the mouse is not within the window, then
 * @a position is modified but @a isPressed is not.
 *
 * If a window is given, the mouse position is in Vuo coordinates relative to the window's content view.
 * Otherwise, the mouse position is in screen coordinates.
 *
 * Returns true if `position` was updated.
 */
bool VuoMouse_getStatus(VuoPoint2d *position, VuoBoolean *isPressed,
						VuoMouseButton button, VuoWindowReference windowRef, VuoModifierKey modifierKey,
						bool onlyUpdateWhenActive)
{
	if (onlyUpdateWhenActive && ! [NSApp isActive])
		return false;

	bool shouldTrackPresses;
	if (windowRef)
	{
		VuoGraphicsWindow *targetWindow = (VuoGraphicsWindow *)windowRef;
		NSPoint pointInScreen = VuoMouseStatus_position;

		if (((VuoGraphicsWindow *)targetWindow).isFullScreen)
			*position = VuoMouse_convertFullScreenToVuoCoordinates(pointInScreen, targetWindow, &shouldTrackPresses);
		else
		{
			NSRect rectInScreen = NSMakeRect(pointInScreen.x, pointInScreen.y, 0, 0);
			NSPoint pointInWindow = [targetWindow convertRectFromScreen:rectInScreen].origin;
			*position = VuoMouse_convertWindowToVuoCoordinates(pointInWindow, targetWindow, &shouldTrackPresses);
			shouldTrackPresses = shouldTrackPresses && [targetWindow isKeyWindow];
		}
	}
	else
	{
		NSPoint pointInScreen = VuoMouseStatus_position;
		pointInScreen.y = CGDisplayPixelsHigh(CGMainDisplayID()) - pointInScreen.y;
		*position = VuoPoint2d_make(pointInScreen.x, pointInScreen.y);

		shouldTrackPresses = true;
	}

	if ( !shouldTrackPresses)
	{
		if (*isPressed)
		{
			// The existing button state is pressed.
			// If the button been released, we should update isPressed to reflect that release (even though we're ignoring presses).
		}
		else
			// The existing button state is not-pressed.  Since we're ignoring presses, we shouldn't change it.
			return true;
	}

	*isPressed = false;
	switch (button)
	{
		case VuoMouseButton_Left:
			*isPressed = VuoMouseStatus_leftButton;
			break;
		case VuoMouseButton_Right:
			*isPressed = VuoMouseStatus_rightButton;

			// Also return true for control-leftclicks.
			if (VuoMouseStatus_leftButton && VuoModifierKey_doMacEventFlagsMatch([NSEvent modifierFlags], VuoModifierKey_Control))
			{
				*isPressed = true;
				return true;
			}

			break;
		case VuoMouseButton_Middle:
			*isPressed = VuoMouseStatus_middleButton;
			break;
		case VuoMouseButton_Any:
			*isPressed = VuoMouseStatus_leftButton || VuoMouseStatus_middleButton || VuoMouseStatus_rightButton;
			break;
	}
	*isPressed = *isPressed && VuoModifierKey_doMacEventFlagsMatch([NSEvent modifierFlags], modifierKey);

	return true;
}
