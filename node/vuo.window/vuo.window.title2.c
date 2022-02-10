/**
 * @file
 * vuo.window.title node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Window Title",
					 "keywords" : [ "label", "name", "properties", "settings" ],
					 "version" : "2.0.0",
					 "node" : {
						  "exampleCompositions" : [ "ShowPrimaryAndSecondaryWindow.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoText) title,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_Title;
	property.title = title;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
