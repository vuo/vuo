/**
 * @file
 * VuoSyphonServerNotifier implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindow.h"
#import "VuoSyphon.h"
#import "VuoSyphonServerNotifier.h"

#define NS_RETURNS_INNER_POINTER
#import <Syphon.h>

#import <objc/message.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSyphonServerNotifier",
					 "dependencies" : [
						"VuoWindow",
						"VuoSyphon",
						"Syphon.framework",
						"objc",
						"AppKit.framework"
					 ]
				 });
#endif


/**
 * Internal implementation of @ref VuoSyphonServerNotifier.
 */
@interface VuoSyphonServerNotifierInternal : NSObject
{
	void (*serversChangedCallback)(VuoList_VuoSyphonServerDescription);  ///< Called when the set of available Syphon servers has changed.
	id serversChangedObject;  ///< Object for serversChangedMethod.
	SEL serversChangedMethod;  ///< Called when the set of available Syphon servers has changed.
}

- (id)init;
- (void)setNotificationFunction:(void (*)(VuoList_VuoSyphonServerDescription))serversChangedCallback;
- (void)setNotificationObject:(id)object method:(SEL)method;
- (void)start;
- (void)stop;

@end


@implementation VuoSyphonServerNotifierInternal

/**
 * Creates a notifier object that has not yet begun tracking notifications.
 */
- (id)init
{
	if (self = [super init])
	{
		serversChangedCallback = NULL;
		serversChangedObject = nil;
		serversChangedMethod = NULL;
	}
	return self;
}

/**
 * Sets the function to call when the set of available Syphon servers has changed.
 */
- (void)setNotificationFunction:(void (*)(VuoList_VuoSyphonServerDescription))_serversChangedCallback
{
	serversChangedCallback = _serversChangedCallback;

	[serversChangedObject release];
	serversChangedObject = nil;
	serversChangedMethod = NULL;
}

/**
 * Sets the method to call when the set of available Syphon servers has changed.
 */
- (void)setNotificationObject:(id)object method:(SEL)method
{
	serversChangedObject = [object retain];
	serversChangedMethod = method;

	serversChangedCallback = NULL;
}

/**
 * Called when the set of available Syphon servers has changed.
 */
- (void)serversChanged
{
	VuoList_VuoSyphonServerDescription descriptions = VuoSyphon_getAvailableServerDescriptions();

	if (serversChangedCallback)
		serversChangedCallback(descriptions);
	else if (serversChangedMethod)
		objc_msgSend(serversChangedObject, serversChangedMethod, descriptions);
}

/**
 * Called when a new Syphon server has become available.
 */
-(void) syphonServerAnnounced:(NSNotification *)notif
{
   [self serversChanged];
}

/**
 * Called when a Syphon server's name has changed.
 */
-(void) syphonServerUpdated:(NSNotification *)notif
{
   [self serversChanged];
}

/**
 * Called when a Syphon server has become unavailable.
 */
-(void) syphonServerRetired:(NSNotification *)notif
{
   [self serversChanged];
}

/**
 * Starts tracking Syphon servers.
 */
- (void)start
{
	VuoApp_init();

	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(syphonServerAnnounced:)
		name:SyphonServerAnnounceNotification
		object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(syphonServerUpdated:)
		name:SyphonServerUpdateNotification
		object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(syphonServerRetired:)
		name:SyphonServerRetireNotification
		object:nil];
}

/**
 * Stops tracking Syphon servers.
 */
- (void)stop
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:SyphonServerAnnounceNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:SyphonServerUpdateNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:SyphonServerRetireNotification object:nil];
}

/**
 * Frees memory.
 */
- (void)dealloc
{
	[serversChangedObject release];
	[super dealloc];
}

@end


/**
 * Frees memory.
 */
void VuoSyphonServerNotifier_free(VuoSyphonServerNotifier serverNotifier)
{
	[(VuoSyphonServerNotifierInternal *)serverNotifier release];
}

/**
 * Creates a notifier object that has not yet begun tracking notifications.
 */
VuoSyphonServerNotifier VuoSyphonServerNotifier_make(void)
{
	VuoSyphonServerNotifierInternal *serverNotifier = [[VuoSyphonServerNotifierInternal alloc] init];
	VuoRegister(serverNotifier, VuoSyphonServerNotifier_free);
	return serverNotifier;
}

/**
 * Sets the function to call when the set of available Syphon servers has changed.
 */
void VuoSyphonServerNotifier_setNotificationFunction(VuoSyphonServerNotifier serverNotifier,
													 VuoOutputTrigger(serversChanged, VuoList_VuoSyphonServerDescription))
{
	[(VuoSyphonServerNotifierInternal *)serverNotifier setNotificationFunction:serversChanged];
}

/**
 * Sets the method to call when the set of available Syphon servers has changed.
 */
void VuoSyphonServerNotifier_setNotificationMethod(VuoSyphonServerNotifier serverNotifier,
												   id object, SEL method)
{
	[(VuoSyphonServerNotifierInternal *)serverNotifier setNotificationObject:object method:method];
}

/**
 * Starts tracking Syphon servers.
 */
void VuoSyphonServerNotifier_start(VuoSyphonServerNotifier serverNotifier)
{
	[(VuoSyphonServerNotifierInternal *)serverNotifier start];
}

/**
 * Stops tracking Syphon servers.
 */
void VuoSyphonServerNotifier_stop(VuoSyphonServerNotifier serverNotifier)
{
	[(VuoSyphonServerNotifierInternal *)serverNotifier stop];
}
