/**
 * @file
 * vuo.color.average node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoList_VuoColor.h"

VuoModuleMetadata({
					 "title" : "Average Colors",
					 "keywords" : [ "mix", "blend", "combine", "tint", "tone", "chroma" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "AverageColors.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoColor, {"default":[{"r":1,"g":1,"b":1,"a":1}, {"r":0,"g":0,"b":0,"a":1}]}) colors,
		VuoOutputData(VuoColor) averageColor
)
{
	*averageColor = VuoColor_average(colors);
}
