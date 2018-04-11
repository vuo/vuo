/**
 * @file
 * VuoGraphicsWindowDrag interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "module.h"
#import "VuoGraphicsWindow.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>

/**
 * Methods related to dragging files from Finder.
 */
@interface VuoGraphicsWindow (Drag)
- (void)initDrag;
- (void)addDragEnteredCallback:(VuoGraphicsWindowDragCallback)dragEnteredCallback
		   dragMovedToCallback:(VuoGraphicsWindowDragCallback)dragMovedCallback
		 dragCompletedCallback:(VuoGraphicsWindowDragCallback)dragCompletedCallback
			dragExitedCallback:(VuoGraphicsWindowDragCallback)dragExitedCallback;
- (void)removeDragEnteredCallback:(VuoGraphicsWindowDragCallback)dragEnteredCallback
			  dragMovedToCallback:(VuoGraphicsWindowDragCallback)dragMovedCallback
			dragCompletedCallback:(VuoGraphicsWindowDragCallback)dragCompletedCallback
			   dragExitedCallback:(VuoGraphicsWindowDragCallback)dragExitedCallback;
@end
