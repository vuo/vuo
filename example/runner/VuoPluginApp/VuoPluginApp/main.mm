/**
 * @file
 * main.m
 * VuoPluginApp
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
// Metal reimplementation coming soon.

NSOpenGLContext *VuoPluginAppSharedOpenGLContext = nil;

/**
 * Creates an OpenGL graphics context to be shared between Vuo and this host app.
 */
void VuoPluginApp_createOpenGLContext(void)
{
    // Create a context with the pixelformat Vuo uses.
	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
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
#pragma clang diagnostic pop

int main(int argc, const char * argv[])
{
	VuoPluginApp_createOpenGLContext();

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		[VuoRunnerCocoa prepareForFastBuild];

		NSURL *defaultComposition = [NSBundle.mainBundle URLForResource:@"VuoPlugin" withExtension:@"vuo"];
		[NSDocumentController.sharedDocumentController openDocumentWithContentsOfURL:defaultComposition display:YES completionHandler:^(NSDocument *, BOOL, NSError *){}];
	});

	return NSApplicationMain(argc, argv);
}
