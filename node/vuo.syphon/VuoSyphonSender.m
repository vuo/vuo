/**
 * @file
 * VuoSyphonSender implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoSyphonSender.h"
#include "VuoImageRenderer.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include "VuoGlContext.h"
#include "module.h"
#include "node.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSyphonSender",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoGlContext"
					 ]
				 });
#endif

@implementation VuoSyphonSender

@synthesize syphonServer;

/**
 * Creates the Syphon server.
 *
 * @param name The server name.
 */
- (void) initServerWithName:(NSString*)name
{
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			syphonServer = [[SyphonServer alloc] initWithName:name context:cgl_ctx options:nil];
		});
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

		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			VuoShader_resetContext(cgl_ctx);

			// Syphon draws its triangles backwards, so we have to temporarily turn off backface culling.
			glDisable(GL_CULL_FACE);

			[syphonServer publishFrameTexture:image->glTextureName
						textureTarget:GL_TEXTURE_2D
						imageRegion:NSMakeRect(0, 0, image->pixelsWide, image->pixelsHigh)
						textureDimensions:NSMakeSize(image->pixelsWide, image->pixelsHigh)
						flipped:NO];

			glEnable(GL_CULL_FACE);
		});
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
