/**
 * @file
 * VuoSyphonListener implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>
#import <Syphon.h>

#include "VuoSyphonServerNotifier.h"
#include "VuoImage.h"

/**
 * This class handles connecting to and receiving frames from a Syphon server.
 */
@interface VuoSyphonListener : NSObject
{
	void (*callback)(VuoImage);  ///< Called when a frame is received.
	VuoSyphonServerNotifier *serverNotifier;  ///< Used to track changes to the available Syphon servers.
	VuoSyphonServerDescription desiredServer;  ///< A description of the Syphon server to connect to.
	dispatch_queue_t refreshQueue;  ///< Synchronizes changes to member variables.
}

@property(retain) SyphonClient *syphonClient;  ///< The Syphon client that receives frames.

-(id) init;
-(void) startListeningWithServerDescription:(VuoSyphonServerDescription)description callback:(void(*)(VuoImage))receivedFrame;
-(void) stopListening;
-(void) refreshSyphonClientThreadUnsafe:(VuoList_VuoSyphonServerDescription)serverDescriptions;
-(void) refreshSyphonClient:(VuoList_VuoSyphonServerDescription)serverDescriptions;

@end
