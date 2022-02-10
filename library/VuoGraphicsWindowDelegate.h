/**
 * @file
 * VuoGraphicsWindowDelegate interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

//#import "module.h"
#import "VuoGraphicsWindow.h"

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

/**
 * Handles window events on behalf of VuoGraphicsWindow.
 */
@interface VuoGraphicsWindowDelegate : NSObject <NSWindowDelegate>
- (instancetype)initWithWindow:(VuoGraphicsWindow *)window;
@end
