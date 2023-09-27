/**
 * @file
 * vuo.mouse.status node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Check Mouse Status",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "button", "press", "get", "current" ],
					  "version" : "1.0.3",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "vuo-example://vuo.audio/ControlLoudness.vuo" ]
					  }
				  });

void *nodeInstanceInit(void)
{
	VuoMouseStatus_use();
	return NULL;
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputData(VuoPoint2d) position,
		VuoOutputData(VuoBoolean) isPressed
)
{
	if (window && !VuoWindowReference_isFocused(window))
		return;

	VuoIgnoreResult(VuoMouse_getStatus(position, isPressed, button, window, modifierKey, true));
}

void nodeInstanceFini
(
		VuoInstanceData(void *) context
)
{
	VuoMouseStatus_disuse();
}
