/**
 * @file
 * vuo.mouse.status node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Check Mouse Status",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "button", "press", "get", "current" ],
					  "version" : "1.0.2",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputData(VuoPoint2d) position,
		VuoOutputData(VuoBoolean) isPressed
)
{
	if (window && !VuoWindowReference_isFocused(window))
		return;

	VuoMouse_getStatus(position, isPressed, button, window, modifierKey);
}
