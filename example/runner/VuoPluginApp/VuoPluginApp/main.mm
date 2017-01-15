/**
 * @file
 * main.m
 * VuoPluginApp
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import <Cocoa/Cocoa.h>

NSOpenGLContext *VuoPluginAppSharedOpenGLContext = nil;

#include <Vuo/Vuo.h>

/**
 * Creates an OpenGL graphics context to be shared between Vuo and this host app.
 */
void VuoPluginApp_createOpenGLContext(void)
{
    // Create a context with the pixelformat Vuo uses.
	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAWindow,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFADepthSize, 16,
		// Multisampling breaks point rendering on some GPUs.  https://b33p.net/kosada/node/8225#comment-31324
//		NSOpenGLPFAMultisample,
//		NSOpenGLPFASampleBuffers, 1,
//		NSOpenGLPFASamples, 4,
		0
	};
    
	NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	VuoPluginAppSharedOpenGLContext = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
    
	CGLContextObj cgl_ctx = (CGLContextObj)[VuoPluginAppSharedOpenGLContext CGLContextObj];
    
	// Share the GL Context with Vuo
	VuoGlContext_setGlobalRootContext((void *)cgl_ctx);
}

int main(int argc, const char * argv[])
{
	VuoPluginApp_createOpenGLContext();
    return NSApplicationMain(argc, argv);
}
