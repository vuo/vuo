
/**
 * @file
 * VuoSyphonInternal implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>
#import <Syphon.h>

#import "VuoGlContext.h"

/**
 * This class handles creating a Syphon server and publishing frames from it.
 */
@interface VuoSyphonSender : NSObject
{

}

@property(retain) SyphonServer *syphonServer;  ///< The Syphon server that publishes frames.

- (void) initServerWithName:(NSString *)name;
- (void) publishFrame:(VuoImage)image;
- (void) setName:(NSString*)newName;
- (void) stop;

@end
