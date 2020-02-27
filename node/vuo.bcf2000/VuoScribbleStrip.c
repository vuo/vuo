/**
 * @file
 * VuoScribbleStrip implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoScribbleStrip.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoScribbleStrip",
					"dependencies" : [
					]
				 });
#endif

#define VuoScribbleStrip_channels 8                            ///< Number of channel strips.
#define VuoScribbleStrip_sections VuoScribbleStrip_Faders + 1  ///< Number of sections in each channel strip.
#define VuoScribbleStrip_maxTextLength 25                      ///< Number of ASCII-7 characters allowed for each section, including null terminator.

static char VuoScribbleStrip_text[VuoScribbleStrip_channels][VuoScribbleStrip_sections][VuoScribbleStrip_maxTextLength];  ///< Process-wide scribble-strip buffer.

/**
 * Allocates memory for the scribble strip.
 */
static void __attribute__((constructor)) VuoScribbleStrip_init(void)
{
//	VuoScribbleStrip_text = (char *)calloc(1, VuoScribbleStrip_sections * VuoScribbleStrip_channels * VuoScribbleStrip_maxTextLength + 1);
}

/**
 * Copies `text` into the process-wide scribble-strip buffer.
 */
void VuoScribbleStrip_set(VuoScribbleStripSection section, VuoInteger channel, VuoText text)
{
	if (!text)
	{
		VuoScribbleStrip_text[channel][section][0] = 0;
		return;
	}

	strncpy(VuoScribbleStrip_text[channel][section],
			text,
			VuoScribbleStrip_maxTextLength - 1); // Leave room for the null terminator.
}

/**
 * Retrieves text from the process-wide scribble-strip buffer.
 */
VuoText VuoScribbleStrip_get(VuoScribbleStripSection section, VuoInteger channel)
{
	return VuoText_makeWithMaxLength(VuoScribbleStrip_text[channel][section], VuoScribbleStrip_maxTextLength);
}
