/**
 * @file
 * vuo.color.get.rgb node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get RGB Color Values",
					 "keywords" : [ "alpha", "transparent", "channel", "tone", "chroma" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoReal) red,
		VuoOutputData(VuoReal) green,
		VuoOutputData(VuoReal) blue,
		VuoOutputData(VuoReal) opacity
)
{
	VuoColor_getRGBA(color, red, green, blue, opacity);
}
