/**
 * @file
 * VuoWindow implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindow.h"
#import "VuoWindowTextInternal.h"
#import "VuoGraphicsWindow.h"
#include "VuoEventLoop.h"
#include "VuoCompositionState.h"
#include "VuoApp.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindow",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoGraphicsWindow",
						 "VuoWindowTextInternal"
					 ]
				 });
#endif

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
	void *compositionState = vuoCopyCompositionStateFromThreadLocalStorage();

	__block VuoWindowTextInternal *window = NULL;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  vuoAddCompositionStateToThreadLocalStorage(compositionState);

					  VuoApp_init();
					  window = [[VuoWindowTextInternal alloc] init];
					  [window makeKeyAndOrderFront:NSApp];

					  vuoRemoveCompositionStateFromThreadLocalStorage();
					  free(compositionState);
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
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
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
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
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
	if (!text)
		return;

	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	char *textCopy = strdup(text);  // ... in case the caller frees text before the asynchronous block uses it.
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
					   [window appendLine:textCopy];
					   free(textCopy);
				   });
}

/**
 * Closes the window.
 *
 * @threadNoMain
 */
void VuoWindowText_close(VuoWindowText vw)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window close];
				  });
}

/**
 * Deallocates the window.
 *
 * @threadNoMain
 */
void VuoWindowText_destroy(VuoWindowText vw)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window release];
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
		void (*initCallback)(void *, float backingScaleFactor),
		void (*updateBackingCallback)(void *, float backingScaleFactor),
		void (*resizeCallback)(void *, unsigned int, unsigned int),
		VuoIoSurface (*drawCallback)(void *),
		void *context
)
{
	__block VuoGraphicsWindow *window = NULL;
	void *compositionState = vuoCopyCompositionStateFromThreadLocalStorage();

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  vuoAddCompositionStateToThreadLocalStorage(compositionState);

					  VuoApp_init();
					  window = [[VuoGraphicsWindow alloc] initWithInitCallback:initCallback
														 updateBackingCallback:updateBackingCallback
																resizeCallback:resizeCallback
																  drawCallback:drawCallback
																	  userData:context];
					  [window makeKeyAndOrderFront:nil];
					  window.compositionUid = vuoGetCompositionUniqueIdentifier(compositionState);

					  vuoRemoveCompositionStateFromThreadLocalStorage();
					  free(compositionState);
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
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal)
)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	[window enableShowedWindowTrigger:showedWindow requestedFrameTrigger:requestedFrame];
}

/**
 * Stops the window from calling trigger functions when events occur.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	[window disableTriggers];
}

/**
 * Schedules the specified window to be redrawn.
 *
 * @threadAny
 */
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	[window scheduleRedraw];
}

/**
 * Applies a list of properties to a window.
 *
 * @threadAny
 */
void VuoWindowOpenGl_setProperties(VuoWindowOpenGl w, VuoList_VuoWindowProperty properties)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window setProperties:properties];
				  });
}

/**
 * Sets the window's current and preferred aspect ratio.
 *
 * If necessary, the window is resized to match the specified aspect ratio.
 * After calling this method, when the user resizes the window, its height is adjusted to match the specified aspect ratio.
 */
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
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
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
					   [window unlockAspectRatio];
				   });
}

/**
 * Closes the window.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_close(VuoWindowOpenGl vw)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window close];
				  });
}

/**
 * Deallocates a @c VuoWindowOpenGl window.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_destroy(VuoWindowOpenGl vw)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window release];

					  // -[NSWindow release] apparently doesn't actually dealloc the window; it adds the dealloc to the NSRunLoop.
					  // Make sure the dealloc actually happens now.
					  VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
				  });
}
