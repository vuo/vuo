/**
 * @file
 * VuoSyphonSender implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoSyphonSender.h"
#include "VuoImageRenderer.h"
#include <OpenGL/OpenGL.h>
#include "VuoGlContext.h"
#include "module.h"
#include "node.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSyphonSender",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoGLContext"
					 ]
				 });
#endif

@implementation VuoSyphonSender

@synthesize syphonServer;

/**
 * Creates the Syphon server.
 *
 * @param name The server name.
 * @param ctx The GL context to use when publishing frames.
 */
- (void) initServerWithName:(NSString*)name context:(VuoGlContext*)ctx
{
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  syphonServer = [[SyphonServer alloc] initWithName:name context:(CGLContextObj)ctx options:nil];
				  });
}

/**
 * Publishes a frame from the Syphon server. The frame is sent to all connected Syphon clients.
 */
- (void) publishFrame:(VuoImage)image
{
	if ([syphonServer hasClients] && image)
	{
		if (image->pixelsWide < 1 || image->pixelsHigh < 1)
			return;

		CGLLockContext(syphonServer.context);

			[syphonServer publishFrameTexture:image->glTextureName
						textureTarget:GL_TEXTURE_2D
						imageRegion:NSMakeRect(0, 0, image->pixelsWide, image->pixelsHigh)
						textureDimensions:NSMakeSize(image->pixelsWide, image->pixelsHigh)
						flipped:NO];

		CGLUnlockContext(syphonServer.context);
	}
}

/**
 * Changes the Syphon server's name.
 */
- (void) setName:(NSString*)newName
{
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  syphonServer.name = newName;
				  });
}

/**
 * Makes the Syphon server unavailable.
 */
- (void) stop
{
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [syphonServer stop];
					  [syphonServer release];
				  });
}

@end
