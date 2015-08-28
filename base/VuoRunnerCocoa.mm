/**
 * @file
 * VuoCocoa implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

/// Avoid conflict between Cocoa and LLVM headers.
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0

#include "VuoRunnerCocoa.h"
#include "VuoRunnerCocoa+Conversion.hh"

#include <OpenGL/CGLMacro.h>
#include <QuartzCore/CoreImage.h>

#include <dispatch/dispatch.h>

#include <zmq/zmq.h>
#include <vector>
#include <set>
#include "VuoRunner.hh"
#include "VuoComposition.hh"
#include "VuoProtocol.hh"
#include "VuoHeap.h"
#include "VuoGlContext.h"

extern "C" {
#include "VuoInteger.h"
}

/**
 * Private extensions to @ref VuoRunnerCocoa.
 */
@interface VuoRunnerCocoa ()
@property dispatch_queue_t runnerQueue;	///< Initializes the runner; ensures that users of the runner execute after initialization.
@property VuoRunner *runner;
@property NSURL *compositionURL;
@property NSString *compositionString;
@property NSString *compositionName;
@property NSString *compositionDescription;
@property NSString *compositionCopyright;
@property VuoProtocol *protocol;
@end

@implementation VuoRunnerCocoa
@synthesize runnerQueue;
@synthesize runner;
@synthesize compositionURL;
@synthesize compositionString;
@synthesize compositionName;
@synthesize compositionDescription;
@synthesize compositionCopyright;
@synthesize protocol;

/**
 * Specifies an OpenGL context to be used as the base for all of Vuo's shared GL contexts.
 *
 * Must be called before any Vuo composition is loaded.
 *
 * @threadAny
 */
+ (void)setGlobalRootContext:(CGLContextObj)context
{
	VuoGlContext_setGlobalRootContext(context);
}

/**
 * Downloads and parses the specified `compositionURL`, to check whether it's compliant with `protocol`.
 */
+ (BOOL)isComposition:(NSURL *)compositionURL compliantWithProtocol:(VuoProtocol *)protocol
{
	VuoRunnerCocoa *runner = [[VuoRunnerCocoa alloc] initWithComposition:compositionURL protocol:protocol];
	if (!runner)
		return NO;

	[runner release];
	return YES;
}

/**
 * Opens and parses the specified @c compositionURL.
 */
- (id)initWithComposition:(NSURL *)theCompositionURL protocol:(VuoProtocol *)theProtocol
{
	if (!(self = [super init]))
		return nil;

	self.compositionURL = theCompositionURL;
	self.protocol = theProtocol;

	NSError *error;
	compositionString = [[NSString alloc] initWithContentsOfURL:self.compositionURL encoding:NSUTF8StringEncoding error:&error];
	if (!self.compositionString)
	{
		NSLog(@"[VuoRunnerCocoa initWithComposition:protocol:] Error reading composition: %@", error);
		[self release];
		return nil;
	}

	self.runnerQueue = dispatch_queue_create("org.vuo.runner.cocoa", NULL);
	self.runner = NULL;

	const char *compositionCString = [self.compositionString UTF8String];
	if (!theProtocol->isCompositionCompliant(compositionCString))
	{
		[self release];
		return nil;
	}

	string name, description, copyright;
	VuoComposition::parseHeader(compositionCString, name, description, copyright);
	compositionName		= [[NSString alloc] initWithUTF8String:name.c_str()];
	compositionDescription	= [[NSString alloc] initWithUTF8String:description.c_str()];
	compositionCopyright	= [[NSString alloc] initWithUTF8String:copyright.c_str()];

	return self;
}

/**
 * Frees instance data.
 */
- (void)dealloc
{
	dispatch_sync(self.runnerQueue, ^{
					  if (self.runner)
					  {
						  self.runner->stop();
						  delete self.runner;
					  }
				  });
	dispatch_release(self.runnerQueue);
	[compositionString release];
	[compositionName release];
	[compositionDescription release];
	[compositionCopyright release];
	[super dealloc];
}

- (void)compileAndRunAsynchronously
{
	// Make sure we don't retain self during long compilations,
	// since the host might release it for what it thinks is the last time, then quit,
	// which would leave a dangling composition process.
	__block typeof(self) weakSelf = self;

	dispatch_async(self.runnerQueue, ^{
					   string compositionFilePath = [[weakSelf.compositionURL path] UTF8String];
					   weakSelf.runner = VuoRunner::newSeparateProcessRunnerFromCompositionFile(compositionFilePath);
					   weakSelf.runner->start();
				   });
}

/**
 * Returns YES if the composition failed to build and run successfully, or if it was running and has stopped.
 */
- (BOOL)isStopped
{
	__block BOOL isStopped = YES;
	dispatch_sync(self.runnerQueue, ^{
					  if (self.runner && !self.runner->isStopped())
						  isStopped = NO;
				  });
	return isStopped;
}

/**
 * Provide a description of this object, for debugging.
 */
- (NSString *)description
{
	return [NSString stringWithFormat:@"VuoRunnerCocoa(\"%@\")", compositionName];
}

/**
 * Returns an array listing the names of the composition's published input ports,
 * excluding those used by protocols.
 */
- (NSArray *)inputPorts
{
	if ([self isStopped])
		return nil;

	NSMutableArray *portsArray = [[NSMutableArray new] autorelease];
	vector<VuoRunner::Port *> ports = self.runner->getPublishedInputPorts();
	for (vector<VuoRunner::Port *>::iterator it = ports.begin(); it != ports.end(); ++it)
		if (self.protocol && !self.protocol->hasInputPort((*it)->getName()))
			[portsArray addObject:[NSString stringWithUTF8String:(*it)->getName().c_str()]];
	return portsArray;
}

/**
 * Returns an array listing the names of the composition's published output ports,
 * excluding those used by protocols.
 */
- (NSArray *)outputPorts
{
	if ([self isStopped])
		return nil;

	NSMutableArray *portsArray = [[NSMutableArray new] autorelease];
	vector<VuoRunner::Port *> ports = self.runner->getPublishedOutputPorts();
	for (vector<VuoRunner::Port *>::iterator it = ports.begin(); it != ports.end(); ++it)
		if (self.protocol && !self.protocol->hasOutputPort((*it)->getName()))
			[portsArray addObject:[NSString stringWithUTF8String:(*it)->getName().c_str()]];
	return portsArray;
}

/**
 * Returns a dictionary with information about the specified @c portName.
 *
 * Keys include:
 *
 *    - @c title — value is an @c NSString with the port's display name
 *    - @c type — value is an @c NSString with the port's type (e.g., @ref VuoText)
 *    - @c default — value is a Cocoa object (see @ref valueForOutputPort:) with the port's default value
 *    - @c menuItems — value is an @c NSArray where each element is an @c NSDictionary with 2 keys: @c value (NSString) and @c name (NSString)
 *    - @c suggestedMin — value is a Cocoa object (see @ref valueForOutputPort:) with the port's suggested minimum value
 *    - @c suggestedMax — value is a Cocoa object (see @ref valueForOutputPort:) with the port's suggested maximum value
 *    - @c suggestedStep — value is a Cocoa object (see @ref valueForOutputPort:) with the port's suggested step (the amount the value changes with each click of a spinbox)
 *
 * If @c menuItems contains any values, the host application should display a select widget.
 * Otherwise, the host application should use @c type to determine the kind of widget to display.
 * Host applications are encouraged to provide widgets for the following specific `type`s:
 *
 *    - @ref VuoText
 *    - @ref VuoReal
 *    - @ref VuoInteger
 *    - @ref VuoBoolean
 *    - @ref VuoPoint2d
 *    - @ref VuoPoint3d
 *    - @ref VuoImage
 *    - @ref VuoColor
 */
- (NSDictionary *)detailsForPort:(NSString *)portName
{
	const char *portNameCString = [portName UTF8String];
	VuoRunner::Port *port = self.runner->getPublishedInputPortWithName(portNameCString);
	if (!port)
	{
		port = self.runner->getPublishedOutputPortWithName(portNameCString);
		if (!port)
			return nil;
	}

	NSMutableDictionary *details = [[NSMutableDictionary new] autorelease];

	[details setObject:[NSString stringWithUTF8String:port->getName().c_str()] forKey:@"title"];

	[details setObject:[NSString stringWithUTF8String:port->getType().c_str()] forKey:@"type"];

	NSArray *menuItems = [VuoRunnerCocoa menuItemsForType:port->getType()];
	if (menuItems)
		[details setObject:menuItems forKey:@"menuItems"];

	json_object *js = port->getDetails();
	if (js)
	{
		json_object *o = NULL;

		if (json_object_object_get_ex(js, "default", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				[details setObject:cocoaObject forKey:@"default"];
		}

		if (json_object_object_get_ex(js, "suggestedMin", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				[details setObject:cocoaObject forKey:@"suggestedMin"];
		}

		if (json_object_object_get_ex(js, "suggestedMax", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				[details setObject:cocoaObject forKey:@"suggestedMax"];
		}

		if (json_object_object_get_ex(js, "suggestedStep", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				[details setObject:cocoaObject forKey:@"suggestedStep"];
		}
	}

	return details;
}

/**
 * Returns a property list with the current values of the composition's (non-protocol) published input ports.
 */
- (id)propertyListFromInputValues
{
	if ([self isStopped])
		return nil;

	NSMutableDictionary *properties = [[NSMutableDictionary new] autorelease];

	vector<VuoRunner::Port *> ports = self.runner->getPublishedInputPorts();
	for (vector<VuoRunner::Port *>::iterator it = ports.begin(); it != ports.end(); ++it)
		if (self.protocol && !self.protocol->hasInputPort((*it)->getName()))
		{
			json_object *portValue = self.runner->getPublishedInputPortValue(*it);
			const char *valueAsString = json_object_to_json_string_ext(portValue, JSON_C_TO_STRING_PLAIN);
			[properties setObject:[NSString stringWithUTF8String:valueAsString] forKey:[NSString stringWithUTF8String:(*it)->getName().c_str()]];
		}

	return properties;
}

/**
 * Sets the values of the composition's published input ports using a property list.
 *
 * Returns @c NO if restoration of any of the port values failed.
 * (However, in the event of failure, it will try to keep going and set other port values.)
 */
- (BOOL)setInputValuesWithPropertyList:(id)propertyList
{
	if ([self isStopped])
		return NO;

	__block BOOL ok = YES;
	[(NSDictionary *)propertyList enumerateKeysAndObjectsUsingBlock:^(NSString *portName, NSString *value, BOOL *stop) {
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName([portName UTF8String]);
		if (!port)
		{
			ok = NO;
			return;
		}

		json_object *valueJson = json_tokener_parse([value UTF8String]);
		if (!valueJson)
		{
			ok = NO;
			return;
		}

		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}];

	return ok;
}

/**
 * Sets the value of the specified published input port.
 *
 * Returns @c NO if no port with that name exists, or if the value could not be converted.
 *
 * This method accepts the following object types for @c value:
 *
 *    - @c NSNumber, @c CFNumberRef
 *    - @c NSString, @c CFStringRef
 *    - @c NSColor, @c CIColor
 *    - @c NSImage, @c NSBitmapImageRep, @c CGImageRef, @c CVPixelBufferRef, @c CVOpenGLTextureRef
 *    - @c NSPoint or @c CGPoint wrapped in @c NSValue (VuoPoint2d)
 *    - @c SCNVector3 or @c double[3] wrapped in @c NSData or @c CFDataRef (VuoPoint3d)
 *    - @c NSArray or @c CFArrayRef containing a list of one of the above types
 */
- (BOOL)setValue:(id)value forInputPort:(NSString *)portName
{
	if ([self isStopped])
		return NO;

	VuoRunner::Port *port = self.runner->getPublishedInputPortWithName([portName UTF8String]);
	if (!port)
		return NO;

	json_object *valueJson = [VuoRunnerCocoa vuoValueWithCocoaObject:value];
	if (!valueJson)
		return NO;

	self.runner->setPublishedInputPortValue(port, valueJson);
	json_object_put(valueJson);
	return YES;
}

/**
 * Returns the value of the specified published output port.
 *
 * Returns @c nil if no port with that name exists, or if the value could not be converted.
 *
 * This method may return any of the following object types:
 *
 *    - @c NSNumber (VuoBoolean, VuoInteger, VuoReal)
 *    - @c NSString (VuoText and keyed types such as VuoCurve)
 *    - @c NSColor
 *    - @c NSImage
 *    - @c NSPoint wrapped in @c NSValue (VuoPoint2d)
 *    - @c double[3] wrapped in @c NSData (VuoPoint3d)
 *    - @c NSArray containing any of the above types
 *
 * @see glTextureWithTarget:forOutputPort:outputPixelsWide:pixelsHigh: for a more efficient way to retrieve image values.
 */
- (id)valueForOutputPort:(NSString *)portName
{
	if ([self isStopped])
		return nil;

	VuoRunner::Port *port = self.runner->getPublishedOutputPortWithName([portName UTF8String]);
	if (!port)
		return nil;

	json_object *valueJson = self.runner->getPublishedOutputPortValue(port);
	return [VuoRunnerCocoa cocoaObjectWithVuoValue:valueJson ofType:port->getType()];
}

/**
 * Returns the image in the specified published output port, keeping it in GPU VRAM as an OpenGL texture.
 *
 * @c target must be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * Returns @c 0 if no port with that name exists, or if the port does not have type @ref VuoImage.
 *
 * The caller is responsible for deleting the texture when it is finished using it.
 */
- (GLuint)glTextureWithTarget:(GLuint)target forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if ([self isStopped])
		return 0;

	VuoRunner::Port *port = self.runner->getPublishedOutputPortWithName([portName UTF8String]);
	if (!port)
		return 0;

	json_object *valueJson = self.runner->getPublishedOutputPortValue(port);
	VuoImage vuoImage = VuoImage_valueFromJson(valueJson);
	if (!vuoImage)
		return 0;
	json_object_put(valueJson);

	if (target == GL_TEXTURE_RECTANGLE_ARB)
	{
		VuoRetain(vuoImage);
		VuoImage newVuoImage = VuoImage_makeGlTextureRectangleCopy(vuoImage);
		VuoRelease(vuoImage);
		vuoImage = newVuoImage;
	}

	if (outputPixelsWide)
		*outputPixelsWide = vuoImage->pixelsWide;
	if (outputPixelsHigh)
		*outputPixelsHigh = vuoImage->pixelsHigh;

	return vuoImage->glTextureName;
}

@end


@implementation VuoImageFilter
/**
 * Returns @c YES if the specified @c compositionURL can be opened and adheres to the Image Filter protocol.
 */
+ (BOOL)canOpenComposition:(NSURL *)compositionURL
{
	return [self isComposition:compositionURL compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)];
}

/**
 * Opens the specified @c compositionURL, compiles it, and starts it running.
 *
 * Returns @c nil if the composition cannot be opened, or if it does not adhere to the Image Filter protocol.
 *
 * This function opens, parses, and verifies protocol compliance, then returns.
 * Compilation and execution are performed asynchronously.
 */
- (id)initWithComposition:(NSURL *)aCompositionURL
{
	if (!(self = [super initWithComposition:aCompositionURL protocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)]))
		return nil;

	[self compileAndRunAsynchronously];

	return self;
}

/**
 * Does nothing.
 */
static void VuoRunnerCocoa_doNothingCallback(VuoImage imageToFree)
{
}

/**
 * Fires an event through the composition with the specified parameters.
 */
- (BOOL)executeWithGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(unsigned long)pixelsWide pixelsHigh:(unsigned long)pixelsHigh atTime:(NSTimeInterval)time
{
	if ([self isStopped])
		return NO;

	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName("image");
		VuoImage image = VuoImage_makeClientOwned(textureName, GL_RGBA, pixelsWide, pixelsHigh, VuoRunnerCocoa_doNothingCallback, NULL);
		image->glTextureTarget = target;
		VuoRetain(image);
		json_object *valueJson = VuoImage_interprocessJsonFromValue(image);
		VuoRelease(image);
		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}

	return [self executeAtTime:time];
}

/**
 * Fires an event through the composition at the specified time.
 */
- (BOOL)executeAtTime:(NSTimeInterval)time
{
	if ([self isStopped])
		return NO;

	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName("time");
		json_object *valueJson = VuoReal_jsonFromValue(time);
		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}

	self.runner->firePublishedInputPortEvent();
	self.runner->waitForAnyPublishedOutputPortEvent();
	return YES;
}

/**
 * Sends @c image to the Vuo composition,
 * instructs the Vuo composition to filter it
 * at the specified logical @c time (number of seconds since rendering started),
 * and returns the filtered image.
 *
 * This method requires a roundtrip between CPU and GPU RAM, which is slow.
 * If possible, use @ref filterGLTexture:target:pixelsWide:pixelsHigh:atTime:outputPixelsWide:pixelsHigh: instead.
 *
 * @note The Vuo composition is not required to produce an image
 * with the same dimensions as the input image
 * (it is a hint, not a guarantee).
 */
- (NSImage *)filterNSImage:(NSImage *)image atTime:(NSTimeInterval)time
{
	[self setValue:image forInputPort:@"image"];

	if (![self executeAtTime:time])
		return nil;

	return [self valueForOutputPort:@"outputImage"];
}

/**
 * Sends @c textureName to the Vuo composition,
 * instructs the Vuo composition to filter it
 * at the specified logical @c time (number of seconds since rendering started),
 * and returns the filtered image (attached to `target`).
 *
 * @c textureName remains owned by the caller.  It may be deleted or recycled after this method returns.
 *
 * @c target must be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @note The Vuo composition is not required to produce an image
 * with the same dimensions as the input image
 * (it is a hint, not a guarantee).
 */
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if (![self executeWithGLTexture:textureName target:target pixelsWide:pixelsWide pixelsHigh:pixelsHigh atTime:time])
		return 0;
	return [self glTextureWithTarget:target forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh];
}

@end


@implementation VuoImageGenerator
/**
 * Returns @c YES if the specified @c compositionURL can be opened and adheres to the Image Generator protocol.
 */
+ (BOOL)canOpenComposition:(NSURL *)compositionURL
{
	return [self isComposition:compositionURL compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)];
}

/**
 * Opens the specified @c compositionURL, compiles it, and starts it running.
 *
 * Returns @c nil if the composition cannot be opened, or if it does not adhere to the Image Generator protocol.
 *
 * This function opens, parses, and verifies protocol compliance, then returns.
 * Compilation and execution are performed asynchronously.
 */
- (id)initWithComposition:(NSURL *)aCompositionURL
{
	if (!(self = [super initWithComposition:aCompositionURL protocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)]))
		return nil;

	[self compileAndRunAsynchronously];

	return self;
}

/**
 * Fires an event through the composition with the specified parameters.
 */
- (BOOL)executeWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time
{
	if ([self isStopped])
		return NO;

	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName("width");
		json_object *valueJson = VuoInteger_jsonFromValue(suggestedPixelsWide);
		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}

	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName("height");
		json_object *valueJson = VuoInteger_jsonFromValue(suggestedPixelsHigh);
		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}

	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName("time");
		json_object *valueJson = VuoReal_jsonFromValue(time);
		self.runner->setPublishedInputPortValue(port, valueJson);
		json_object_put(valueJson);
	}

	self.runner->firePublishedInputPortEvent();
	self.runner->waitForAnyPublishedOutputPortEvent();
	return YES;
}

/**
 * Instructs the Vuo composition to generate an image with the specified dimensions
 * at the specified logical @c time (number of seconds since rendering started),
 * and returns the generated image.
 *
 * This method requires a roundtrip between CPU and GPU RAM, which is slow.
 * If possible, use @ref generateGLTextureWithTarget:suggestedPixelsWide:pixelsHigh:atTime:outputPixelsWide:pixelsHigh: instead.
 *
 * @note The Vuo composition is not required to use the specified dimensions
 * (it is a hint, not a guarantee).
 */
- (NSImage *)generateNSImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time
{
	if (![self executeWithSuggestedPixelsWide:suggestedPixelsWide pixelsHigh:suggestedPixelsHigh atTime:time])
		return nil;
	return [self valueForOutputPort:@"outputImage"];
}

/**
 * Instructs the Vuo composition to generate an image with the specified dimensions
 * at the specified logical @c time (number of seconds since rendering started),
 * and returns the generated image.
 *
 * @c target must be either @c GL_TEXTURE_2D or @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @note The Vuo composition is not required to use the specified dimensions
 * (it is a hint, not a guarantee).
 */
- (GLuint)generateGLTextureWithTarget:(GLuint)target suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if (![self executeWithSuggestedPixelsWide:suggestedPixelsWide pixelsHigh:suggestedPixelsHigh atTime:time])
		return 0;
	return [self glTextureWithTarget:target forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh];
}

@end
