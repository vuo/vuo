/**
 * @file
 * vuo.mouse node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <ApplicationServices/ApplicationServices.h>
#include <objc/runtime.h>

VuoModuleMetadata({
					 "title" : "Get Mouse",
					 "keywords" : [ "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "ApplicationServices.framework",
						 "AppKit.framework",
						 "objc"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "ShowMouseClicks.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	CFMachPortRef tap;
	CFRunLoopSourceRef source;

	dispatch_queue_t clickQueue;
	int pendingClickCount;

	void (*movedTo)(VuoPoint2d);
	void (*movedBy)(VuoPoint2d);
	void (*scrolled)(VuoPoint2d);
	void (*usedButton)(VuoMouseButtonAction);

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
};

CGEventRef vuoDeviceMouseCGEventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
	struct nodeInstanceData * context = (struct nodeInstanceData *)refcon;

	CGPoint positionCG = CGEventGetLocation(event);
	VuoPoint2d position;
	position.x = positionCG.x;
	position.y = positionCG.y;

	VuoPoint2d distance;
	VuoMouseButtonAction buttonEvent;
	VuoMouseButton button;

	switch(CGEventGetType(event))
	{
		case kCGEventMouseMoved:
		case kCGEventLeftMouseDragged:
		case kCGEventOtherMouseDragged:
		case kCGEventRightMouseDragged:
			context->movedTo(position);
			distance.x = CGEventGetDoubleValueField(event,kCGMouseEventDeltaX);
			distance.y = CGEventGetDoubleValueField(event,kCGMouseEventDeltaY);
			context->movedBy(distance);
			break;
		case kCGEventScrollWheel:
			distance.x = -CGEventGetIntegerValueField(event,kCGScrollWheelEventDeltaAxis2);
			distance.y = CGEventGetIntegerValueField(event,kCGScrollWheelEventDeltaAxis1);
			if (distance.x || distance.y)
				context->scrolled(distance);
			break;
		case kCGEventLeftMouseDown:
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Left, VuoMouseButtonActionType_Press, position);
			context->usedButton(buttonEvent);
			break;
		case kCGEventLeftMouseUp:
			button = VuoMouseButton_Left;
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Left, VuoMouseButtonActionType_Release, position);
			context->usedButton(buttonEvent);
			break;
		case kCGEventOtherMouseDown:
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Middle, VuoMouseButtonActionType_Press, position);
			context->usedButton(buttonEvent);
			break;
		case kCGEventOtherMouseUp:
			button = VuoMouseButton_Middle;
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Middle, VuoMouseButtonActionType_Release, position);
			context->usedButton(buttonEvent);
			break;
		case kCGEventRightMouseDown:
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Right, VuoMouseButtonActionType_Press, position);
			context->usedButton(buttonEvent);
			break;
		case kCGEventRightMouseUp:
			button = VuoMouseButton_Right;
			buttonEvent = VuoMouseButtonAction_make(VuoMouseButton_Right, VuoMouseButtonActionType_Release, position);
			context->usedButton(buttonEvent);
			break;
	}

	int clickCount = CGEventGetIntegerValueField(event,kCGMouseEventClickState);

	switch(CGEventGetType(event))
	{
		case kCGEventLeftMouseUp:
		case kCGEventOtherMouseUp:
		case kCGEventRightMouseUp:
		{
			// [NSEvent doubleClickInterval];
			Class nsEventClass = (Class)objc_getClass("NSEvent");
			SEL doubleClickIntervalSEL = sel_registerName("doubleClickInterval");
			Method nsEventDoubleClickIntervalMethod = class_getClassMethod(nsEventClass, doubleClickIntervalSEL);
			typedef double (*nsEventDoubleClickIntervalType)(id, SEL);
			nsEventDoubleClickIntervalType nsEventDoubleClickInterval = (nsEventDoubleClickIntervalType)method_getImplementation(nsEventDoubleClickIntervalMethod);
			double clickIntervalSeconds = nsEventDoubleClickInterval((id)nsEventClass, method_getName(nsEventDoubleClickIntervalMethod));

			dispatch_time_t clickInterval = dispatch_time(DISPATCH_TIME_NOW, clickIntervalSeconds * NSEC_PER_SEC);

			context->pendingClickCount = clickCount;

			if (clickCount == 1)
			{
				buttonEvent = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_SingleClick, position);
				dispatch_after(clickInterval, context->clickQueue, ^{
								   if (context->pendingClickCount == clickCount)
								   {
									   context->usedButton(buttonEvent);
									   context->pendingClickCount = 0;
								   }
							   });
			}
			else if (clickCount == 2)
			{
				buttonEvent = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_DoubleClick, position);
				dispatch_after(clickInterval, context->clickQueue, ^{
								   if (context->pendingClickCount == clickCount)
								   {
									   context->usedButton(buttonEvent);
									   context->pendingClickCount = 0;
								   }
							   });
			}
			else if (clickCount == 3)
			{
				buttonEvent = VuoMouseButtonAction_make(button, VuoMouseButtonActionType_TripleClick, position);
				dispatch_sync(context->clickQueue, ^{
								  if (context->pendingClickCount == clickCount)
								  {
									  context->usedButton(buttonEvent);
									  context->pendingClickCount = 0;
								  }
							  });
			}
			break;
		}
	}

	return NULL; // Return value is ignored since we're kCGEventTapOptionListenOnly.
}

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->pendingClickCount = 0;
	context->clickQueue = dispatch_queue_create("vuo.mouse.click", 0);

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(movedTo, VuoPoint2d),
		VuoOutputTrigger(movedBy, VuoPoint2d),
		VuoOutputTrigger(scrolled, VuoPoint2d),
		VuoOutputTrigger(usedButton, VuoMouseButtonAction)
)
{
	(*context)->movedTo = movedTo;
	(*context)->movedBy = movedBy;
	(*context)->scrolled = scrolled;
	(*context)->usedButton = usedButton;

	(*context)->tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, kCGEventMaskForAllEvents, vuoDeviceMouseCGEventTapCallback, *context);
	CGEventTapEnable((*context)->tap, true);
	(*context)->source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, (*context)->tap, 0);
	CFRunLoopAddSource(CFRunLoopGetMain(), (*context)->source, kCFRunLoopCommonModes);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputData(VuoPoint2d) position,
		VuoOutputData(VuoBoolean) isLeftPressed,
		VuoOutputData(VuoBoolean) isMiddlePressed,
		VuoOutputData(VuoBoolean) isRightPressed
)
{
	CGEventRef event = CGEventCreate(NULL);
	CGPoint p = CGEventGetLocation(event);
	CFRelease(event);
	position->x = p.x;
	position->y = p.y;

	*isLeftPressed = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
	*isMiddlePressed = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonCenter);
	*isRightPressed = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	CFRunLoopRemoveSource(CFRunLoopGetMain(), (*context)->source, kCFRunLoopCommonModes);
	CFRelease((*context)->source);
	CGEventTapEnable((*context)->tap, false);
	CFRelease((*context)->tap);

	dispatch_sync((*context)->clickQueue, ^{});  // wait for any pending click events to complete
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	dispatch_release((*context)->clickQueue);
}
