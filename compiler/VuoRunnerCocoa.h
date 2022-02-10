/**
 * @file
 * VuoCocoa header.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#if defined(__OBJC__) || defined(DOXYGEN)

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

/**
 * Compiles, runs, and controls a Vuo composition.
 *
 * This is an abstract base class which provides common methods for @ref VuoImageFilter and @ref VuoImageGenerator.
 *
 * @see VuoRunner (a similar API, in C++)
 */
@interface VuoRunnerCocoa : NSObject

/**
 * The composition's user-defined title.
 *
 * This is the @c \@brief line of the composition's Doxygen header.
 */
@property (readonly) NSString *compositionName;

/**
 * The composition's user-defined description.
 *
 * The composition description consists of the lines after the first line of the composition's Doxygen header.
 */
@property (readonly) NSString *compositionDescription;

/**
 * The composition's user-defined copyright text.
 *
 * This is the @c \@copyright line of the composition's Doxygen header.
 */
@property (readonly) NSString *compositionCopyright;

+ (void)setGlobalRootContext:(CGLContextObj)context;
+ (void)prepareForFastBuild;

- (NSArray *)inputPorts;
- (NSArray *)outputPorts;
- (NSDictionary *)detailsForPort:(NSString *)portName;
- (id)propertyListFromInputValues;
- (BOOL)setInputValuesWithPropertyList:(id)propertyList;
- (BOOL)setInputValues:(NSDictionary *)namesAndValues;
- (BOOL)setInputJSON:(NSDictionary *)namesAndJSON;

- (id)valueForOutputPort:(NSString *)portName;
- (GLuint)glTextureWithTarget:(GLuint)target forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
- (GLuint)glTextureFromProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Filter protocol.
 */
@interface VuoImageFilter : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL;
+ (BOOL)canOpenCompositionString:(NSString *)compositionString;
- (id)initWithComposition:(NSURL *)compositionURL;
- (id)initWithCompositionString:(NSString *)compositionString name:(NSString *)name sourcePath:(NSString *)sourcePath;
- (NSImage *)filterNSImage:(NSImage *)image atTime:(NSTimeInterval)time;
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time withTextureProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Generator protocol.
 */
@interface VuoImageGenerator : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL;
+ (BOOL)canOpenCompositionString:(NSString *)compositionString;
- (id)initWithComposition:(NSURL *)compositionURL;
- (id)initWithCompositionString:(NSString *)compositionString name:(NSString *)name sourcePath:(NSString *)sourcePath;
- (NSImage *)generateNSImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time;
- (GLuint)generateGLTextureWithTarget:(GLuint)target suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
- (GLuint)generateGLTextureWithProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider suggestedPixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface;
- (VuoImage)generateVuoImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time;
@end

#undef VuoRunner
#undef VuoCompiler
#undef VuoComposition
#undef VuoProtocol

#endif // defined(__OBJC__) || defined(DOXYGEN)
