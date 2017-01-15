/**
 * @file
 * VuoWindowReference implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>

#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>

#include "type.h"
#include "VuoWindowReference.h"
#include "VuoWindowOpenGLInternal.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Window",
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoInteger",
						"VuoReal",
						"VuoText",
						"AppKit.framework"
					  ]
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
	return (void *)json_object_get_int64(js);
}

/**
 * @ingroup VuoWindowReference
 * Encodes @a value as a JSON object.
 */
json_object * VuoWindowReference_getJson(const VuoWindowReference value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * @ingroup VuoWindowReference
 * Returns a brief human-readable summary of @a value.
 */
char * VuoWindowReference_getSummary(const VuoWindowReference value)
{
	if (value == 0)
		return strdup("(no window)");

	return VuoText_format("window ID %p", value);
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
 * Returns the window's current content size in pixels.
 *
 * On Retina displays, this function returns the physical number of pixels (device/backing resolution, not logical resolution).
 *
 * @threadNoMain
 */
void VuoWindowReference_getContentSize(const VuoWindowReference value, VuoInteger *width, VuoInteger *height, float *backingScaleFactor)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)value;
	NSRect contentRect = [window convertRectToBacking:[window contentRectCached]];
	*width = contentRect.size.width;
	*height = contentRect.size.height;
	*backingScaleFactor = [window backingScaleFactorCached];
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

	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)wr;
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

	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)wr;
	[window removeDragEnteredCallback:dragEnteredCallback
				  dragMovedToCallback:dragMovedToCallback
				dragCompletedCallback:dragCompletedCallback
				   dragExitedCallback:dragExitedCallback];
}
