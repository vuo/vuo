/**
 * @file
 * VuoWindowRecorder interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoMacOSSDKWorkaround.h"
#import <Foundation/Foundation.h>

#import "VuoGlPool.h"

@class VuoGraphicsWindow;

/**
 * Manages recording the contents of a @ref VuoGraphicsWindow.
 */
@interface VuoWindowRecorder : NSObject
- (instancetype)initWithWindow:(VuoGraphicsWindow *)window url:(NSURL *)url;
- (void)saveImage:(VuoIoSurface)vis;
- (void)finish;
@end
