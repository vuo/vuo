/**
 * @file
 * VuoGraphicsWindowDrag interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "module.h"
#import "VuoGraphicsWindow.h"

#include "VuoMacOSSDKWorkaround.h"
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
