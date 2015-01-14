/**
 * @file
 * vuo.mouse.get.action node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Mouse Button Action Values",
					 "keywords" : [ "trackpad", "touchpad", "wheel", "scroll", "click", "tap", "cursor", "pointer" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoMouseButtonAction) action,
		VuoOutputData(VuoBoolean) isLeftButton,
		VuoOutputData(VuoBoolean) isMiddleButton,
		VuoOutputData(VuoBoolean) isRightButton,
		VuoOutputData(VuoBoolean) isPress,
		VuoOutputData(VuoBoolean) isRelease,
		VuoOutputData(VuoBoolean) isSingleClick,
		VuoOutputData(VuoBoolean) isDoubleClick,
		VuoOutputData(VuoBoolean) isTripleClick,
		VuoOutputData(VuoPoint2d) position
)
{
	*isLeftButton = (action.button == VuoMouseButton_Left);
	*isMiddleButton = (action.button == VuoMouseButton_Middle);
	*isRightButton = (action.button == VuoMouseButton_Right);
	*isPress = (action.type == VuoMouseButtonActionType_Press);
	*isRelease = (action.type == VuoMouseButtonActionType_Release);
	*isSingleClick = (action.type == VuoMouseButtonActionType_SingleClick);
	*isDoubleClick = (action.type == VuoMouseButtonActionType_DoubleClick);
	*isTripleClick = (action.type == VuoMouseButtonActionType_TripleClick);
	*position = action.position;
}
