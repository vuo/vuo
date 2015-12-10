/**
 * @file
 * VuoWindow implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindow.h"
#import "VuoWindowApplication.h"
#import "VuoWindowTextInternal.h"
#import "VuoWindowOpenGLInternal.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindow",
					 "dependencies" : [
						 "VuoWindowApplication",
						 "VuoWindowTextInternal",
						 "VuoWindowOpenGLInternal"
					 ]
				 });
#endif


/**
 * Creates a VuoWindowApplication instance (if one doesn't already exist).
 *
 * @threadMain
 */
void VuoWindowApplication_init(void)
{
	[NSAutoreleasePool new];
	if (! NSApp)
	{
		// http://stackoverflow.com/a/11010614/238387
		[VuoWindowApplication sharedApplication];
		[NSApp activateIgnoringOtherApps:YES];
		[NSApp run];
	}
}

/**
 * Called at startup. Creates a VuoWindowApplication instance for any composition that
 * uses this library.
 */
static void __attribute__((constructor)) launchApplication()
{
	// Wait a bit before calling `VuoWindowApplication_init()`, since -[NSApplication init] fails with
	// `RegisterApplication(), FAILED TO REGISTER PROCESS WITH CPS/CoreGraphics in WindowServer, err=1003`
	// apparently if it happens too early.
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.25*NSEC_PER_SEC), dispatch_get_main_queue(), ^{
					  VuoWindowApplication_init();
				  });
}

void VuoWindowText_destroy(VuoWindowText w);

/**
 * Creates and displays a window containing a text edit widget.
 *
 * Creates a new NSApplication instance if one did not already exist.
 *
 * @threadNoMain
 */
VuoWindowText VuoWindowText_make(void)
{
	__block VuoWindowTextInternal *window = NULL;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VuoWindowApplication_init();
					  window = [[VuoWindowTextInternal alloc] init];
					  [window makeKeyAndOrderFront:NSApp];
				   });
	VuoRegister(window, VuoWindowText_destroy);
	return (VuoWindowText *)window;
}

/**
 * Sets up the window to call the trigger functions when its text is edited.
 *
 * @threadNoMain
 */
void VuoWindowText_enableTriggers
(
		VuoWindowText w,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)w;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window enableTriggersWithTypedLine:typedLine typedWord:typedWord typedCharacter:typedCharacter];
				  });
}

/**
 * Stops the window from calling trigger functions when its text is edited.
 *
 * @threadNoMain
 */
void VuoWindowText_disableTriggers(VuoWindowText w)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)w;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window disableTriggers];
				  });
}

/**
 * Appends text and a linebreak to the text edit widget in the window.
 *
 * @threadAny
 */
void VuoWindowText_appendLine(VuoWindowText vw, const char *text)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	char *textCopy = strdup(text);  // ... in case the caller frees text before the asynchronous block uses it.
	dispatch_async(dispatch_get_main_queue(), ^{
					   [window appendLine:textCopy];
					   free(textCopy);
				   });
}

/**
 * Closes and deallocates a @c VuoWindowText window.
 *
 * Destroys the current NSApplication instance if it no longer contains any widgets.
 *
 * @threadNoMain
 */
void VuoWindowText_destroy(VuoWindowText vw)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window close];
//					  [window release];
				  });
}


void VuoWindowOpenGl_destroy(VuoWindowOpenGl w);

/**
 * Creates and displays a window containing an OpenGL view.
 *
 * Creates a new NSApplication instance if one did not already exist.
 *
 * @threadNoMain
 */
VuoWindowOpenGl VuoWindowOpenGl_make
(
		bool useDepthBuffer,
		void (*initCallback)(VuoGlContext glContext, float backingScaleFactor, void *),
		void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int, unsigned int),
		void (*drawCallback)(VuoGlContext glContext, void *),
		void *context
)
{
	__block VuoWindowOpenGLInternal *window = NULL;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VuoWindowApplication_init();
					  window = [[VuoWindowOpenGLInternal alloc] initWithDepthBuffer:useDepthBuffer
						  initCallback:initCallback
						  resizeCallback:resizeCallback
						  drawCallback:drawCallback
						  drawContext:context];
					  [window makeKeyAndOrderFront:nil];
				  });
	VuoRegister(window, VuoWindowOpenGl_destroy);
	return (VuoWindowOpenGl *)window;
}

/**
 * Sets up the window to call the trigger functions when events occur.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_enableTriggers
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(showedWindow, VuoWindowReference)
)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window enableTriggers:showedWindow];
				  });
}

/**
 * Stops the window from calling trigger functions when events occur.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window disableTriggers];
				  });
}

/**
 * Schedules the specified window to be redrawn.
 *
 * @threadAny
 */
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	[window scheduleRedraw];
}

/**
 * Applies a list of properties to a window.
 *
 * @threadAny
 */
void VuoWindowOpenGl_setProperties(VuoWindowOpenGl w, VuoList_VuoWindowProperty properties)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window setProperties:properties];
				  });
}

/**
 * Executes the specifed block on the window's OpenGL context, then returns.
 * Ensures that nobody else is using the OpenGL context at that time
 * (by synchronizing with the window's @c drawQueue).
 *
 * @threadAny
 */
void VuoWindowOpenGl_executeWithWindowContext(VuoWindowOpenGl w, void (^blockToExecute)(VuoGlContext glContext))
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	[window executeWithWindowContext:blockToExecute];
}

/**
 * Sets the window's current and preferred aspect ratio.
 *
 * If necessary, the window is resized to match the specified aspect ratio.
 * After calling this method, when the user resizes the window, its height is adjusted to match the specified aspect ratio.
 */
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	dispatch_async(dispatch_get_main_queue(), ^{
					   [window setAspectRatioToWidth:pixelsWide height:pixelsHigh];
				   });
}

/**
 * Removes the aspect ratio constraint set by @ref VuoWindowOpenGl_setAspectRatio.
 *
 * After calling this method, the user will be able to resizes the window freely.
 */
void VuoWindowOpenGl_unlockAspectRatio(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	dispatch_async(dispatch_get_main_queue(), ^{
					   [window unlockAspectRatio];
				   });
}

/**
 * Closes and deallocates a @c VuoWindowOpenGl window.
 *
 * Destroys the current NSApplication instance if it no longer contains any widgets.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_destroy(VuoWindowOpenGl vw)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)vw;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  [window close];

					  // Don't release the window after closing it, since isReleasedWhenClosed defaults to YES.
//					  [window release];
				  });
}
