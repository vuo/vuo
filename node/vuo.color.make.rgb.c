/**
 * @file
 * vuo.color.make.rgb node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make RGB Color",
					 "description" :
						"<p>Creates a color from RGBA (red, green, blue, alpha) components.</p> \
						<p>Red, green, blue, and alpha typically range from 0 to 1 (although numbers outside this range may be used for high dynamic range imaging). \
						An alpha value of 0 is completely transparent, and an alpha value of 1 is completely opaque.</p>",
					 "keywords" : [ "red", "green", "blue", "alpha", "opacity", "transparent", "channel" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":1}) red,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":1}) green,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":1}) blue,
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedMax":1}) alpha,
		VuoOutputData(VuoColor) color
)
{
	*color = VuoColor_makeWithRGBA(red, green, blue, alpha);
}
