/**
 * @file
 * VuoCocoa header.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#if defined(__OBJC__) || defined(DOXYGEN)

#ifndef VUOCOCOA_H
#define VUOCOCOA_H

#import <Cocoa/Cocoa.h>

/// @{
#define VuoRunner void
#define VuoCompiler void
#define VuoComposition void
#define VuoProtocol void
/// @}

/**
 * Compiles, runs, and controls a Vuo composition.
 *
 * This is an abstract base class which provides common methods for @ref VuoImageFilter and @ref VuoImageGenerator.
 *
 * @see VuoRunner (a similar API, in C++)
 */
@interface VuoRunnerCocoa : NSObject
{
/// @{
	// Explicit property ivar declarations are required for 32-bit Objective-C.
	dispatch_queue_t runnerQueue;
	VuoRunner *runner;
	VuoCompiler *compiler;
	VuoComposition *composition;
	NSURL *compositionURL;
	NSString *compositionString;
	NSString *compositionName;
	NSString *compositionDescription;
	NSString *compositionCopyright;
	VuoProtocol *protocol;
/// @}
}

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

- (NSArray *)inputPorts;
- (NSArray *)outputPorts;
- (NSDictionary *)detailsForPort:(NSString *)portName;
- (id)propertyListFromInputValues;
- (BOOL)setInputValuesWithPropertyList:(id)propertyList;
- (BOOL)setValue:(id)value forInputPort:(NSString *)portName;

- (id)valueForOutputPort:(NSString *)portName;
- (GLuint)glTextureWithTarget:(GLuint)target forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Filter protocol.
 */
@interface VuoImageFilter : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL;
- (id)initWithComposition:(NSURL *)compositionURL;
- (NSImage *)filterNSImage:(NSImage *)image atTime:(NSTimeInterval)time;
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
@end

/**
 * Compiles, runs, and controls a Vuo composition that adheres to the Image Generator protocol.
 */
@interface VuoImageGenerator : VuoRunnerCocoa
+ (BOOL)canOpenComposition:(NSURL *)compositionURL;
- (id)initWithComposition:(NSURL *)compositionURL;
- (NSImage *)generateNSImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time;
- (GLuint)generateGLTextureWithTarget:(GLuint)target suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh;
@end

#undef VuoRunner
#undef VuoCompiler
#undef VuoComposition
#undef VuoProtocol

#endif // VUOCOCOA_H

#endif // defined(__OBJC__) || defined(DOXYGEN)
