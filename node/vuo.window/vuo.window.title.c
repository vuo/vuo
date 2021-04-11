/**
 * @file
 * vuo.window.title node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Window Title",
					 "keywords" : [ "label", "name", "properties", "set" ],
					 "version" : "1.0.0",
					 "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText) title,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_Title;
	(*property).title = title;
}
