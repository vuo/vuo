/**
 * @file
 * VuoMouseUtility implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif

#include "module.h"
#include <AppKit/AppKit.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoMouseUtility",
					"dependencies" : [
						"AppKit.framework"
					]
				 });
#endif

/**
 * Get the time-frame within which a second mouse press must occur in order to be considered a double click.
 *
 * @note This is separate from node/vuo.mouse/VuoMouse due to a long dependency chain in VuoMouse.
 */
VuoReal VuoMouse_getDoubleClickInterval(void)
{
	NSTimeInterval interval = [NSEvent doubleClickInterval];
	return (VuoReal) interval;
}
