/**
 * @file
 * vuo.mouse.filter.action node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Filter Mouse Button Action",
					 "description" :
						"<p>Only lets a mouse press, release, or click pass through if it matches all conditions.</p> \
						<p><ul> \
						<li>`action` — An event into this port is blocked unless the port's value meets all of the criteria \
						specified by the other input ports.</li> \
						<li>`includeLeftButton`, `includeMiddleButton`, `includeRightButton` – If <i>true</i>, accepts actions \
						that used the left, middle, or right mouse button. At least one of these ports must be <i>true</i> \
						for this node to accept any mouse button actions.</li> \
						<li>`includePress`, `includeRelease`, `includeSingleClick`, `includeDoubleClick`, `includeTripleClick` — \
						If <i>true</i>, accepts presses, releases, single-clicks, double-clicks, or triple-clicks. \
						At least one of these ports must be <i>true</i> for this node to accept any mouse button actions.</li> \
						</ul></p>",
					 "keywords" : [ "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
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
