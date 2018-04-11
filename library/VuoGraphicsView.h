/**
 * @file
 * VuoGraphicsView interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "module.h"
#import "VuoDisplayRefresh.h"
#import "VuoGlContext.h"
#import "VuoWindow.h"
#import "VuoGraphicsWindow.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>

/**
 * Uses VuoDisplayRefresh to render callbacks onto an IOSurface,
 * then (on the main thread) draws that IOSurface onto a window.
 *
 * The view maintains its own local (non-shared) OpenGL context.
 */
@interface VuoGraphicsView : NSView
@property CGLContextObj rootContext;         ///< @todo https://b33p.net/kosada/node/11554 — remove when contexts are unified.
@property(atomic) bool needsIOSurfaceRedraw; ///< Callers can set this to true if they want the content to be redrawn the next time the VuoDisplayRefresh fires.
@property VuoIoSurface ioSurface;             ///< The IOSurface to pass the texture from Vuo's OpenGL context to the window's OpenGL context.
@property NSRect viewport;                    ///< The position and size of the viewport (subset of the view that we're actually drawing on), in pixels.

- (instancetype)initWithInitCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
				  backingScaleFactor:(float)backingScaleFactor
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData;

- (void)enableTriggers;
- (void)disableTriggers;
- (void)close;
@end
