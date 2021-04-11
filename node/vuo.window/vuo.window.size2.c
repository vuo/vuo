/**
 * @file
 * vuo.window.size node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Window Size",
					 "keywords" : [ "width", "height", "dimensions", "properties", "settings" ],
					 "version" : "2.0.1",
					 "node" : {
						  "exampleCompositions" : [ "ShowPrimaryAndSecondaryWindow.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoInteger, {"default":640}) width,
		VuoInputData(VuoInteger, {"default":480}) height,
		VuoInputData(VuoCoordinateUnit, {"default":"points","includeValues":["points","pixels"]}) unit,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_Size;
	property.width = width;
	property.height = height;
	property.unit = unit;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
