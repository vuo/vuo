/**
 * @file
 * vuo.color.list.dmx node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDmxColorMap.h"

VuoModuleMetadata({
					 "title" : "Convert Color List to DMX",
					 "keywords" : [ "artnet", "art net", "art-net", "fixture", "lamp", "lighting", "rgbaw", "rgbw", "wwcw", "ww/cw" ],
					 "version" : "1.0.0",
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

		// How similar is the color to white (255,255,255)?
		VuoReal whiteness = c.a * (c.r + c.g + c.b) / 3;

		if (colorMap == VuoDmxColorMap_RGB)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b * c.a * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBA)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBAW)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
		}
		else if (colorMap == VuoDmxColorMap_RGBW)
		{
			VuoListAppendValue_VuoInteger(*dmx, c.r * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.g * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, c.b * c.a * 255);
			VuoListAppendValue_VuoInteger(*dmx, whiteness * 255);
		}
		else if (colorMap == VuoDmxColorMap_WWCW)
		{
			VuoListAppendValue_VuoInteger(*dmx,       c.r * whiteness * 255);
			VuoListAppendValue_VuoInteger(*dmx, (1 - c.r) * whiteness * 255);
		}
	}
}
