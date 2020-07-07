/**
 * @file
 * VuoWindowReference implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <AppKit/AppKit.h>

#include "type.h"
#include "VuoWindowReference.h"
#include "VuoGraphicsWindow.h"
#include "VuoGraphicsWindowDrag.h"
#include "VuoScreenCommon.h"
#include "VuoText.h"
#include "VuoApp.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
    "title": "Window",
    "version": "1.0.0",
    "dependencies": [
        "VuoApp",
        "VuoGraphicsWindow",
        "VuoGraphicsWindowDrag",
        "VuoInteger",
        "VuoReal",
        "VuoScreenCommon",
        "VuoText",
        "AppKit.framework",
    ],
});
#endif
/// @}

/**
 * @ingroup VuoWindowReference
 * Creates a VuoWindowReference from a VuoWindow. Since the VuoWindowReference contains the memory address
 * of the VuoWindow, it's only valid as long as the VuoWindow remains in memory.
 */
VuoWindowReference VuoWindowReference_make(void *window)
{
	return (VuoWindowReference)window;
}

/**
 * @ingroup VuoWindowReference
 * Decodes the JSON object @a js, expected to contain a string, to create a new VuoMouseButton.
 */
VuoWindowReference VuoWindowReference_makeFromJson(json_object * js)
{
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "pointer", &o))
		return (VuoWindowReference)json_object_get_int64(o);

	return NULL;
}

/**
 * @ingroup VuoWindowReference
 * Encodes @a value as a JSON object.
 */
json_object * VuoWindowReference_getJson(const VuoWindowReference value)
{
	if (!value)
		return NULL;

	json_object *js = json_object_new_object();
	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value));
	return js;
}

/**
 * @ingroup VuoWindowReference
 * Calls VuoWindowReference_getJson(). Interprocess support is not yet implemented.
 */
json_object * VuoWindowReference_getInterprocessJson(const VuoWindowReference value)
{
	return VuoWindowReference_getJson(value);
}

/**
 * @ingroup VuoWindowReference
 * Returns a brief human-readable summary of @a value.
 */
char * VuoWindowReference_getSummary(const VuoWindowReference value)
{
	if (value == 0)
		return strdup("No window");

	__block const char *title;
	VuoApp_executeOnMainThread(^{
		VuoGraphicsWindow *window = (VuoGraphicsWindow *)value;
		title = window.title.UTF8String;
	});

	return VuoText_format("Title: “%s”", title);
}

/**
 * Returns the window's current aspect ratio.
 */
VuoReal VuoWindowReference_getAspectRatio(const VuoWindowReference value)
{
	VuoInteger width, height;
	float backingScaleFactor;
	VuoWindowReference_getContentSize(value, &width, &height, &backingScaleFactor);
	return (VuoReal)width/(VuoReal)height;
}

/**
 * Returns the position of the top-left of the window's content area, in screen points.
 *
 * @threadAny
 */
VuoPoint2d VuoWindowReference_getPosition(const VuoWindowReference value)
{
	__block VuoPoint2d position;
	VuoApp_executeOnMainThread(^{
		VuoGraphicsWindow *window = (VuoGraphicsWindow *)value;
		NSRect contentRect = [window contentRectForFrameRect:window.frame];
		NSRect mainScreenRect = ((NSScreen *)NSScreen.screens[0]).frame;
		position.x = contentRect.origin.x;
		position.y = mainScreenRect.size.height - contentRect.size.height - contentRect.origin.y;
	});
	return position;
}

/**
 * Returns the window's current content size in pixels.
 *
 * On Retina displays, this function returns the physical number of pixels (device/backing resolution, not logical resolution).
 *
 * @threadAny
 */
void VuoWindowReference_getContentSize(const VuoWindowReference value, VuoInteger *width, VuoInteger *height, float *backingScaleFactor)
{
	VuoApp_executeOnMainThread(^{
		VuoGraphicsWindow *window = (VuoGraphicsWindow *)value;
		NSRect contentRect = [window convertRectToBacking:[window contentRectCached]];
		*width = contentRect.size.width;
		*height = contentRect.size.height;
		*backingScaleFactor = [window backingScaleFactorCached];
	});
}

/**
 * Returns true if the application is currently focused
 * and the window is currently focused (disregarding auxiliary windows, such as the About box).
 */
bool VuoWindowReference_isFocused(const VuoWindowReference value)
{
	NSWindow *window = (NSWindow *)value;
	return [NSApp mainWindow] == window;
}

/**
 * Returns true if the window is currently fullscreen.
 */
bool VuoWindowReference_isFullscreen(const VuoWindowReference value)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)value;
	return window.isFullScreen;
}

/**
 * Returns the screen the window is currently on.
 */
VuoScreen VuoWindowReference_getScreen(const VuoWindowReference value)
{
	VuoGraphicsWindow *window = (VuoGraphicsWindow *)value;
	return VuoScreen_makeFromNSScreen(window.screen);
}

/**
 * Adds callbacks to be invoked when files are dragged from Finder.
 */
void VuoWindowReference_addDragCallbacks(const VuoWindowReference wr,
										 void (*dragEnteredCallback)(VuoDragEvent e),
										 void (*dragMovedToCallback)(VuoDragEvent e),
										 void (*dragCompletedCallback)(VuoDragEvent e),
										 void (*dragExitedCallback)(VuoDragEvent e))
{
	if (!wr)
		return;

	VuoGraphicsWindow *window = (VuoGraphicsWindow *)wr;
	[window addDragEnteredCallback:dragEnteredCallback
			   dragMovedToCallback:dragMovedToCallback
			 dragCompletedCallback:dragCompletedCallback
				dragExitedCallback:dragExitedCallback];
}

/**
 * Removes callbacks that would have been invoked when files were dragged from Finder.
 */
void VuoWindowReference_removeDragCallbacks(const VuoWindowReference wr,
											void (*dragEnteredCallback)(VuoDragEvent e),
											void (*dragMovedToCallback)(VuoDragEvent e),
											void (*dragCompletedCallback)(VuoDragEvent e),
											void (*dragExitedCallback)(VuoDragEvent e))
{
	if (!wr)
		return;

	VuoGraphicsWindow *window = (VuoGraphicsWindow *)wr;
	[window removeDragEnteredCallback:dragEnteredCallback
				  dragMovedToCallback:dragMovedToCallback
				dragCompletedCallback:dragCompletedCallback
				   dragExitedCallback:dragExitedCallback];
}
