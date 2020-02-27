/**
 * @file
 * vuo.window.position node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Window Position",
					 "keywords" : [ "top", "right", "bottom", "left", "arrange", "properties", "settings" ],
					 "version" : "2.0.0",
					 "node" : {
						  "exampleCompositions" : [ "ShowPrimaryAndSecondaryWindow.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) topLeft,
		VuoInputData(VuoCoordinateUnit, {"default":"points"}) unit,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_Position;
	property.left = topLeft.x;
	property.top = topLeft.y;
	property.unit = unit;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
