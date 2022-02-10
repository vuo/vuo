/**
 * @file
 * vuo.window.cursor node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowDescription.h"

VuoModuleMetadata({
					 "title" : "Change Mouse Cursor",
					 "keywords" : [ "pointer", "hand", "arrow", "hide", "touch", "properties", "settings" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "DragWithHandCursor.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowDescription) windowDescription,
		VuoInputData(VuoCursor, {"default":"none"}) cursor,
		VuoOutputData(VuoWindowDescription) changedWindowDescription
)
{
	VuoWindowProperty property = VuoWindowProperty_makeFromJson(NULL);
	property.type = VuoWindowProperty_Cursor;
	property.cursor = cursor;
	*changedWindowDescription = VuoWindowDescription_copy(windowDescription);
	VuoWindowDescription_setProperty(*changedWindowDescription, property);
}
