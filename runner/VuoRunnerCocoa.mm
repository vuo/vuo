/**
 * @file
 * VuoCocoa implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRunnerCocoa.h"
#include "VuoRunnerCocoa+Conversion.hh"

#include <OpenGL/CGLMacro.h>
#include "VuoMacOSSDKWorkaround.h"
#include <QuartzCore/CoreImage.h>

#include <dispatch/dispatch.h>

#include <zmq/zmq.h>
#include <vector>
#include <set>
#include "VuoRunner.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerIssue.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoException.hh"
#include "VuoProtocol.hh"
#include "VuoHeap.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

extern "C" {
#include "VuoInteger.h"
#include "VuoImageGet.h"
}

bool VuoRunnerCocoa_hasInitializedCompilerCache = false;  ///< Tracks whether the one-time cache initialization has been performed.

/**
 * Private extensions to @ref VuoRunnerCocoa.
 */
@interface VuoRunnerCocoa ()
@property dispatch_queue_t runnerQueue;	///< Initializes the runner; ensures that users of the runner execute after initialization.
@property VuoRunner *runner;
@property (retain) NSURL *compositionURL;
@property (retain) NSString *compositionString;
@property (retain) NSString *compositionSourcePath;
@property (retain) NSString *compositionProcessName;
@property (retain) NSString *compositionName;
@property (retain) NSString *compositionDescription;
@property (retain) NSString *compositionCopyright;
@property VuoProtocol *protocol;
@property set<VuoRunner::Port *> *changedPorts;  ///< Tracks changes to published input port values to know which ports the next event should be fired through.
@end

@implementation VuoRunnerCocoa
@synthesize runnerQueue;
@synthesize runner;
@synthesize compositionURL;
@synthesize compositionString;
@synthesize compositionSourcePath;
@synthesize compositionProcessName;
@synthesize compositionName;
@synthesize compositionDescription;
@synthesize compositionCopyright;
@synthesize protocol;
@synthesize changedPorts;

/**
 * Specifies an OpenGL context to be used as the base for all of Vuo's shared GL contexts.
 *
 * The `CGLContext` must be unlocked when calling this function,
 * but after that you may lock it at any time (Vuo doesn't require it to be locked or unlocked).
 *
 * Must be called before any Vuo composition is loaded.
 *
 * @threadAny
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (void)setGlobalRootContext:(CGLContextObj)context
{
	VuoGlContext_setGlobalRootContext(context);
}

/**
 * Asynchronously makes the module caches available to avoid a delay the first time a composition is compiled and run.
 *
 * @see VuoCompiler::prepareModuleCaches()
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (void)prepareModuleCaches
{
	@synchronized(self)
	{
		if (! VuoRunnerCocoa_hasInitializedCompilerCache)
		{
			VuoCompiler c;
			c.prepareModuleCaches();
			VuoRunnerCocoa_hasInitializedCompilerCache = true;
		}
	}
}

/**
 * Downloads and parses the specified `compositionURL`, to check whether it's compliant with `protocol`.
 *
 * @vuoRunnerCocoaDeprecated
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
 * Downloads and parses the specified `compositionURL`, to check whether it's compliant with `protocol`.
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (BOOL)isCompositionString:(NSString *)compositionString compliantWithProtocol:(VuoProtocol *)protocol
{
	VuoRunnerCocoa *runner = [[VuoRunnerCocoa alloc] initWithCompositionString:compositionString name:@"" sourcePath:nil protocol:protocol];
	if (!runner)
		return NO;

	[runner release];
	return YES;
}

/**
 * Opens and parses the specified @c compositionURL.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithComposition:(NSURL *)theCompositionURL protocol:(VuoProtocol *)theProtocol
{
	NSError *error;
	NSString *s = [NSString stringWithContentsOfURL:theCompositionURL encoding:NSUTF8StringEncoding error:&error];
	if (!s)
	{
		NSLog(@"[VuoRunnerCocoa initWithComposition:protocol:] Error reading composition: %@", error);
		[self release];
		return nil;
	}

	self = [self initWithCompositionString:s name:[theCompositionURL lastPathComponent] sourcePath:nil protocol:theProtocol];
	if (!self)
		return nil;

	self.compositionURL = theCompositionURL;

	return self;
}

/**
 * Parses the specified @c compositionString.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithCompositionString:(NSString *)theCompositionString name:(NSString *)processName sourcePath:(NSString *)sourcePath protocol:(VuoProtocol *)theProtocol
{
	if (!(self = [super init]))
		return nil;

	self.protocol = theProtocol;

	self.runnerQueue = dispatch_queue_create("org.vuo.runner.cocoa", NULL);
	self.runner = NULL;

	self.compositionString = theCompositionString;
	const char *compositionCString = [self.compositionString UTF8String];
	if (!theProtocol->isCompositionCompliant(compositionCString))
	{
		[self release];
		return nil;
	}

	self.compositionProcessName = processName;
	self.compositionSourcePath = sourcePath;

	VuoCompositionMetadata metadata(compositionCString);
	self.compositionName        = [NSString stringWithUTF8String:metadata.getName().c_str()];
	self.compositionDescription = [NSString stringWithUTF8String:metadata.getDescription().c_str()];
	self.compositionCopyright   = [NSString stringWithUTF8String:metadata.getCopyright().c_str()];

	self.changedPorts = new set<VuoRunner::Port *>();

	return self;
}

/**
 * Frees instance data.
 *
 * @vuoRunnerCocoaDeprecated
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
	[compositionURL release];
	[compositionString release];
	[compositionSourcePath release];
	[compositionName release];
	[compositionDescription release];
	[compositionCopyright release];
	delete self.changedPorts;
	[super dealloc];
}

- (void)compileAndRun
{
	[VuoRunnerCocoa prepareModuleCaches];

	dispatch_sync(self.runnerQueue, ^{
		VuoCompilerIssues issues;
		if (self.compositionURL)
			self.runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(
				[[self.compositionURL path] UTF8String],
				&issues);
		else
			self.runner = VuoCompiler::newSeparateProcessRunnerFromCompositionString(
				[self.compositionString UTF8String],
				[self.compositionProcessName UTF8String],
				[self.compositionSourcePath UTF8String],
				&issues);

		if (!issues.isEmpty())
			VUserLog("%s", issues.getLongDescription(false).c_str());

		if (self.runner)
		{
			self.runner->start();
			self.runner->subscribeToEventTelemetry("");

			try
			{
				vector<VuoRunner::Port *> allInputPorts = self.runner->getPublishedInputPorts();
				std::copy_if(allInputPorts.begin(), allInputPorts.end(), std::inserter(*self.changedPorts, self.changedPorts->begin()),
						  [] (VuoRunner::Port *port) { return ! port->getType().empty(); });
			}
			catch (VuoException &e)
			{
				VUserLog("Error: %s", e.what());
			}
		}
	});
}

/**
 * Returns YES if the composition failed to build and run successfully, or if it was running and has stopped.
 *
 * @vuoRunnerCocoaDeprecated
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
 *
 * @vuoRunnerCocoaDeprecated
 */
- (NSString *)description
{
    return [NSString stringWithFormat:@"VuoRunnerCocoa(%c%@%c)", '"', compositionName, '"'];
}

/**
 * Returns an array listing the names of the composition's published input ports,
 * excluding those used by protocols.
 *
 * @vuoRunnerCocoaDeprecated
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
 *
 * @vuoRunnerCocoaDeprecated
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
 *    - @c menuItems — value is an @c NSArray where each element is one of the following:
 *       - an `NSDictionary` with 2 keys: `value` (NSString or NSNumber: identifier) and `name` (NSString: display name)
 *       - the NSString `---` — a menu separator line
 *       - any other NSString — a non-selectable menu label, for labeling multiple sections within the menu
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
 *
 * @vuoRunnerCocoaDeprecated
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

	details[@"title"] = [NSString stringWithUTF8String:port->getName().c_str()];

	details[@"type"] = [NSString stringWithUTF8String:port->getType().c_str()];

	json_object *js = port->getDetails();
	if (js)
	{
		json_object *o = NULL;

		if (json_object_object_get_ex(js, "default", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				details[@"default"] = cocoaObject;
		}

		if (json_object_object_get_ex(js, "suggestedMin", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				details[@"suggestedMin"] = cocoaObject;
		}

		if (json_object_object_get_ex(js, "suggestedMax", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				details[@"suggestedMax"] = cocoaObject;
		}

		if (json_object_object_get_ex(js, "suggestedStep", &o))
		{
			id cocoaObject = [VuoRunnerCocoa cocoaObjectWithVuoValue:o ofType:port->getType()];
			if (cocoaObject)
				details[@"suggestedStep"] = cocoaObject;
		}

		if (json_object_object_get_ex(js, "menuItems", &o))
		{
			NSMutableArray *menuItems = [NSMutableArray new];

			int itemCount = json_object_array_length(o);
			for (int i = 0; i < itemCount; ++i)
			{
				json_object *item = json_object_array_get_idx(o, i);
				id menuItem = nil;

				if (json_object_is_type(item, json_type_object))
				{
					json_object *value;
					json_object_object_get_ex(item, "value", &value);
					json_object *name;
					json_object_object_get_ex(item, "name", &name);

					id valueNS = nil;
					if (json_object_is_type(value, json_type_string))
						valueNS = [NSString stringWithUTF8String:json_object_get_string(value)];
					else if (json_object_is_type(value, json_type_int))
						valueNS = [NSNumber numberWithLong:json_object_get_int64(value)];

					const char *summary = json_object_get_string(name);
					menuItem = @{
						@"value": valueNS,
						@"name": [NSString stringWithUTF8String:summary],
					};
				}
				else if (json_object_is_type(item, json_type_string))
					menuItem = [NSString stringWithUTF8String:json_object_get_string(item)];

				if (menuItem)
					[menuItems addObject:menuItem];
			}

			details[@"menuItems"] = menuItems;
			[menuItems release];
		}
	}

	return details;
}

/**
 * Returns a property list with the current values of the composition's (non-protocol) published input ports.
 *
 * @vuoRunnerCocoaDeprecated
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
			properties[[NSString stringWithUTF8String:(*it)->getName().c_str()]] = [NSString stringWithUTF8String:valueAsString];
		}

	return properties;
}

/**
 * Sets the values of the composition's published input ports using a property list.
 *
 * Returns @c NO if restoration of any of the port values failed.
 * (However, in the event of failure, it will try to keep going and set other port values.)
 *
 * @vuoRunnerCocoaDeprecated
 */
- (BOOL)setInputValuesWithPropertyList:(id)propertyList
{
	if ([self isStopped])
		return NO;

	__block BOOL ok = YES;
	__block map<VuoRunner::Port *, json_object *> m;
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

		m[port] = valueJson;
	}];

	runner->setPublishedInputPortValues(m);

	for (auto &kv : m)
	{
		self.changedPorts->insert(kv.first);
		json_object_put(kv.second);
	}

	return ok;
}

/**
 * Sets the value of one or more published input ports.
 *
 * Returns @c NO if any of the port names do not exist, or if any of the values could not be converted.
 *
 * This method accepts the following object types:
 *
 *    - @c NSNumber, @c CFNumberRef
 *    - @c NSString, @c CFStringRef
 *    - @c NSColor, @c CIColor
 *    - @c NSImage, @c NSBitmapImageRep, @c CGImageRef, @c CVPixelBufferRef, @c CVOpenGLTextureRef
 *    - @c NSPoint or @c CGPoint wrapped in @c NSValue (VuoPoint2d)
 *    - @c SCNVector3 or @c double[3] wrapped in @c NSData or @c CFDataRef (VuoPoint3d)
 *    - @c NSArray or @c CFArrayRef containing a list of one of the above types
 *
 * @vuoRunnerCocoaDeprecated
 */
- (BOOL)setInputValues:(NSDictionary *)namesAndValues
{
	if ([self isStopped])
		return NO;

	BOOL ok = YES;
	map<VuoRunner::Port *, json_object *> m;
	for (NSString *portName in namesAndValues)
	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName([portName UTF8String]);
		if (!port)
		{
			ok = NO;
			continue;
		}

		id value = namesAndValues[portName];
		json_object *valueJson = [VuoRunnerCocoa vuoValueWithCocoaObject:value];
		if (!valueJson)
		{
			ok = NO;
			continue;
		}

		m[port] = valueJson;
	}

	self.runner->setPublishedInputPortValues(m);

	for (auto &kv : m)
	{
		self.changedPorts->insert(kv.first);
		json_object_put(kv.second);
	}

	return ok;
}

/**
 * Sets the value of one or more published input ports using `json_object *` values wrapped in `NSValue`.
 *
 * Returns @c NO if any of the port names do not exist.
 *
 * The `json_object` remains owned by the caller (this method doesn't change its reference count).
 *
 * @vuoRunnerCocoaDeprecated
 */
- (BOOL)setInputJSON:(NSDictionary *)namesAndJSON
{
	if ([self isStopped])
		return NO;

	BOOL ok = YES;
	map<VuoRunner::Port *, json_object *> m;
	for (NSString *portName in namesAndJSON)
	{
		VuoRunner::Port *port = self.runner->getPublishedInputPortWithName([portName UTF8String]);
		if (!port)
		{
			ok = NO;
			continue;
		}

		id nsValue = namesAndJSON[portName];
		json_object *value = (json_object *)[nsValue pointerValue];

		// If we're feeding a string to a VuoImage port, assume it's an image URL, and try to load it.
		if (port->getType() == "VuoImage")
		{
			if (json_object_get_type(value) == json_type_string)
			{
				char *s = strdup(json_object_get_string(value));
				VuoImage image = VuoImage_get(s);
				if (!image)
				{
					NSLog(@"Error: Couldn't load image '%s'.\n", s);
					free(s);
					ok = NO;
					continue;
				}
				free(s);

				m[port] = VuoImage_getInterprocessJson(image);
			}
		}
		else
			m[port] = value;
	}

	self.runner->setPublishedInputPortValues(m);

	for (auto &kv : m)
		self.changedPorts->insert(kv.first);

	return ok;
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
 *
 * @vuoRunnerCocoaDeprecated
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
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)glTextureWithTarget:(GLuint)target forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if ([self isStopped])
		return 0;

	VuoRunner::Port *port = self.runner->getPublishedOutputPortWithName([portName UTF8String]);
	if (!port)
		return 0;

	json_object *valueJson = self.runner->getPublishedOutputPortValue(port);
	VuoImage vuoImage = VuoImage_makeFromJson(valueJson);
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

	VuoGlTexture_disown(vuoImage->glTextureName);

	return vuoImage->glTextureName;
}

/**
 * Retrieves the image in the specified published output port, keeping it in GPU VRAM as an OpenGL texture.
 *
 * Returns `0` if the composition is stopped, if no port with that name exists, if the port does not have type `VuoImage`,
 * or if the image could not be converted.
 * Otherwise, returns the host-owned OpenGL texture name whose `GL_TEXTURE_RECTANGLE_ARB` target has been populated.
 *
 * @param provider A block that returns an OpenGL texture name with the requested width and height.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param portName The name of an output port.
 * @param[out] outputPixelsWide Upon return, this contains the width of the output texture.
 * @param[out] outputPixelsHigh Upon return, this contains the height of the output texture.
 * @param[out] outputIOSurface Upon return, this contains the IOSurface backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurface:
 *
 *                     VuoIoSurfacePool_signal(outputIOSurface);
 *                     CFRelease(outputIOSurface);
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)glTextureFromProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider forOutputPort:(NSString *)portName outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface
{
	if ([self isStopped])
		return 0;

	VuoRunner::Port *port = self.runner->getPublishedOutputPortWithName([portName UTF8String]);
	if (!port)
		return 0;

	json_object *valueJson = self.runner->getPublishedOutputPortValue(port);

	GLuint (^provider2)(unsigned int, unsigned int) = ^GLuint(unsigned int pixelsWide, unsigned int pixelsHigh){
		return provider(pixelsWide, pixelsHigh);
	};
	unsigned int outputPixelsWide2, outputPixelsHigh2;
	GLuint outputTexture = VuoImage_resolveInterprocessJsonUsingTextureProvider(valueJson, provider2, &outputPixelsWide2, &outputPixelsHigh2, outputIOSurface);
	*outputPixelsWide = outputPixelsWide2;
	*outputPixelsHigh = outputPixelsHigh2;
	return outputTexture;
}

@end


@implementation VuoImageFilter
/**
 * Returns @c YES if the specified @c compositionURL can be opened and adheres to the Image Filter protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (BOOL)canOpenComposition:(NSURL *)compositionURL
{
	return [self isComposition:compositionURL compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)];
}

/**
 * Returns @c YES if the specified @c compositionString and adheres to the Image Filter protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (BOOL)canOpenCompositionString:(NSString *)compositionString
{
	return [self isCompositionString:compositionString compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)];
}

/**
 * Opens the specified @c compositionURL, compiles it, and starts it running.
 *
 * Returns @c nil if the composition cannot be opened, if it does not adhere to the Image Filter protocol,
 * or if it cannot be compiled.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithComposition:(NSURL *)aCompositionURL
{
	if (!(self = [super initWithComposition:aCompositionURL protocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)]))
		return nil;

	[self compileAndRun];
	if (! self.runner)
		return nil;

	return self;
}

/**
 * Compiles the specified @c compositionString and starts it running.
 *
 * `name` specifies the running process should have.
 *
 * Returns @c nil if the composition does not adhere to the Image Filter protocol, or if it cannot be compiled.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithCompositionString:(NSString *)aCompositionString name:(NSString *)name sourcePath:(NSString *)sourcePath
{
	if (!(self = [super initWithCompositionString:aCompositionString name:name sourcePath:sourcePath protocol:VuoProtocol::getProtocol(VuoProtocol::imageFilter)]))
		return nil;

	[self compileAndRun];
	if (! self.runner)
		return nil;

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
 *
 * @vuoRunnerCocoaDeprecated
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
		json_object *valueJson = VuoImage_getInterprocessJson(image);
		VuoRelease(image);
		map<VuoRunner::Port *, json_object *> m;
		m[port] = valueJson;
		self.runner->setPublishedInputPortValues(m);
		json_object_put(valueJson);

		self.changedPorts->insert(port);
	}

	return [self executeAtTime:time];
}

/**
 * Fires an event through the composition at the specified time.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (BOOL)executeAtTime:(NSTimeInterval)time
{
	if ([self isStopped])
		return NO;

	VuoRunner::Port *timePort = self.runner->getPublishedInputPortWithName("time");

	{
		json_object *valueJson = VuoReal_getJson(time);
		map<VuoRunner::Port *, json_object *> m;
		m[timePort] = valueJson;
		self.runner->setPublishedInputPortValues(m);
		json_object_put(valueJson);

		self.changedPorts->insert(timePort);
	}

	self.runner->firePublishedInputPortEvent(*self.changedPorts);
	self.changedPorts->clear();

	self.runner->waitForFiredPublishedInputPortEvent();
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
 *
 * @vuoRunnerCocoaDeprecated
 */
- (NSImage *)filterNSImage:(NSImage *)image atTime:(NSTimeInterval)time
{
	[self setInputValues:@{@"image":image}];

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
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if (![self executeWithGLTexture:textureName target:target pixelsWide:pixelsWide pixelsHigh:pixelsHigh atTime:time])
		return 0;
	return [self glTextureWithTarget:target forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh];
}

/**
 * Sends @c textureName to the Vuo composition,
 * instructs the Vuo composition to filter it
 * at the specified logical @c time (number of seconds since rendering started),
 * requests that the host provide a texture of the appropriate size,
 * and returns that texture with image data attached to its `GL_TEXTURE_RECTANGLE_ARB` target.
 *
 * @param textureName The OpenGL texture name to filter.  Remains owned by the caller.
 *                    The host app may delete or recycle it after this method returns.
 * @param target      The target to which the input image is bound.
 *                    Must be either `GL_TEXTURE_2D` or `GL_TEXTURE_RECTANGLE_ARB`.
 * @param pixelsWide  The input image's width.
 * @param pixelsHigh  The input image's height.
 * @param time        The logical time at which to filter the image.
 * @param provider A block that returns an OpenGL texture name with the requested width and height.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param[out] outputPixelsWide Upon return, this contains the width of the output texture.
 * @param[out] outputPixelsHigh Upon return, this contains the height of the output texture.
 * @param[out] outputIOSurface Upon return, this contains the IOSurface backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurface:
 *
 *                     VuoIoSurfacePool_signal(outputIOSurface);
 *                     CFRelease(outputIOSurface);
 *
 * @note The Vuo composition is not required to produce an image
 * with the same dimensions as the input image
 * (it is a hint, not a guarantee).
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)filterGLTexture:(GLuint)textureName target:(GLuint)target pixelsWide:(NSUInteger)pixelsWide pixelsHigh:(NSUInteger)pixelsHigh atTime:(NSTimeInterval)time withTextureProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface
{
	if (![self executeWithGLTexture:textureName target:target pixelsWide:pixelsWide pixelsHigh:pixelsHigh atTime:time])
		return 0;

	return [self glTextureFromProvider:provider forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh ioSurface:outputIOSurface];
}

@end


/**
 * Private extensions to @ref VuoImageGenerator.
 */
@interface VuoImageGenerator ()
@property NSUInteger previousSuggestedPixelsWide;  ///< Tracks changes to image width to know if the next event should be fired through that published input port.
@property NSUInteger previousSuggestedPixelsHigh;  ///< Tracks changes to image height to know if the next event should be fired through that published input port.
@end

@implementation VuoImageGenerator
@synthesize previousSuggestedPixelsWide;
@synthesize previousSuggestedPixelsHigh;

/**
 * Returns @c YES if the specified @c compositionURL can be opened and adheres to the Image Generator protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (BOOL)canOpenComposition:(NSURL *)compositionURL
{
	return [self isComposition:compositionURL compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)];
}

/**
 * Returns @c YES if the specified @c compositionString and adheres to the Image Generator protocol.
 *
 * @vuoRunnerCocoaDeprecated
 */
+ (BOOL)canOpenCompositionString:(NSString *)compositionString
{
	return [self isCompositionString:compositionString compliantWithProtocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)];
}

/**
 * Opens the specified @c compositionURL, compiles it, and starts it running.
 *
 * Returns @c nil if the composition cannot be opened, if it does not adhere to the Image Generator protocol,
 * or if it cannot be compiled.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithComposition:(NSURL *)aCompositionURL
{
	if (!(self = [super initWithComposition:aCompositionURL protocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)]))
		return nil;

	[self compileAndRun];
	if (! self.runner)
		return nil;

	self.previousSuggestedPixelsWide = NSUIntegerMax;
	self.previousSuggestedPixelsHigh = NSUIntegerMax;

	return self;
}

/**
 * Compiles the specified @c compositionString and starts it running.
 *
 * `name` specifies the running process should have.
 *
 * Returns @c nil if the composition does not adhere to the Image Generator protocol, or if it cannot be compiled.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (id)initWithCompositionString:(NSString *)aCompositionString name:(NSString *)name sourcePath:(NSString *)sourcePath
{
	if (!(self = [super initWithCompositionString:aCompositionString name:name sourcePath:sourcePath protocol:VuoProtocol::getProtocol(VuoProtocol::imageGenerator)]))
		return nil;

	[self compileAndRun];
	if (! self.runner)
		return nil;

	return self;
}

/**
 * Fires an event through the composition with the specified parameters.
 *
 * @vuoRunnerCocoaDeprecated
 */
- (BOOL)executeWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time
{
	if ([self isStopped])
		return NO;

	VuoRunner::Port *timePort = self.runner->getPublishedInputPortWithName("time");
	VuoRunner::Port *widthPort = self.runner->getPublishedInputPortWithName("width");
	VuoRunner::Port *heightPort = self.runner->getPublishedInputPortWithName("height");

	map<VuoRunner::Port *, json_object *> m;

	m[timePort] = VuoReal_getJson(time);

	if (suggestedPixelsWide != self.previousSuggestedPixelsWide)
	{
		m[widthPort] = VuoInteger_getJson(suggestedPixelsWide);
		self.previousSuggestedPixelsWide = suggestedPixelsWide;
	}
	if (suggestedPixelsHigh != self.previousSuggestedPixelsHigh)
	{
		m[heightPort] = VuoInteger_getJson(suggestedPixelsHigh);
		self.previousSuggestedPixelsHigh = suggestedPixelsHigh;
	}

	self.runner->setPublishedInputPortValues(m);
	for (auto &kv : m)
	{
		self.changedPorts->insert(kv.first);
		json_object_put(kv.second);
	}

	self.runner->firePublishedInputPortEvent(*self.changedPorts);
	self.changedPorts->clear();

	self.runner->waitForFiredPublishedInputPortEvent();
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
 *
 * @vuoRunnerCocoaDeprecated
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
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)generateGLTextureWithTarget:(GLuint)target suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh
{
	if (![self executeWithSuggestedPixelsWide:suggestedPixelsWide pixelsHigh:suggestedPixelsHigh atTime:time])
		return 0;
	return [self glTextureWithTarget:target forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh];
}

/**
 * Instructs the Vuo composition to generate an image with the specified dimensions
 * at the specified logical @c time (number of seconds since rendering started),
 * requests that the host provide a texture of the appropriate size,
 * and returns that texture with image data attached to its `GL_TEXTURE_RECTANGLE_ARB` target.
 *
 * @param suggestedPixelsWide  The suggested width of the output image.
 * @param suggestedPixelsHigh  The suggested height of the output image.
 * @param time        The logical time at which to filter the image.
 * @param provider A block that returns an OpenGL texture name with the requested width and height.
 *                 The host app must not call `glTexImage2D()` on the texture,
 *                 since this makes the texture incompatible with the IOSurface backing.
 * @param[out] outputPixelsWide Upon return, this contains the width of the output texture.
 * @param[out] outputPixelsHigh Upon return, this contains the height of the output texture.
 * @param[out] outputIOSurface Upon return, this contains the IOSurface backing the output texture.
 *                 When the host app is finished with the output texture, it must signal and release the IOSurface:
 *
 *                     VuoIoSurfacePool_signal(outputIOSurface);
 *                     CFRelease(outputIOSurface);
 *
 * @note The Vuo composition is not required to use the specified dimensions
 * (it is a hint, not a guarantee).
 *
 * @vuoRunnerCocoaDeprecated
 */
- (GLuint)generateGLTextureWithProvider:(GLuint (^)(NSUInteger pixelsWide, NSUInteger pixelsHigh))provider suggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time outputPixelsWide:(NSUInteger *)outputPixelsWide pixelsHigh:(NSUInteger *)outputPixelsHigh ioSurface:(IOSurfaceRef *)outputIOSurface
{
	if (![self executeWithSuggestedPixelsWide:suggestedPixelsWide pixelsHigh:suggestedPixelsHigh atTime:time])
		return 0;
	return [self glTextureFromProvider:provider forOutputPort:@"outputImage" outputPixelsWide:outputPixelsWide pixelsHigh:outputPixelsHigh ioSurface:outputIOSurface];
}

/**
 * Instructs the Vuo composition to generate an image with the specified dimensions
 * at the specified logical @c time (number of seconds since rendering started),
 * and returns the generated image.
 *
 * @note The Vuo composition is not required to use the specified dimensions
 * (it is a hint, not a guarantee).
 *
 * @vuoRunnerCocoaDeprecated
 */
- (VuoImage)generateVuoImageWithSuggestedPixelsWide:(NSUInteger)suggestedPixelsWide pixelsHigh:(NSUInteger)suggestedPixelsHigh atTime:(NSTimeInterval)time
{
	if (![self executeWithSuggestedPixelsWide:suggestedPixelsWide pixelsHigh:suggestedPixelsHigh atTime:time])
		return NULL;


	VuoRunner::Port *port = self.runner->getPublishedOutputPortWithName("outputImage");
	if (!port)
		return NULL;

	json_object *valueJson = self.runner->getPublishedOutputPortValue(port);
	VuoImage image = VuoImage_makeFromJson(valueJson);
	json_object_put(valueJson);

	return image;
}

@end
