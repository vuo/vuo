/**
 * @file
 * vuo.color.get.rgb node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get RGB Color Values",
					 "keywords" : [ "red", "green", "blue", "alpha", "opacity", "transparent", "channel" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoReal) red,
		VuoOutputData(VuoReal) green,
		VuoOutputData(VuoReal) blue,
		VuoOutputData(VuoReal) alpha
)
{
	VuoColor_getRGBA(color, red, green, blue, alpha);
}
