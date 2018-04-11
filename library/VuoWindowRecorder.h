/**
 * @file
 * VuoWindowRecorder interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>

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

