/**
 * @file
 * vuo.color.list.dmx node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDmxColorMap.h"

VuoModuleMetadata({
					 "title" : "Convert Color List to DMX",
					 "keywords" : [ "artnet", "art net", "art-net", "fixture", "lamp", "lighting", "rgbaw", "rgbw", "wwcw", "ww/cw", "cmy", "hsl" ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoColor) colors,
	VuoInputData(VuoDmxColorMap, {"default":"rgb"}) colorMap,
	VuoOutputData(VuoList_VuoInteger, {"name":"DMX"}) dmx
)
{
	VuoInteger colorCount = VuoListGetCount_VuoColor(colors);
	*dmx = VuoListCreate_VuoInteger();
	for (VuoInteger i = 1; i <= colorCount; ++i)
	{
		VuoColor c = VuoListGetValue_VuoColor(colors, i);
		c = VuoColor_premultiply(c);

		VuoReal h, s, l, a;
		VuoColor_getHSLA(c, &h, &s, &l, &a);

		// How similar is the color to white?
		// As the HSL 'l' value increases from 0.5 to 1,
		// the color becomes whiter (regardless of hue and saturation).
		VuoReal whiteness = VuoReal_clamp((l - 0.5) * 2, 0, 1);

		// How similar is the color to amber (HSL 1/6, 1, 0.5) ?
		// As the HSL 'l' value increases from 0.5 to 1, the color becomes whiter (regardless of hue and saturation).
		VuoReal hslDistanceFromAmber = VuoPoint3d_distance((VuoPoint3d){1./6., 1, 0.5}, (VuoPoint3d){h, s, l});
		VuoReal amberness = VuoReal_clamp(pow(MAX(0, 1 - hslDistanceFromAmber), 4), 0, 1);

		// Warmth: How far is the color from cyan (hue 0.5) ?
		VuoReal warmth = fabs(0.5 - h) * 2;
		// Skew desaturated colors toward the center.
		warmth = VuoReal_lerp(0.5, warmth, s * (1. - fabs(0.5 - l)*2));

//		VLog("RGB=%g %g %g  HSL=%g %g %g  wh=%g  am=%g  warm=%g", c.r,c.g,c.b, h,s,l, whiteness, amberness, warmth);

		if (colorMap == VuoDmxColorMap_RGB)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBA)	// A = Amber
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b       * 255);
			VuoListAppendValue_VuoInteger(*dmx, amberness * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBAW)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b       * 255);
			VuoListAppendValue_VuoInteger(*dmx, amberness * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBW)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g       * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b       * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
		}
		else if (colorMap == VuoDmxColorMap_WWCW)
		{
			VuoListAppendValue_VuoInteger(*dmx, VuoReal_clamp(     warmth  * 2 * l * 255, 0, 255));
			VuoListAppendValue_VuoInteger(*dmx, VuoReal_clamp((1 - warmth) * 2 * l * 255, 0, 255));
		}
		else if (colorMap == VuoDmxColorMap_CMY)
		{
			VuoListAppendValue_VuoInteger(*dmx, (1. - c.r) * 255);
			VuoListAppendValue_VuoInteger(*dmx, (1. - c.g) * 255);
			VuoListAppendValue_VuoInteger(*dmx, (1. - c.b) * 255);
		}
		else if (colorMap == VuoDmxColorMap_HSL)
		{
			VuoListAppendValue_VuoInteger(*dmx, h * 255);
			VuoListAppendValue_VuoInteger(*dmx, s * 255);
			VuoListAppendValue_VuoInteger(*dmx, l * 255);
		}
	}
}
