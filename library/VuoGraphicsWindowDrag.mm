/**
 * @file
 * VuoWindowDrag implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoGraphicsWindowDrag.h"
#import "VuoMouse.h"
#import "VuoTriggerSet.hh"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsWindowDrag",
	"dependencies" : [
		"VuoDragEvent",
		"VuoList_VuoUrl",
		"VuoMouse"
	]
});
#endif


@implementation VuoGraphicsWindow (Drag)
/**
 * Helper for VuoGraphicsWindow's init method.
 */
- (void)initDrag
{
	self.dragEntered   = new VuoTriggerSet<VuoDragEvent>;
	self.dragMovedTo   = new VuoTriggerSet<VuoDragEvent>;
	self.dragCompleted = new VuoTriggerSet<VuoDragEvent>;
	self.dragExited    = new VuoTriggerSet<VuoDragEvent>;
}

/**
 * Adds callbacks to be invoked when files are dragged from Finder.
 */
- (void)addDragEnteredCallback:(VuoGraphicsWindowDragCallback)dragEnteredCallback
		   dragMovedToCallback:(VuoGraphicsWindowDragCallback)dragMovedToCallback
		 dragCompletedCallback:(VuoGraphicsWindowDragCallback)dragCompletedCallback
			dragExitedCallback:(VuoGraphicsWindowDragCallback)dragExitedCallback
{
	((VuoTriggerSet<VuoDragEvent>*)self.dragEntered)->addTrigger(dragEnteredCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragMovedTo)->addTrigger(dragMovedToCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragCompleted)->addTrigger(dragCompletedCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragExited)->addTrigger(dragExitedCallback);
}

/**
 * Removes callbacks that would have been invoked when files were dragged from Finder.
 */
- (void)removeDragEnteredCallback:(VuoGraphicsWindowDragCallback)dragEnteredCallback
			  dragMovedToCallback:(VuoGraphicsWindowDragCallback)dragMovedToCallback
			dragCompletedCallback:(VuoGraphicsWindowDragCallback)dragCompletedCallback
			   dragExitedCallback:(VuoGraphicsWindowDragCallback)dragExitedCallback
{
	((VuoTriggerSet<VuoDragEvent>*)self.dragEntered)->removeTrigger(dragEnteredCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragMovedTo)->removeTrigger(dragMovedToCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragCompleted)->removeTrigger(dragCompletedCallback);
	((VuoTriggerSet<VuoDragEvent>*)self.dragExited)->removeTrigger(dragExitedCallback);
}

/**
 * Returns true if dragEvent was successfully populated.
 */
- (bool)makeDragEvent:(VuoDragEvent *)dragEvent fromSender:(id<NSDraggingInfo>)sender
{
	if (!((VuoTriggerSet<VuoDragEvent>*)self.dragEntered)->size())
		return NO;

	NSPasteboard *pboard = [sender draggingPasteboard];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// The replacement, NSPasteboardTypeFileURL, isn't available until macOS 11.
	if (![[pboard types] containsObject:NSFilenamesPboardType])
		return NO;
#pragma clang diagnostic pop

	NSPoint p = [sender draggingLocation];

	VuoList_VuoUrl urls = VuoListCreate_VuoUrl();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// The replacement, NSPasteboardTypeFileURL, isn't available until macOS 11.
	NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
#pragma clang diagnostic pop
	for (NSString *file in files)
	{
		VuoText path = VuoText_makeFromCFString(file);
		VuoRetain(path);
		VuoUrl url = VuoUrl_normalize(path, VuoUrlNormalize_default);
		VuoListAppendValue_VuoUrl(urls, url);
		VuoRelease(path);
	}

	bool shouldFire;
	*dragEvent = VuoDragEvent_make(VuoMouse_convertWindowToVuoCoordinates(p, self, &shouldFire), urls);

	return YES;
}

/**
 * Invoked when a Finder drag first moves into the window.
 */
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
	VuoDragEvent dragEvent;
	if (![self makeDragEvent:&dragEvent fromSender:sender])
		return NSDragOperationNone;

	((VuoTriggerSet<VuoDragEvent>*)self.dragEntered)->fire(dragEvent);
	return NSDragOperationGeneric;
}

/**
 * Invoked when a Finder drag moves around within the window.
 */
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
	VuoDragEvent dragEvent;
	if (![self makeDragEvent:&dragEvent fromSender:sender])
		return NSDragOperationNone;

	((VuoTriggerSet<VuoDragEvent>*)self.dragMovedTo)->fire(dragEvent);
	return NSDragOperationGeneric;
}

/**
 * Invoked when a Finder drag is released over the window.
 */
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
	VuoDragEvent dragEvent;
	if (![self makeDragEvent:&dragEvent fromSender:sender])
		return NO;

	((VuoTriggerSet<VuoDragEvent>*)self.dragCompleted)->fire(dragEvent);
	return YES;
}

/**
 * Invoked when a Finder drag leaves the window without being released.
 */
- (void)draggingExited:(id<NSDraggingInfo>)sender
{
	VuoDragEvent dragEvent;
	if (![self makeDragEvent:&dragEvent fromSender:sender])
		return;

	((VuoTriggerSet<VuoDragEvent>*)self.dragExited)->fire(dragEvent);
}

@end
