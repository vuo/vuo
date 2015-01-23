/**
 * @file
 * vuo.mouse.filter.action node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Filter Mouse Button Action",
					 "keywords" : [ "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "ShowMouseClicks.vuo" ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMouseButtonAction) action,
		VuoInputEvent(VuoPortEventBlocking_Door, action) actionEvent,

		VuoInputData(VuoBoolean, {"default":true}) includeLeftButton,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeLeftButton) includeLeftButtonEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeMiddleButton,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeMiddleButton) includeMiddleButtonEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeRightButton,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeRightButton) includeRightButtonEvent,

		VuoInputData(VuoBoolean, {"default":true}) includePress,
		VuoInputEvent(VuoPortEventBlocking_Wall, includePress) includePressEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeRelease,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeRelease) includeReleaseEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeSingleClick,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeSingleClick) includeSingleClickEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeDoubleClick,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeDoubleClick) includeDoubleClickEvent,

		VuoInputData(VuoBoolean, {"default":false}) includeTripleClick,
		VuoInputEvent(VuoPortEventBlocking_Wall, includeTripleClick) includeTripleClickEvent,

		VuoOutputData(VuoMouseButtonAction) filteredAction,
		VuoOutputEvent(filteredAction) filteredActionEvent
)
{
	if (action.button == VuoMouseButton_Left && ! includeLeftButton)
		return;

	if (action.button == VuoMouseButton_Middle && ! includeMiddleButton)
		return;

	if (action.button == VuoMouseButton_Right && ! includeRightButton)
		return;

	if (action.type == VuoMouseButtonActionType_Press && ! includePress)
		return;

	if (action.type == VuoMouseButtonActionType_Release && ! includeRelease)
		return;

	if (action.type == VuoMouseButtonActionType_SingleClick && ! includeSingleClick)
		return;

	if (action.type == VuoMouseButtonActionType_DoubleClick && ! includeDoubleClick)
		return;

	if (action.type == VuoMouseButtonActionType_TripleClick && ! includeTripleClick)
		return;

	*filteredAction = action;
	*filteredActionEvent = true;
}
