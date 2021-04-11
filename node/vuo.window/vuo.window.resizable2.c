/**
 * @file
 * vuo.window.resizable node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Resizable Status",
					 "keywords" : [ "resize", "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify", "lock", "fixed", "size", "properties", "settings" ],
					 "version" : "2.0.0",
					 "node" : {
						  "exampleCompositions" : [ "ShowPrimaryAndSecondaryWindow.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoBoolean, {"default":false}) resizable,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_Resizable;
	property.resizable = resizable;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
