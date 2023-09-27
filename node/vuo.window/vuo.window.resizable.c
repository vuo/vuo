/**
 * @file
 * vuo.window.resizable node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Resizable Status",
					 "keywords" : [ "resize", "scale", "stretch", "fill", "tile", "shrink", "blow up", "enlarge", "magnify", "lock", "fixed", "size", "properties", "set" ],
					 "version" : "1.0.0",
					 "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) resizable,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_Resizable;
	(*property).resizable = resizable;
}
