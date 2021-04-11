/**
 * @file
 * VuoGraphicsView implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoGraphicsView.h"

#import "module.h"
#import "VuoGraphicsLayer.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsView",
	"dependencies" : [
		"VuoGraphicsLayer",
	]
});
#endif

/**
 * Private VuoGraphicsView data.
 */
@interface VuoGraphicsView ()
@property(retain) NSImage *circleImage; ///< The touch-circle mouse cursor.
@property NSRect circleRect;            ///< The bounding box of `circleImage`.

@property NSMutableSet *touchTriggers;  ///< Callbacks to invoke when touch events are received.
@property NSMutableSet *zoomedTriggers;  ///< Callbacks to invoke when touch events are received.
@property NSMutableSet *swipedLeftTriggers;  ///< Callbacks to invoke when touch events are received.
@property NSMutableSet *swipedRightTriggers;  ///< Callbacks to invoke when touch events are received.
@end

@implementation VuoGraphicsView

/**
 * Creates a view that renders content using @ref VuoGraphicsLayer.
 *
 * @threadMain
 */
- (instancetype)init
{
	if (self = [super init])
	{
		self.wantsLayer = true;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// Replaced by acceptsTouchEvents, which isn't available in OS X 10.11.
		self.acceptsTouchEvents = YES;
#pragma clang diagnostic pop

		self.wantsRestingTouches = YES;
		_touchTriggers = [NSMutableSet new];
		_zoomedTriggers = [NSMutableSet new];
		_swipedLeftTriggers = [NSMutableSet new];
		_swipedRightTriggers = [NSMutableSet new];

		// Prepare the circle mouse cursor.
		{
			_circleRect = NSMakeRect(0,0,48,48);
			_circleImage = [[NSImage alloc] initWithSize:_circleRect.size];
			[_circleImage lockFocus];
			{
				[[NSColor colorWithDeviceWhite:1 alpha:0.75] setFill];
				[[NSColor colorWithDeviceWhite:0 alpha:0.15] setStroke];
				NSBezierPath *circlePath = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(_circleRect, 1, 1)];
				[circlePath fill];
				[circlePath stroke];
			}
			[_circleImage unlockFocus];
		}
	}
	return self;
}

/**
 * Ensures mouse coordinates can be accurately reported even before drawing.
 * https://b33p.net/kosada/node/12691
 *
 * Reimplemented from NSView.
 */
- (void)viewDidMoveToWindow
{
	if (!self.window)
		return;

	_viewport = self.frame;
}

/**
 * Called when the view changes screens (and thus maybe the backingScaleFactor changed).
 *
 * Reimplemented from NSView.
 * @threadMain
 */
- (void)viewDidChangeBackingProperties
{
	VuoGraphicsLayer *l = (VuoGraphicsLayer *)self.layer;
	[l viewDidChangeBackingProperties];
	[l setNeedsDisplay];
}

/**
 * Reimplemented from NSView.
 * @threadMain
 */
- (void)resetCursorRects
{
	VuoGraphicsWindow *gw = (VuoGraphicsWindow *)self.window;
	VuoCursor cursor = gw.cursor;
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
		nsCursor = [[[NSCursor alloc] initWithImage:_circleImage hotSpot:NSMakePoint(NSMidX(_circleRect),NSMidY(_circleRect))] autorelease];

	if (nsCursor)
		[self addCursorRect:[self visibleRect] cursor:nsCursor];
}

/**
 * Without this, double-clicks cause the window to minimize/maximize itself (depending on System Preferences).
 *
 * Reimplemented from NSView.
 * @threadMain
 */
- (BOOL)acceptsFirstResponder
{
	return YES;
}

/**
 * Workaround for apparent macOS 10.14 bug where,
 * when 'System Preferences > Dock > Double-click on a window's title bar to [minimize/zoom]' is enabled
 * and 'System Preferences > Mission Control > Displays have separate Spaces' is disabled,
 * un-fullscreening a window then double-clicking on its content area causes it to minimize/zoom.
 *
 * https://vuo.org/node/2425
 *
 * Reimplemented from NSView.
 * @threadMain
 */
- (BOOL)mouseDownCanMoveWindow
{
	return NO;
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	[_circleImage release];
	[super dealloc];
}

static NSInteger VuoGraphicsView_touchComparator(NSTouch *a, NSTouch *b, void *p)
{
	float ax = a.normalizedPosition.x;
	float bx = b.normalizedPosition.x;
	if (ax < bx)
		return -1;
	else if (ax > bx)
		return 1;
	else
		return 0;
}

- (void)fireTouches:(NSEvent *)e
{
	NSSet *touchesSet = [e touchesMatchingPhase:NSTouchPhaseAny inView:nil];
	NSArray *orderedTouches = [touchesSet.allObjects sortedArrayUsingFunction:VuoGraphicsView_touchComparator context:nil];

	VuoList_VuoPoint2d touches = VuoListCreateWithCount_VuoPoint2d(orderedTouches.count, (VuoPoint2d){0,0});
	VuoPoint2d *touchPoints = VuoListGetData_VuoPoint2d(touches);
	int i = 0;
	for (NSTouch *t in orderedTouches)
		touchPoints[i++] = (VuoPoint2d){
			t.normalizedPosition.x * 2 - 1,
			(t.normalizedPosition.y * 2 - 1) * (t.deviceSize.height / t.deviceSize.width)
		};

	dispatch_async(dispatch_get_main_queue(), ^{
		for (NSValue *v in self.touchTriggers)
		{
			void (*touchesMoved)(VuoList_VuoPoint2d) = v.pointerValue;
			if (touchesMoved)
				touchesMoved(touches);
		}
	});
}
- (void)touchesBeganWithEvent:(NSEvent *)event
{
	[self fireTouches:event];
}
- (void)touchesMovedWithEvent:(NSEvent *)event
{
	[self fireTouches:event];
}
- (void)touchesEndedWithEvent:(NSEvent *)event
{
	[self fireTouches:event];
}
- (void)touchesCancelledWithEvent:(NSEvent *)event
{
	[self fireTouches:event];
}

- (void)magnifyWithEvent:(NSEvent *)event
{
	if (event.magnification != 0)
		dispatch_async(dispatch_get_main_queue(), ^{
			for (NSValue *v in self.zoomedTriggers)
			{
				void (*zoomed)(VuoReal) = v.pointerValue;
				if (zoomed)
					zoomed(event.magnification);
			}
		});
}

- (void)swipeWithEvent:(NSEvent *)event
{
	if (event.deltaX > 0)
		dispatch_async(dispatch_get_main_queue(), ^{
			for (NSValue *v in self.swipedLeftTriggers)
			{
				void (*swipedLeft)(void) = v.pointerValue;
				if (swipedLeft)
					swipedLeft();
			}
		});
	else if (event.deltaX < 0)
		dispatch_async(dispatch_get_main_queue(), ^{
			for (NSValue *v in self.swipedRightTriggers)
			{
				void (*swipedRight)(void) = v.pointerValue;
				if (swipedRight)
					swipedRight();
			}
		});
}

/**
 * Adds callbacks to be invoked when touch events are received.
 *
 * @threadAny
 * @version200New
 */
- (void)addTouchesMovedTrigger:(void (*)(VuoList_VuoPoint2d))touchesMoved
	zoomed:(void (*)(VuoReal))zoomed
	swipedLeft:(void (*)(void))swipedLeft
	swipedRight:(void (*)(void))swipedRight
{
	dispatch_async(dispatch_get_main_queue(), ^{
		[self.touchTriggers addObject:[NSValue valueWithPointer:touchesMoved]];
		[self.zoomedTriggers addObject:[NSValue valueWithPointer:zoomed]];
		[self.swipedLeftTriggers addObject:[NSValue valueWithPointer:swipedLeft]];
		[self.swipedRightTriggers addObject:[NSValue valueWithPointer:swipedRight]];
	});
}

/**
 * Removes callbacks that would have been invoked when touch events were received.
 *
 * @threadAny
 * @version200New
 */
- (void)removeTouchesMovedTrigger:(void (*)(VuoList_VuoPoint2d))touchesMoved
	zoomed:(void (*)(VuoReal))zoomed
	swipedLeft:(void (*)(void))swipedLeft
	swipedRight:(void (*)(void))swipedRight
{
	dispatch_async(dispatch_get_main_queue(), ^{
		for (NSValue *v in self.touchTriggers)
			if (v.pointerValue == touchesMoved)
				[self.touchTriggers removeObject:v];
		for (NSValue *v in self.zoomedTriggers)
			if (v.pointerValue == zoomed)
				[self.zoomedTriggers removeObject:v];
		for (NSValue *v in self.swipedLeftTriggers)
			if (v.pointerValue == swipedLeft)
				[self.swipedLeftTriggers removeObject:v];
		for (NSValue *v in self.swipedRightTriggers)
			if (v.pointerValue == swipedRight)
				[self.swipedRightTriggers removeObject:v];
	});
}
@end
