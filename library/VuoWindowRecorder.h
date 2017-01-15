/**
 * @file
 * VuoWindowRecorder interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOWINDOWRECORDER_HH
#define VUOWINDOWRECORDER_HH

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>

@class VuoWindowOpenGLInternal;

/**
 * Manages recording the contents of a `VuoWindowOpenGL`.
 */
@interface VuoWindowRecorder : NSObject
- (instancetype)initWithWindow:(VuoWindowOpenGLInternal *)window url:(NSURL *)url;
- (void)captureImageOfContext:(CGLContextObj)cgl_ctx;
- (void)finish;
@end

#endif // VUOWINDOWRECORDER_HH
