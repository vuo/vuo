/**
 * @file
 * vuo.color.make.hsl node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make HSL Color",
					 "description" :
						"<p>Creates a color from HSLA (hue, saturation, lightness, alpha) values.</p> \
						<p>Hue, saturation, lightness, and alpha all range from 0 to 1. \
						An alpha value of 0 is completely transparent, and an alpha value of 1 is completely opaque.</p>",
					 "keywords" : [ "hue", "saturation", "lightness", "alpha", "opacity", "transparent", "channel" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) hue,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) saturation,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1}) lightness,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) alpha,
		VuoOutputData(VuoColor) color
)
{
	*color = VuoColor_makeWithHSLA(hue, saturation, lightness, alpha);
}
