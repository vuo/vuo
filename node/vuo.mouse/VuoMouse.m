/**
 * @file
 * VuoMouse implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>

#include "VuoMouse.h"
#include "VuoWindowOpenGLInternal.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoMouse",
					"dependencies" : [
						"VuoBoolean",
						"VuoModifierKey",
						"VuoMouseButton",
						"VuoPoint2d",
						"VuoWindowReference",
						"VuoWindowOpenGLInternal",
						"AppKit.framework"
					]
				 });
#endif


/**
 * Handle for starting and stopping event listeners.
 */
struct VuoMouseContext
{
	id monitor;  ///< The handle returned by NSEvent's method to start monitoring events, to be used to stop monitoring.

	dispatch_queue_t clickQueue;  ///< Synchronizes handling of click events.
	dispatch_group_t clickGroup;  ///< Synchronizes handling of click events.
	int pendingClickCount;  ///< The number of clicks so far in the in-progress series of clicks, or 0 if none in progress.
};

/**
 * Creates a handle for starting and stopping event listeners.
 */
VuoMouse * VuoMouse_make(void)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)calloc(1, sizeof(struct VuoMouseContext));
	VuoRegister(context, free);
	return (VuoMouse *)context;
}


/**
 * Converts a position relative to the window into a position relative to the current screen.
 */
static VuoPoint2d VuoMouse_convertWindowToScreenCoordinates(NSPoint pointInWindow, NSWindow *window)
{
	NSPoint pointInScreen = [window convertBaseToScreen:pointInWindow];
	pointInScreen.y = [[NSScreen mainScreen] frame].size.height - pointInScreen.y;
	return VuoPoint2d_make(pointInScreen.x, pointInScreen.y);
}

/**
 * Converts a position relative to a view and in view coordinates into a position in Vuo coordinates.
 */
static VuoPoint2d VuoMouse_convertViewToVuoCoordinates(NSPoint pointInView, NSView *view)
{
	NSRect bounds = [(VuoWindowOpenGLView *)view viewport];
	VuoPoint2d pointInVuo;
	pointInVuo.x = ((pointInView.x - NSMinX(bounds) - NSWidth(bounds) /2.) * 2.) / NSWidth(bounds);
	pointInVuo.y = ((pointInView.y - NSMinY(bounds) - NSHeight(bounds)/2.) * 2.) / NSWidth(bounds);
	return pointInVuo;
}

/**
 * Converts a position in screen coordinates into a position relative to the the window's fullscreen view and in Vuo coordinates.
 */
static VuoPoint2d VuoMouse_convertFullScreenToVuoCoordinates(NSPoint pointInScreen, NSWindow *window, bool *isInScreen)
{
	NSRect fullScreenFrame = [[window screen] frame];
	// Expand the frame by one point, since NSPointInRect() decides the topmost point (largest Y value) is outside the rectangle.
	// (Allow using the top edge, per Fitts's Law.)
	fullScreenFrame = NSInsetRect(fullScreenFrame, 0, -1);

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
static VuoPoint2d VuoMouse_convertWindowToVuoCoordinates(NSPoint pointInWindow, NSWindow *window)
{
	NSView *view = [window contentView];
	NSPoint pointInView = [view convertPoint:pointInWindow fromView:nil];
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
 * If the mouse was scrolled a non-zero amount, calls the trigger function and passes it the scroll delta.
 */
static void VuoMouse_fireScrollDeltaIfNeeded(NSEvent *event, VuoModifierKey modifierKey, void (*scrolled)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	VuoPoint2d delta = VuoPoint2d_make(-[event deltaX], [event deltaY]);
	if (delta.x != 0 || delta.y != 0)
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
static void VuoMouse_fireMousePositionIfNeeded(NSEvent *event, VuoWindowReference windowRef, VuoModifierKey modifierKey, void (^fire)(VuoPoint2d))
{
	if (! VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey))
		return;

	bool shouldFire = false;
	VuoPoint2d convertedPoint;
	NSPoint pointInWindowOrScreen = [event locationInWindow];
	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (targetWindow)
	{
		if ([(VuoWindowOpenGLView *)[targetWindow contentView] isFullScreen])
		{
			// https://b33p.net/kosada/node/8489
			// When in fullscreen mode with multiple displays, -[event locationInWindow] produces inconsistent results,
			// so use +[NSEvent mouseLocation] which seems to work with both single and multiple displays.
			pointInWindowOrScreen = [NSEvent mouseLocation];
			convertedPoint = VuoMouse_convertFullScreenToVuoCoordinates(pointInWindowOrScreen, targetWindow, &shouldFire);
		}
		else if (targetWindow == [event window])
		{
			shouldFire = true;
			convertedPoint = VuoMouse_convertWindowToVuoCoordinates(pointInWindowOrScreen, targetWindow);
		}
		else if (! [event window] && [targetWindow isKeyWindow])
		{
			// Workaround (https://b33p.net/kosada/node/7545):
			// [event window] becomes nil for some reason after mousing over certain other applications.
			shouldFire = true;
			NSPoint pointInWindow = [targetWindow convertScreenToBase:pointInWindowOrScreen];
			convertedPoint = VuoMouse_convertWindowToVuoCoordinates(pointInWindow, targetWindow);
		}
	}
	else
	{
		shouldFire = true;
		convertedPoint = VuoMouse_convertWindowToScreenCoordinates(pointInWindowOrScreen, [event window]);
	}

	if (shouldFire)
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
	NSPoint pointInWindowOrScreen = [event locationInWindow];
	NSPoint deltaInScreen = NSMakePoint([event deltaX], [event deltaY]);
	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (targetWindow)
	{
		if ([(VuoWindowOpenGLView *)[targetWindow contentView] isFullScreen])
		{
			// https://b33p.net/kosada/node/8489
			// When in fullscreen mode with multiple displays, -[event locationInWindow] produces inconsistent results,
			// so use +[NSEvent mouseLocation] which seems to work with both single and multiple displays.
			pointInWindowOrScreen = [NSEvent mouseLocation];
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
	NSTimeInterval clickIntervalInSeconds = [NSEvent doubleClickInterval];
	dispatch_time_t clickInterval = dispatch_time(DISPATCH_TIME_NOW, clickIntervalInSeconds * NSEC_PER_SEC);
	context->pendingClickCount = clickCount;

	void (*fired)(VuoPoint2d) = NULL;
	if (clickCount == 1)
		fired = singleClicked;
	else if (clickCount == 2)
		fired = doubleClicked;
	else if (clickCount == 3)
		fired = tripleClicked;
	if (! fired)
		return;

	dispatch_group_enter(context->clickGroup);
	dispatch_after(clickInterval, context->clickQueue, ^{
					   if (context->pendingClickCount == clickCount)
					   {
						   VuoMouse_fireMousePositionIfNeeded(event, windowRef, modifierKey, ^(VuoPoint2d point){ fired(point); });
						   context->pendingClickCount = 0;
					   }
					   dispatch_group_leave(context->clickGroup);
				   });
}


/**
 * Starts listening for scroll events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForScrolls(VuoMouse *mouseListener, void (*scrolled)(VuoPoint2d), VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSScrollWheelMask handler:^(NSEvent *event) {
		VuoMouse_fireScrollDeltaIfNeeded(event, modifierKey, scrolled);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * Starts listening for mouse move events, and calling the trigger function (with mouse position) for each one,.
 */
void VuoMouse_startListeningForMoves(VuoMouse *mouseListener, void (*movedTo)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSMouseMovedMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, window, modifierKey, ^(VuoPoint2d point){ movedTo(point); });
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * Starts listening for mouse move events, and calling the trigger function (with change in mouse position) for each one.
 */
void VuoMouse_startListeningForDeltas(VuoMouse *mouseListener, void (*movedBy)(VuoPoint2d), VuoWindowReference window, VuoModifierKey modifierKey)
{
	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSMouseMovedMask handler:^(NSEvent *event) {
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
												 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
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
			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseDraggedMask | NSOtherMouseDraggedMask | NSRightMouseDraggedMask;
			break;
	}

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, window, modifierKey, dragMovedTo);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;

}

/**
 * Starts listening for mouse drag events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForDrags(VuoMouse *mouseListener, void (*dragMovedTo)(VuoPoint2d), VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	VuoMouse_startListeningForDragsWithCallback(mouseListener, ^(VuoPoint2d point){ dragMovedTo(point); }, button, window, modifierKey);
}

/**
 * Starts listening for mouse press events, and calling the given block for each one.
 */
void VuoMouse_startListeningForPressesWithCallback(VuoMouse *mouseListener, void (^pressed)(VuoPoint2d),
												   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
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
			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseDownMask | NSOtherMouseDownMask | NSRightMouseDownMask;
			break;
	}

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, window, modifierKey, pressed);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
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
													VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
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
			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseUpMask | NSOtherMouseUpMask | NSRightMouseUpMask;
			break;
	}

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
		VuoMouse_fireMousePositionIfNeeded(event, window, modifierKey, released);
		return event;
	}];

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->monitor = monitor;
}

/**
 * Starts listening for mouse release events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForReleases(VuoMouse *mouseListener, void (*released)(VuoPoint2d), VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
	VuoMouse_startListeningForReleasesWithCallback(mouseListener, ^(VuoPoint2d point){ released(point); }, button, window, modifierKey);
}

/**
 * Starts listening for mouse click events, and calling the trigger function for each one.
 */
void VuoMouse_startListeningForClicks(VuoMouse *mouseListener, void (*singleClicked)(VuoPoint2d), void (*doubleClicked)(VuoPoint2d), void (*tripleClicked)(VuoPoint2d),
											VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey)
{
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
			break;
		case VuoMouseButton_Any:
			eventMask = NSLeftMouseUpMask | NSOtherMouseUpMask | NSRightMouseUpMask;
			break;
	}

	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;
	context->clickQueue = dispatch_queue_create("vuo.mouse.click", 0);
	context->clickGroup = dispatch_group_create();
	context->pendingClickCount = 0;

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^(NSEvent *event) {
		VuoMouse_fireMouseClickIfNeeded(context, event, window, modifierKey, singleClicked, doubleClicked, tripleClicked);
		return event;
	}];

	context->monitor = monitor;
}


/**
 * Stops listening for mouse events for the given handle.
 */
void VuoMouse_stopListening(VuoMouse *mouseListener)
{
	struct VuoMouseContext *context = (struct VuoMouseContext *)mouseListener;

	dispatch_sync(dispatch_get_main_queue(), ^{  // wait for any in-progress monitor handlers to complete
						if (context->monitor)
						{
							[NSEvent removeMonitor:context->monitor];
							context->monitor = nil;
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


/**
 * Returns the current mouse position.
 */
VuoPoint2d VuoMouse_getPosition(VuoWindowReference windowRef)
{
	VuoPoint2d convertedPoint;
	if (windowRef)
	{
		NSWindow *targetWindow = (NSWindow *)windowRef;
		NSPoint pointInWindow = [targetWindow mouseLocationOutsideOfEventStream];
		convertedPoint = VuoMouse_convertWindowToVuoCoordinates(pointInWindow, targetWindow);
	}
	else
	{
		NSPoint pointInScreen = [NSEvent mouseLocation];
		pointInScreen.y = [[NSScreen mainScreen] frame].size.height - pointInScreen.y;
		convertedPoint = VuoPoint2d_make(pointInScreen.x, pointInScreen.y);
	}
	return convertedPoint;
}

/**
 * Returns true if the given mouse button is currently pressed.
 */
VuoBoolean VuoMouse_isPressed(VuoMouseButton button, VuoModifierKey modifierKey)
{
	NSUInteger buttonMask = 0;
	switch (button)
	{
		case VuoMouseButton_Left:
			buttonMask = 1 << 0;
			break;
		case VuoMouseButton_Right:
			buttonMask = 1 << 1;
			break;
		case VuoMouseButton_Middle:
			buttonMask = 1 << 2;
			break;
		case VuoMouseButton_Any:
			buttonMask = (1 << 0) | (1 << 1) | (1 << 2);
			break;
	}
	return ([NSEvent pressedMouseButtons] & buttonMask) && VuoModifierKey_doMacEventFlagsMatch([NSEvent modifierFlags], modifierKey);
}
