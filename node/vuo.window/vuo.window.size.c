/**
 * @file
 * vuo.window.size node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Window Size",
					 "keywords" : [ "width", "height", "dimensions", "properties", "set" ],
					 "version" : "1.0.0",
					 "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":640}) width,
		VuoInputData(VuoInteger, {"default":480}) height,
		VuoInputData(VuoCoordinateUnit, {"default":"points","includeValues":["points","pixels"]}) unit,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_Size;
	(*property).width = width;
	(*property).height = height;
	(*property).unit = unit;
}
