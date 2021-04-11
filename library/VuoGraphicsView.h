/**
 * @file
 * VuoGraphicsView interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

#include "VuoPoint2d.h"
#include "VuoList_VuoPoint2d.h"

/**
 * Renders content using @ref VuoGraphicsLayer.
 */
@interface VuoGraphicsView : NSView
@property NSRect viewport;                    ///< The position and size of the viewport (subset of the view that we're actually drawing on), in pixels.

- (void)addTouchesMovedTrigger:(void (*)(VuoList_VuoPoint2d))touchesMoved
	zoomed:(void (*)(VuoReal))zoomed
	swipedLeft:(void (*)(void))swipedLeft
	swipedRight:(void (*)(void))swipedRight;
- (void)removeTouchesMovedTrigger:(void (*)(VuoList_VuoPoint2d))touchesMoved
	zoomed:(void (*)(VuoReal))zoomed
	swipedLeft:(void (*)(void))swipedLeft
	swipedRight:(void (*)(void))swipedRight;
@end
