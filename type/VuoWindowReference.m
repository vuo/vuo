/**
 * @file
 * VuoWindowReference implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	__block NSRect contentRect;
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)value;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  contentRect = [[window glView] viewport];

					  if ([window respondsToSelector:@selector(convertRectToBacking:)])
					  {
						  typedef double (*backingFuncType)(id receiver, SEL selector);
						  backingFuncType backingFunc = (backingFuncType)[[window class] instanceMethodForSelector:@selector(backingScaleFactor)];
						  *backingScaleFactor = backingFunc(window, @selector(backingScaleFactor));

						  typedef NSRect (*funcType)(id receiver, SEL selector, NSRect);
						  funcType convertRectToBacking = (funcType)[[window class] instanceMethodForSelector:@selector(convertRectToBacking:)];
						  contentRect = convertRectToBacking(window, @selector(convertRectToBacking:), contentRect);
					  }
				  });
	*width = contentRect.size.width;
	*height = contentRect.size.height;
}

/**
 * Returns true if the application is currently focused
 * and the window is currently focused (disregarding auxiliary windows, such as the About box).
 */
bool VuoWindowReference_isFocused(const VuoWindowReference value)
{
	__block bool focused;
	NSWindow *window = (NSWindow *)value;
	dispatch_sync(dispatch_get_main_queue(), ^{
					  focused = [window isMainWindow];
				  });
	return focused;
}
