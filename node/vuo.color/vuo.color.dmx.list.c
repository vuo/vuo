/**
 * @file
 * vuo.color.dmx.list node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoColorspace.h"
#include "VuoDmxColorMap.h"

VuoModuleMetadata({
	"title": "Convert DMX to Color List",
	"keywords": [ "artnet", "art net", "art-net", "fixture", "lamp", "lighting", "rgbaw", "rgbw", "wwcw", "ww/cw" ],
	"version": "1.0.1",
	"node": {
		"exampleCompositions": [ ]
	},
	"dependencies" : [
		"VuoColorspace",
	],
});

void nodeEvent
(
	VuoInputData(VuoList_VuoInteger, {"name":"DMX"}) dmx,
	VuoInputData(VuoDmxColorMap, {"default":"rgb"}) colorMap,
	VuoOutputData(VuoList_VuoColor) colors
)
{
	VuoInteger channelCount = VuoListGetCount_VuoInteger(dmx);
	*colors = VuoListCreate_VuoColor();

	if (colorMap == VuoDmxColorMap_RGB)
		for (VuoInteger i = 1; i <= (channelCount/3)*3; i += 3)
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithRGBA(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  )) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1)) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2)) / 255., 0, 1),
											1.));
	else if (colorMap == VuoDmxColorMap_RGBA)
		for (VuoInteger i = 1; i <= (channelCount/4)*4; i += 4)
		{
			VuoInteger amber = VuoListGetValue_VuoInteger(dmx, i+3);
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithRGBA(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  ) + amber  ) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1) + amber  ) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2)          ) / 255., 0, 1),
											1.));
		}
	else if (colorMap == VuoDmxColorMap_RGBAW)
		for (VuoInteger i = 1; i <= (channelCount/5)*5; i += 5)
		{
			VuoInteger amber = VuoListGetValue_VuoInteger(dmx, i+3);
			VuoInteger white = VuoListGetValue_VuoInteger(dmx, i+4);
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithRGBA(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  ) + white + amber  ) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1) + white + amber  ) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2) + white          ) / 255., 0, 1),
											1.));
		}
	else if (colorMap == VuoDmxColorMap_RGBW)
		for (VuoInteger i = 1; i <= (channelCount/4)*4; i += 4)
		{
			VuoInteger white = VuoListGetValue_VuoInteger(dmx, i+3);
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithRGBA(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  ) + white) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1) + white) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2) + white) / 255., 0, 1),
											1.));
		}
	else if (colorMap == VuoDmxColorMap_WWCW)
		for (VuoInteger i = 1; i <= (channelCount/2)*2; i += 2)
		{
			VuoReal warmWhite = VuoListGetValue_VuoInteger(dmx, i);		// treating as 5600 K
			VuoReal coolWhite = VuoListGetValue_VuoInteger(dmx, i+1);	// treating as 7600 K
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithRGBA(
											VuoReal_clamp(warmWhite*1.00/255. + coolWhite*0.88/255., 0, 2)/1.88,
											VuoReal_clamp(warmWhite*0.94/255. + coolWhite*0.94/255., 0, 2)/1.88,
											VuoReal_clamp(warmWhite*0.88/255. + coolWhite*1.00/255., 0, 2)/1.88,
											1.));
		}
	else if (colorMap == VuoDmxColorMap_CMY)
		for (VuoInteger i = 1; i <= (channelCount/3)*3; i += 3)
			VuoListAppendValue_VuoColor(*colors, VuoColorspace_makeCMYKAColor(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  )) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1)) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2)) / 255., 0, 1),
											0, 1, 0));
	else if (colorMap == VuoDmxColorMap_HSL)
		for (VuoInteger i = 1; i <= (channelCount/3)*3; i += 3)
			VuoListAppendValue_VuoColor(*colors, VuoColor_makeWithHSLA(
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i  )) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+1)) / 255., 0, 1),
											VuoReal_clamp(((double)VuoListGetValue_VuoInteger(dmx, i+2)) / 255., 0, 1),
											1));
}
