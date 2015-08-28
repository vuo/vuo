/**
 * @file
 * VuoSyphonInternal implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoSyphon.h"
#import "VuoSyphonListener.h"
#include "VuoImageRenderer.h"
#include <OpenGL/OpenGL.h>
#include "VuoGlContext.h"
#include "module.h"
#include "node.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSyphonListener",
					 "dependencies" : [
						 "VuoSyphonServerNotifier",
						 "AppKit.framework",
						 "VuoGLContext"
					 ]
				 });
#endif

/**
 * VuoImage @c freeCallback that releases the @c SyphonImage (but doesn't delete the texture).
 */
void VuoSyphonListener_freeSyphonImageCallback(VuoImage image)
{
	SyphonImage *frame = (SyphonImage *)image->freeCallbackContext;
	[frame release];
}

@implementation VuoSyphonListener

@synthesize syphonClient;

/**
 * Creates a Syphon client that is not yet connected to any server.
 */
-(id) init
{
	if (self = [super init])
	{
		callback = NULL;
		serverNotifier = NULL;
		desiredServer = VuoSyphonServerDescription_make(VuoText_make(""), VuoText_make(""), VuoText_make(""));
		VuoSyphonServerDescription_retain(desiredServer);
		refreshQueue = dispatch_queue_create("vuo.syphon.VuoSyphonListener", 0);
	}
	return self;
}

/**
 * Starts listening for frames from the Syphon server (or continues if already connected).
 */
-(void) startListeningWithServerDescription:(VuoSyphonServerDescription)description callback:(void(*)(VuoImage))receivedFrame
{
	dispatch_sync(refreshQueue, ^{

					  VuoSyphonServerDescription_retain(description);
					  VuoSyphonServerDescription_release(desiredServer);
					  desiredServer = description;

					  callback = receivedFrame;

					  VuoList_VuoSyphonServerDescription allServerDescriptions = VuoSyphon_getAvailableServerDescriptions();
					  [self refreshSyphonClientThreadUnsafe:allServerDescriptions];

					  if (!serverNotifier)
					  {
						  serverNotifier = VuoSyphonServerNotifier_make();
						  VuoRetain(serverNotifier);
						  VuoSyphonServerNotifier_setNotificationMethod(serverNotifier, self, @selector(refreshSyphonClient:));
						  VuoSyphonServerNotifier_start(serverNotifier);
					  }
				  });
}

/**
 * Updates the connected server in response to changes to the desired server or available servers.
 *
 * Must be called on @ref refreshQueue.
 */
-(void) refreshSyphonClientThreadUnsafe:(VuoList_VuoSyphonServerDescription)serverDescriptions
{
	if (syphonClient)
	{
		// Already connected to a server — make sure it's still desired and available.

		NSDictionary *currentDict = [syphonClient serverDescription];

		bool isCurrentStillDesired;
		{
			VuoText currentName = VuoText_make([[currentDict objectForKey:SyphonServerDescriptionUUIDKey] UTF8String]);
			VuoText currentAppName = VuoText_make([[currentDict objectForKey:SyphonServerDescriptionUUIDKey] UTF8String]);
			VuoSyphonServerDescription current = VuoSyphonServerDescription_make(VuoText_make(""), currentName, currentAppName);
			VuoList_VuoSyphonServerDescription currentAsList = VuoListCreate_VuoSyphonServerDescription();
			VuoListAppendValue_VuoSyphonServerDescription(currentAsList, current);
			VuoList_VuoSyphonServerDescription currentMatchingDesired = VuoSyphon_filterServerDescriptions(currentAsList, desiredServer);
			isCurrentStillDesired = (VuoListGetCount_VuoSyphonServerDescription(currentMatchingDesired) > 0);
		}

		bool isCurrentStillAvailable;
		{
			VuoText currentUUID = VuoText_make([[currentDict objectForKey:SyphonServerDescriptionUUIDKey] UTF8String]);
			VuoSyphonServerDescription current = VuoSyphonServerDescription_make(currentUUID, VuoText_make(""), VuoText_make(""));
			VuoList_VuoSyphonServerDescription availableMatchingCurrent = VuoSyphon_filterServerDescriptions(serverDescriptions, current);
			isCurrentStillAvailable = (VuoListGetCount_VuoSyphonServerDescription(availableMatchingCurrent) > 0);
		}

		if (isCurrentStillDesired && isCurrentStillAvailable)
		{
			return;
		}
		else
		{
			[syphonClient stop];
			self.syphonClient = nil;
		}
	}

	// Not connected to a server — see if a desired one is available.

	VuoList_VuoSyphonServerDescription matchingServers = VuoSyphon_filterServerDescriptions(serverDescriptions, desiredServer);

	if (VuoListGetCount_VuoSyphonServerDescription(matchingServers) == 0)
	{
		// No desired server is available.
		[syphonClient stop];
		self.syphonClient = nil;
		return;
	}

	VuoSyphonServerDescription chosenServer = VuoListGetValueAtIndex_VuoSyphonServerDescription(matchingServers, 1);

	// Look up the official server description. (SyphonClient won't accept one constructed from chosenServer's fields.)
	NSDictionary *chosenServerDict = nil;
	NSString *chosenUUID = [NSString stringWithUTF8String:chosenServer.serverUUID];
	NSArray *allServerDicts = [[SyphonServerDirectory sharedDirectory] servers];
	for (NSDictionary *serverDict in allServerDicts)
	{
		if ([chosenUUID isEqualToString:[serverDict objectForKey:SyphonServerDescriptionUUIDKey]])
		{
			chosenServerDict = serverDict;
			break;
		}
	}

	// Connect the client to the server.
	SyphonClient *s = [[SyphonClient alloc] initWithServerDescription:chosenServerDict options:nil newFrameHandler:^(SyphonClient *client) {

					CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
					SyphonImage *frame = [client newFrameImageForContext:cgl_ctx];

					if (frame.textureSize.width < 1 || frame.textureSize.height < 1)
						return;

					VuoImage image = VuoImage_makeClientOwnedGlTextureRectangle(frame.textureName, GL_RGBA, frame.textureSize.width, frame.textureSize.height, VuoSyphonListener_freeSyphonImageCallback, frame);
					VuoRetain(image);
					callback(VuoImage_makeCopy(image));
					VuoRelease(image);

					VuoGlContext_disuse(cgl_ctx);
	}];
	self.syphonClient = s;
	[s release];
}

/**
 * Updates the connected server in response to changes to the desired server or available servers.
 */
-(void) refreshSyphonClient:(VuoList_VuoSyphonServerDescription)serverDescriptions
{
	dispatch_sync(refreshQueue, ^{
					  [self refreshSyphonClientThreadUnsafe:serverDescriptions];
				  });
}

/**
 * Disconnects the Syphon client from its server, making it stop receiving frames.
 */
-(void) stopListening
{
	dispatch_sync(refreshQueue, ^{

					  VuoSyphonServerNotifier_stop(serverNotifier);
					  VuoRelease(serverNotifier);
					  serverNotifier = NULL;

					  [syphonClient stop];
					  self.syphonClient = nil;
				  });
}

-(void) dealloc
{
	VuoSyphonServerDescription_release(desiredServer);
	dispatch_release(refreshQueue);
	[super dealloc];
}

@end
