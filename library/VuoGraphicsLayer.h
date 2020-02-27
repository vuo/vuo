/**
 * @file
 * VuoGraphicsLayer interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
@interface VuoGraphicsLayer : CAOpenGLLayer
@property(assign) VuoGraphicsWindow *window;          ///< The window this layer is part of.
@property VuoIoSurface ioSurface;             ///< The IOSurface to pass the texture from Vuo's OpenGL context to the window's OpenGL context.
//@property NSRect viewport;                    ///< The position and size of the viewport (subset of the view that we're actually drawing on), in pixels.

- (instancetype)initWithWindow:(VuoGraphicsWindow *)window
						initCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
				  backingScaleFactor:(float)backingScaleFactor
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData;
- (void)viewDidChangeBackingProperties;
- (void)enableTriggers;
- (void)disableTriggers;
- (void)close;
- (void)draw;
@end
