/**
 * @file
 * vuo.window.position node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Window Position",
					 "keywords" : [ "top", "right", "bottom", "left", "arrange", "properties", "set" ],
					 "version" : "1.0.0",
					 "node" : {
						  "isDeprecated": true,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) topLeft,
		VuoInputData(VuoCoordinateUnit, {"default":"points","includeValues":["points","pixels"]}) unit,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_Position;
	(*property).left = topLeft.x;
	(*property).top = topLeft.y;
	(*property).unit = unit;
}
