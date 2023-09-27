/**
 * @file
 * VuoCocoa header.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#if defined(__OBJC__)

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

/// @{ Stub.
#define VuoRunner void
#define VuoCompiler void
#define VuoComposition void
#define VuoProtocol void
struct _VuoImage;
typedef struct _VuoImage * VuoImage;
typedef struct json_object json_object;
/// @}

#ifdef VUO_PCH_DEFAULT
/// Empty deprecation annotation.
#define VUO_RUNNER_COCOA_DEPRECATED
#else
/// Deprecation annotation.
#define VUO_RUNNER_COCOA_DEPRECATED __attribute__((deprecated("Use @ref VuoRunner (a similar API, in C++) instead.  The VuoRunnerCocoa Objective-C API is deprecated as of Vuo 2.4.3 and tentatively planned for removal in Vuo 3.0.0, since, as far as we know, it isn't currently being used by any apps.  If you do still need the Objective-C API, please contact Team Vuo to tell us about your use-case.")))
#endif

/**
 * (deprecated) Compiles, runs, and controls a Vuo composition.
 *
 * This is an abstract base class which provides common methods for @ref VuoImageFilter and @ref VuoImageGenerator.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface VuoRunnerCocoa : NSObject

/**
 * The composition's user-defined title.
 *
 * This is the @c \@brief line of the composition's Doxygen header.
 *
 * @vuoRunnerCocoaDeprecated
 */
@property (readonly) NSString *compositionName VUO_RUNNER_COCOA_DEPRECATED;

/**
 * The composition's user-defined description.
 *
 * The composition description consists of the lines after the first line of the composition's Doxygen header.
 *
 * @vuoRunnerCocoaDeprecated
 */
@property (readonly) NSString *compositionDescription VUO_RUNNER_COCOA_DEPRECATED;

/**
 * The composition's user-defined copyright text.
 *
 * This is the @c \@copyright line of the composition's Doxygen header.
 *
 * @vuoRunnerCocoaDeprecated
 */
@property (readonly) NSString *compositionCopyright VUO_RUNNER_COCOA_DEPRECATED;

+ (void)setGlobalRootContext:(CGLContextObj)context VUO_RUNNER_COCOA_DEPRECATED;
+ (void)prepareModuleCaches VUO_RUNNER_COCOA_DEPRECATED;

- (NSArray *)inputPorts VUO_RUNNER_COCOA_DEPRECATED;
- (NSArray *)outputPorts VUO_RUNNER_COCOA_DEPRECATED;
- (NSDictionary *)detailsForPort:(NSString *)portName VUO_RUNNER_COCOA_DEPRECATED;
- (id)propertyListFromInputValues VUO_RUNNER_COCOA_DEPRECATED;
- (BOOL)setInputValuesWithPropertyList:(id)propertyList VUO_RUNNER_COCOA_DEPRECATED;
- (BOOL)setInputValues:(NSDictionary *)namesAndValues VUO_RUNNER_COCOA_DEPRECATED;
- (BOOL)setInputJSON:(NSDictionary *)namesAndJSON VUO_RUNNER_COCOA_DEPRECATED;

- (id)valueForOutputPort:(NSString *)portName VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)glTextureWithTarget:(GLuint)target forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)glTextureFromProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Filter protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface VuoImageFilter : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL VUO_RUNNER_COCOA_DEPRECATED;
+ (BOOL)canOpenCompositionString:(NSString *)compositionString VUO_RUNNER_COCOA_DEPRECATED;
- (id)initWithComposition:(NSURL *)compositionURL VUO_RUNNER_COCOA_DEPRECATED;
- (id)initWithCompositionString:(NSString *)compositionString name:(NSString *)name sourcePath:(NSString *)sourcePath VUO_RUNNER_COCOA_DEPRECATED;
- (NSImage *)filterNSImage:(NSImage *)image atTime:(NSTimeInterval)time VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time withTextureProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Generator protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface VuoImageGenerator : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL VUO_RUNNER_COCOA_DEPRECATED;
+ (BOOL)canOpenCompositionString:(NSString *)compositionString VUO_RUNNER_COCOA_DEPRECATED;
- (id)initWithComposition:(NSURL *)compositionURL VUO_RUNNER_COCOA_DEPRECATED;
- (id)initWithCompositionString:(NSString *)compositionString name:(NSString *)name sourcePath:(NSString *)sourcePath VUO_RUNNER_COCOA_DEPRECATED;
- (NSImage *)generateNSImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)generateGLTextureWithTarget:(GLuint)target suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh VUO_RUNNER_COCOA_DEPRECATED;
- (GLuint)generateGLTextureWithProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider suggestedPixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface VUO_RUNNER_COCOA_DEPRECATED;
- (VuoImage)generateVuoImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time VUO_RUNNER_COCOA_DEPRECATED;
@end

#undef VuoRunner
#undef VuoCompiler
#undef VuoComposition
#undef VuoProtocol

#endif // defined(__OBJC__) || defined(DOXYGEN)
