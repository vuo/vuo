/**
 * @file
 * vuo.color.get.cmyk node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get CMYK Color Value",
					 "keywords" : [ "red", "green", "blue", "alpha", "cyan",
						"magenta", "key", "black", "yellow", "channel", "tone",
						"chroma"
					 ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoReal) cyan,
		VuoOutputData(VuoReal) magenta,
		VuoOutputData(VuoReal) yellow,
		VuoOutputData(VuoReal) black,
		VuoOutputData(VuoReal) alpha
)
{
	VuoColor_getCMYKA(color, cyan, magenta, yellow, black, alpha);
}
