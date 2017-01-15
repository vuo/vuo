/**
 * @file
 * VuoWindowDrag implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowOpenGLInternal.h"

#include "VuoTriggerSet.hh"

#include "module.h"
#include "VuoMouse.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowDrag",
					 "dependencies" : [
						 "VuoDragEvent",
						 "VuoList_VuoUrl",
						 "VuoMouse"
					 ]
				 });
#endif


@implementation VuoWindowOpenGLInternal(Drag)
/**
 * Helper for VuoWindowOpenGLInternal's init method.
 */
- (void)initDrag
{
	dragEntered   = new VuoTriggerSet<VuoDragEvent>;
	dragMovedTo   = new VuoTriggerSet<VuoDragEvent>;
	dragCompleted = new VuoTriggerSet<VuoDragEvent>;
	dragExited    = new VuoTriggerSet<VuoDragEvent>;
}

/**
 * Adds callbacks to be invoked when files are dragged from Finder.
 */
- (void)addDragEnteredCallback:(void (*)(VuoDragEvent e))dragEnteredCallback
	dragMovedToCallback:(void (*)(VuoDragEvent e))dragMovedToCallback
	dragCompletedCallback:(void (*)(VuoDragEvent e))dragCompletedCallback
	dragExitedCallback:(void (*)(VuoDragEvent e))dragExitedCallback
{
	((VuoTriggerSet<VuoDragEvent>*)dragEntered)->addTrigger(dragEnteredCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragMovedTo)->addTrigger(dragMovedToCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragCompleted)->addTrigger(dragCompletedCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragExited)->addTrigger(dragExitedCallback);
}

/**
 * Removes callbacks that would have been invoked when files were dragged from Finder.
 */
- (void)removeDragEnteredCallback:(void (*)(VuoDragEvent e))dragEnteredCallback
	dragMovedToCallback:(void (*)(VuoDragEvent e))dragMovedToCallback
	dragCompletedCallback:(void (*)(VuoDragEvent e))dragCompletedCallback
	dragExitedCallback:(void (*)(VuoDragEvent e))dragExitedCallback
{
	((VuoTriggerSet<VuoDragEvent>*)dragEntered)->removeTrigger(dragEnteredCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragMovedTo)->removeTrigger(dragMovedToCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragCompleted)->removeTrigger(dragCompletedCallback);
	((VuoTriggerSet<VuoDragEvent>*)dragExited)->removeTrigger(dragExitedCallback);
}

/**
 * Returns true if dragEvent was successfully populated.
 */
- (bool)makeDragEvent:(VuoDragEvent *)dragEvent fromSender:(id<NSDraggingInfo>)sender
{
	if (!((VuoTriggerSet<VuoDragEvent>*)dragEntered)->size())
		return NO;

	NSPasteboard *pboard = [sender draggingPasteboard];

	if (![[pboard types] containsObject:NSFilenamesPboardType])
		return NO;

	NSPoint p = [sender draggingLocation];

	VuoList_VuoUrl urls = VuoListCreate_VuoUrl();
	NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
	for (NSString *file in files)
	{
		VuoText path = VuoText_makeFromCFString(file);
		VuoRetain(path);
		VuoUrl url = VuoUrl_normalize(path, false);
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

	((VuoTriggerSet<VuoDragEvent>*)dragEntered)->fire(dragEvent);
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

	((VuoTriggerSet<VuoDragEvent>*)dragMovedTo)->fire(dragEvent);
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

	((VuoTriggerSet<VuoDragEvent>*)dragCompleted)->fire(dragEvent);
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

	((VuoTriggerSet<VuoDragEvent>*)dragExited)->fire(dragEvent);
}

@end
