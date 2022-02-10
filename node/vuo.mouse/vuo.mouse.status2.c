/**
 * @file
 * vuo.mouse.status node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Check Mouse Status",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "button", "press", "get", "current" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.transform/TiltAndOrbitCube.vuo" ]
					  }
				  });

VuoWindowReference nodeInstanceInit(void)
{
	VuoMouseStatus_use();
	return NULL;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoWindowReference) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputEvent({"data":"window","eventBlocking":"wall"}) windowEvent,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputData(VuoPoint2d) position,
		VuoOutputData(VuoBoolean) isPressed
)
{
	VuoIgnoreResult(VuoRenderedLayers_getWindow(window, context));

	if (*context && !VuoWindowReference_isFocused(*context))
		return;

	VuoIgnoreResult(VuoMouse_getStatus(position, isPressed, button, *context, modifierKey, true));
}

void nodeInstanceFini
(
		VuoInstanceData(VuoWindowReference) context
)
{
	VuoMouseStatus_disuse();
}
