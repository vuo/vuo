/**
 * @file
 * VuoBlackmagicVideoMode implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DeckLinkAPI.h>

extern "C"
{
#include "type.h"
#include "VuoBlackmagicVideoMode.h"
#include "VuoList_VuoBlackmagicVideoMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Blackmagic Video Mode",
	"description" : "Video resolution and framerate.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoList_VuoBlackmagicVideoMode"
	]
});
#endif
/// @}
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "ntsc2398"
 * }
 */
VuoBlackmagicVideoMode VuoBlackmagicVideoMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoBlackmagicVideoMode value = VuoBlackmagicVideoMode_NTSC;

	if (strcmp(valueAsString, "NTSC2398") == 0)
		value = VuoBlackmagicVideoMode_NTSC2398;
	else if (strcmp(valueAsString, "NTSCp") == 0)
		value = VuoBlackmagicVideoMode_NTSCp;
	else if (strcmp(valueAsString, "PAL") == 0)
		value = VuoBlackmagicVideoMode_PAL;
	else if (strcmp(valueAsString, "PALp") == 0)
		value = VuoBlackmagicVideoMode_PALp;
	else if (strcmp(valueAsString, "HD720p50") == 0)
		value = VuoBlackmagicVideoMode_HD720p50;
	else if (strcmp(valueAsString, "HD720p5994") == 0)
		value = VuoBlackmagicVideoMode_HD720p5994;
	else if (strcmp(valueAsString, "HD720p60") == 0)
		value = VuoBlackmagicVideoMode_HD720p60;
	else if (strcmp(valueAsString, "HD1080p2398") == 0)
		value = VuoBlackmagicVideoMode_HD1080p2398;
	else if (strcmp(valueAsString, "HD1080p24") == 0)
		value = VuoBlackmagicVideoMode_HD1080p24;
	else if (strcmp(valueAsString, "HD1080p25") == 0)
		value = VuoBlackmagicVideoMode_HD1080p25;
	else if (strcmp(valueAsString, "HD1080p2997") == 0)
		value = VuoBlackmagicVideoMode_HD1080p2997;
	else if (strcmp(valueAsString, "HD1080p30") == 0)
		value = VuoBlackmagicVideoMode_HD1080p30;
	else if (strcmp(valueAsString, "HD1080i50") == 0)
		value = VuoBlackmagicVideoMode_HD1080i50;
	else if (strcmp(valueAsString, "HD1080i5994") == 0)
		value = VuoBlackmagicVideoMode_HD1080i5994;
	else if (strcmp(valueAsString, "HD1080i6000") == 0)
		value = VuoBlackmagicVideoMode_HD1080i6000;
	else if (strcmp(valueAsString, "HD1080p50") == 0)
		value = VuoBlackmagicVideoMode_HD1080p50;
	else if (strcmp(valueAsString, "HD1080p5994") == 0)
		value = VuoBlackmagicVideoMode_HD1080p5994;
	else if (strcmp(valueAsString, "HD1080p6000") == 0)
		value = VuoBlackmagicVideoMode_HD1080p6000;
	else if (strcmp(valueAsString, "2k2398") == 0)
		value = VuoBlackmagicVideoMode_2k2398;
	else if (strcmp(valueAsString, "2k24") == 0)
		value = VuoBlackmagicVideoMode_2k24;
	else if (strcmp(valueAsString, "2k25") == 0)
		value = VuoBlackmagicVideoMode_2k25;
	else if (strcmp(valueAsString, "2kDCI2398") == 0)
		value = VuoBlackmagicVideoMode_2kDCI2398;
	else if (strcmp(valueAsString, "2kDCI24") == 0)
		value = VuoBlackmagicVideoMode_2kDCI24;
	else if (strcmp(valueAsString, "2kDCI25") == 0)
		value = VuoBlackmagicVideoMode_2kDCI25;
	else if (strcmp(valueAsString, "4K2160p2398") == 0)
		value = VuoBlackmagicVideoMode_4K2160p2398;
	else if (strcmp(valueAsString, "4K2160p24") == 0)
		value = VuoBlackmagicVideoMode_4K2160p24;
	else if (strcmp(valueAsString, "4K2160p25") == 0)
		value = VuoBlackmagicVideoMode_4K2160p25;
	else if (strcmp(valueAsString, "4K2160p2997") == 0)
		value = VuoBlackmagicVideoMode_4K2160p2997;
	else if (strcmp(valueAsString, "4K2160p30") == 0)
		value = VuoBlackmagicVideoMode_4K2160p30;
	else if (strcmp(valueAsString, "4K2160p50") == 0)
		value = VuoBlackmagicVideoMode_4K2160p50;
	else if (strcmp(valueAsString, "4K2160p5994") == 0)
		value = VuoBlackmagicVideoMode_4K2160p5994;
	else if (strcmp(valueAsString, "4K2160p60") == 0)
		value = VuoBlackmagicVideoMode_4K2160p60;
	else if (strcmp(valueAsString, "4kDCI2398") == 0)
		value = VuoBlackmagicVideoMode_4kDCI2398;
	else if (strcmp(valueAsString, "4kDCI24") == 0)
		value = VuoBlackmagicVideoMode_4kDCI24;
	else if (strcmp(valueAsString, "4kDCI25") == 0)
		value = VuoBlackmagicVideoMode_4kDCI25;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoBlackmagicVideoMode_getJson(const VuoBlackmagicVideoMode value)
{
	const char *valueAsString = "NTSC";

	if (value == VuoBlackmagicVideoMode_NTSC2398)
		valueAsString = "NTSC2398";
	else if (value == VuoBlackmagicVideoMode_NTSCp)
		valueAsString = "NTSCp";
	else if (value == VuoBlackmagicVideoMode_PAL)
		valueAsString = "PAL";
	else if (value == VuoBlackmagicVideoMode_PALp)
		valueAsString = "PALp";
	else if (value == VuoBlackmagicVideoMode_HD720p50)
		valueAsString = "HD720p50";
	else if (value == VuoBlackmagicVideoMode_HD720p5994)
		valueAsString = "HD720p5994";
	else if (value == VuoBlackmagicVideoMode_HD720p60)
		valueAsString = "HD720p60";
	else if (value == VuoBlackmagicVideoMode_HD1080p2398)
		valueAsString = "HD1080p2398";
	else if (value == VuoBlackmagicVideoMode_HD1080p24)
		valueAsString = "HD1080p24";
	else if (value == VuoBlackmagicVideoMode_HD1080p25)
		valueAsString = "HD1080p25";
	else if (value == VuoBlackmagicVideoMode_HD1080p2997)
		valueAsString = "HD1080p2997";
	else if (value == VuoBlackmagicVideoMode_HD1080p30)
		valueAsString = "HD1080p30";
	else if (value == VuoBlackmagicVideoMode_HD1080i50)
		valueAsString = "HD1080i50";
	else if (value == VuoBlackmagicVideoMode_HD1080i5994)
		valueAsString = "HD1080i5994";
	else if (value == VuoBlackmagicVideoMode_HD1080i6000)
		valueAsString = "HD1080i6000";
	else if (value == VuoBlackmagicVideoMode_HD1080p50)
		valueAsString = "HD1080p50";
	else if (value == VuoBlackmagicVideoMode_HD1080p5994)
		valueAsString = "HD1080p5994";
	else if (value == VuoBlackmagicVideoMode_HD1080p6000)
		valueAsString = "HD1080p6000";
	else if (value == VuoBlackmagicVideoMode_2k2398)
		valueAsString = "2k2398";
	else if (value == VuoBlackmagicVideoMode_2k24)
		valueAsString = "2k24";
	else if (value == VuoBlackmagicVideoMode_2k25)
		valueAsString = "2k25";
	else if (value == VuoBlackmagicVideoMode_2kDCI2398)
		valueAsString = "2kDCI2398";
	else if (value == VuoBlackmagicVideoMode_2kDCI24)
		valueAsString = "2kDCI24";
	else if (value == VuoBlackmagicVideoMode_2kDCI25)
		valueAsString = "2kDCI25";
	else if (value == VuoBlackmagicVideoMode_4K2160p2398)
		valueAsString = "4K2160p2398";
	else if (value == VuoBlackmagicVideoMode_4K2160p24)
		valueAsString = "4K2160p24";
	else if (value == VuoBlackmagicVideoMode_4K2160p25)
		valueAsString = "4K2160p25";
	else if (value == VuoBlackmagicVideoMode_4K2160p2997)
		valueAsString = "4K2160p2997";
	else if (value == VuoBlackmagicVideoMode_4K2160p30)
		valueAsString = "4K2160p30";
	else if (value == VuoBlackmagicVideoMode_4K2160p50)
		valueAsString = "4K2160p50";
	else if (value == VuoBlackmagicVideoMode_4K2160p5994)
		valueAsString = "4K2160p5994";
	else if (value == VuoBlackmagicVideoMode_4K2160p60)
		valueAsString = "4K2160p60";
	else if (value == VuoBlackmagicVideoMode_4kDCI2398)
		valueAsString = "4kDCI2398";
	else if (value == VuoBlackmagicVideoMode_4kDCI24)
		valueAsString = "4kDCI24";
	else if (value == VuoBlackmagicVideoMode_4kDCI25)
		valueAsString = "4kDCI25";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBlackmagicVideoMode VuoBlackmagicVideoMode_getAllowedValues(void)
{
	VuoList_VuoBlackmagicVideoMode l = VuoListCreate_VuoBlackmagicVideoMode();
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_NTSC);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_NTSC2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_NTSCp);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_PAL);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_PALp);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD720p50);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD720p5994);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD720p60);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p24);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p25);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p2997);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p30);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080i50);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080i5994);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080i6000);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p50);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p5994);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_HD1080p6000);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2k2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2k24);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2k25);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2kDCI2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2kDCI24);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_2kDCI25);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p24);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p25);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p2997);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p30);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p50);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p5994);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4K2160p60);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4kDCI2398);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4kDCI24);
	VuoListAppendValue_VuoBlackmagicVideoMode(l, VuoBlackmagicVideoMode_4kDCI25);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoBlackmagicVideoMode_getSummary(const VuoBlackmagicVideoMode value)
{
	const char *valueAsString = "NTSC";

	if (value == VuoBlackmagicVideoMode_NTSC2398)
		valueAsString = "NTSC 23.98 fps (3:2 pulldown)";
	else if (value == VuoBlackmagicVideoMode_NTSCp)
		valueAsString = "NTSC Progressive";
	else if (value == VuoBlackmagicVideoMode_PAL)
		valueAsString = "PAL";
	else if (value == VuoBlackmagicVideoMode_PALp)
		valueAsString = "PAL Progressive";
	else if (value == VuoBlackmagicVideoMode_HD720p50)
		valueAsString = "HD 720p 50 fps";
	else if (value == VuoBlackmagicVideoMode_HD720p5994)
		valueAsString = "HD 720p 59.94 fps";
	else if (value == VuoBlackmagicVideoMode_HD720p60)
		valueAsString = "HD 720p 60 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p2398)
		valueAsString = "HD 1080p 23.98 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p24)
		valueAsString = "HD 1080p 24 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p25)
		valueAsString = "HD 1080p 25 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p2997)
		valueAsString = "HD 1080p 29.97 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p30)
		valueAsString = "HD 1080p 30 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080i50)
		valueAsString = "HD 1080i 50 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080i5994)
		valueAsString = "HD 1080i 59.94 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080i6000)
		valueAsString = "HD 1080i 60 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p50)
		valueAsString = "HD 1080p 50 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p5994)
		valueAsString = "HD 1080p 59.94 fps";
	else if (value == VuoBlackmagicVideoMode_HD1080p6000)
		valueAsString = "HD 1080p 60 fps";
	else if (value == VuoBlackmagicVideoMode_2k2398)
		valueAsString = "2K 23.98 fps";
	else if (value == VuoBlackmagicVideoMode_2k24)
		valueAsString = "2K 24 fps";
	else if (value == VuoBlackmagicVideoMode_2k25)
		valueAsString = "2K 25 fps";
	else if (value == VuoBlackmagicVideoMode_2kDCI2398)
		valueAsString = "2K DCI 23.98 fps";
	else if (value == VuoBlackmagicVideoMode_2kDCI24)
		valueAsString = "2K DCI 24 fps";
	else if (value == VuoBlackmagicVideoMode_2kDCI25)
		valueAsString = "2K DCI 25 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p2398)
		valueAsString = "4K 2160p 23.98 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p24)
		valueAsString = "4K 2160p 24 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p25)
		valueAsString = "4K 2160p 25 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p2997)
		valueAsString = "4K 2160p 29.97 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p30)
		valueAsString = "4K 2160p 30 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p50)
		valueAsString = "4K 2160p 50 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p5994)
		valueAsString = "4K 2160p 59.94 fps";
	else if (value == VuoBlackmagicVideoMode_4K2160p60)
		valueAsString = "4K 2160p 60 fps";
	else if (value == VuoBlackmagicVideoMode_4kDCI2398)
		valueAsString = "4K DCI 23.98 fps";
	else if (value == VuoBlackmagicVideoMode_4kDCI24)
		valueAsString = "4K DCI 24 fps";
	else if (value == VuoBlackmagicVideoMode_4kDCI25)
		valueAsString = "4K DCI 25 fps";

	return strdup(valueAsString);
}

/**
 * Returns the @ref VuoBlackmagicVideoMode corresponding to the input `BMDDisplayMode` value.
 */
VuoBlackmagicVideoMode VuoBlackmagicVideoMode_makeFromBMDDisplayMode(const uint32_t value)
{
	VuoBlackmagicVideoMode mode = VuoBlackmagicVideoMode_NTSC;

	if (value == bmdModeNTSC2398)
		mode = VuoBlackmagicVideoMode_NTSC2398;
	else if (value == bmdModeNTSCp)
		mode = VuoBlackmagicVideoMode_NTSCp;
	else if (value == bmdModePAL)
		mode = VuoBlackmagicVideoMode_PAL;
	else if (value == bmdModePALp)
		mode = VuoBlackmagicVideoMode_PALp;
	else if (value == bmdModeHD720p50)
		mode = VuoBlackmagicVideoMode_HD720p50;
	else if (value == bmdModeHD720p5994)
		mode = VuoBlackmagicVideoMode_HD720p5994;
	else if (value == bmdModeHD720p60)
		mode = VuoBlackmagicVideoMode_HD720p60;
	else if (value == bmdModeHD1080p2398)
		mode = VuoBlackmagicVideoMode_HD1080p2398;
	else if (value == bmdModeHD1080p24)
		mode = VuoBlackmagicVideoMode_HD1080p24;
	else if (value == bmdModeHD1080p25)
		mode = VuoBlackmagicVideoMode_HD1080p25;
	else if (value == bmdModeHD1080p2997)
		mode = VuoBlackmagicVideoMode_HD1080p2997;
	else if (value == bmdModeHD1080p30)
		mode = VuoBlackmagicVideoMode_HD1080p30;
	else if (value == bmdModeHD1080i50)
		mode = VuoBlackmagicVideoMode_HD1080i50;
	else if (value == bmdModeHD1080i5994)
		mode = VuoBlackmagicVideoMode_HD1080i5994;
	else if (value == bmdModeHD1080i6000)
		mode = VuoBlackmagicVideoMode_HD1080i6000;
	else if (value == bmdModeHD1080p50)
		mode = VuoBlackmagicVideoMode_HD1080p50;
	else if (value == bmdModeHD1080p5994)
		mode = VuoBlackmagicVideoMode_HD1080p5994;
	else if (value == bmdModeHD1080p6000)
		mode = VuoBlackmagicVideoMode_HD1080p6000;
	else if (value == bmdMode2k2398)
		mode = VuoBlackmagicVideoMode_2k2398;
	else if (value == bmdMode2k24)
		mode = VuoBlackmagicVideoMode_2k24;
	else if (value == bmdMode2k25)
		mode = VuoBlackmagicVideoMode_2k25;
	else if (value == bmdMode2kDCI2398)
		mode = VuoBlackmagicVideoMode_2kDCI2398;
	else if (value == bmdMode2kDCI24)
		mode = VuoBlackmagicVideoMode_2kDCI24;
	else if (value == bmdMode2kDCI25)
		mode = VuoBlackmagicVideoMode_2kDCI25;
	else if (value == bmdMode4K2160p2398)
		mode = VuoBlackmagicVideoMode_4K2160p2398;
	else if (value == bmdMode4K2160p24)
		mode = VuoBlackmagicVideoMode_4K2160p24;
	else if (value == bmdMode4K2160p25)
		mode = VuoBlackmagicVideoMode_4K2160p25;
	else if (value == bmdMode4K2160p2997)
		mode = VuoBlackmagicVideoMode_4K2160p2997;
	else if (value == bmdMode4K2160p30)
		mode = VuoBlackmagicVideoMode_4K2160p30;
	else if (value == bmdMode4K2160p50)
		mode = VuoBlackmagicVideoMode_4K2160p50;
	else if (value == bmdMode4K2160p5994)
		mode = VuoBlackmagicVideoMode_4K2160p5994;
	else if (value == bmdMode4K2160p60)
		mode = VuoBlackmagicVideoMode_4K2160p60;
	else if (value == bmdMode4kDCI2398)
		mode = VuoBlackmagicVideoMode_4kDCI2398;
	else if (value == bmdMode4kDCI24)
		mode = VuoBlackmagicVideoMode_4kDCI24;
	else if (value == bmdMode4kDCI25)
		mode = VuoBlackmagicVideoMode_4kDCI25;

	return mode;
}

/**
 * Returns the `BMDDisplayMode` corresponding to the input `value`.
 */
uint32_t VuoBlackmagicVideoMode_getBMDDisplayMode(const VuoBlackmagicVideoMode value)
{
	BMDDisplayMode mode = bmdModeNTSC;

	if (value == VuoBlackmagicVideoMode_NTSC2398)
		mode = bmdModeNTSC2398;
	else if (value == VuoBlackmagicVideoMode_NTSCp)
		mode = bmdModeNTSCp;
	else if (value == VuoBlackmagicVideoMode_PAL)
		mode = bmdModePAL;
	else if (value == VuoBlackmagicVideoMode_PALp)
		mode = bmdModePALp;
	else if (value == VuoBlackmagicVideoMode_HD720p50)
		mode = bmdModeHD720p50;
	else if (value == VuoBlackmagicVideoMode_HD720p5994)
		mode = bmdModeHD720p5994;
	else if (value == VuoBlackmagicVideoMode_HD720p60)
		mode = bmdModeHD720p60;
	else if (value == VuoBlackmagicVideoMode_HD1080p2398)
		mode = bmdModeHD1080p2398;
	else if (value == VuoBlackmagicVideoMode_HD1080p24)
		mode = bmdModeHD1080p24;
	else if (value == VuoBlackmagicVideoMode_HD1080p25)
		mode = bmdModeHD1080p25;
	else if (value == VuoBlackmagicVideoMode_HD1080p2997)
		mode = bmdModeHD1080p2997;
	else if (value == VuoBlackmagicVideoMode_HD1080p30)
		mode = bmdModeHD1080p30;
	else if (value == VuoBlackmagicVideoMode_HD1080i50)
		mode = bmdModeHD1080i50;
	else if (value == VuoBlackmagicVideoMode_HD1080i5994)
		mode = bmdModeHD1080i5994;
	else if (value == VuoBlackmagicVideoMode_HD1080i6000)
		mode = bmdModeHD1080i6000;
	else if (value == VuoBlackmagicVideoMode_HD1080p50)
		mode = bmdModeHD1080p50;
	else if (value == VuoBlackmagicVideoMode_HD1080p5994)
		mode = bmdModeHD1080p5994;
	else if (value == VuoBlackmagicVideoMode_HD1080p6000)
		mode = bmdModeHD1080p6000;
	else if (value == VuoBlackmagicVideoMode_2k2398)
		mode = bmdMode2k2398;
	else if (value == VuoBlackmagicVideoMode_2k24)
		mode = bmdMode2k24;
	else if (value == VuoBlackmagicVideoMode_2k25)
		mode = bmdMode2k25;
	else if (value == VuoBlackmagicVideoMode_2kDCI2398)
		mode = bmdMode2kDCI2398;
	else if (value == VuoBlackmagicVideoMode_2kDCI24)
		mode = bmdMode2kDCI24;
	else if (value == VuoBlackmagicVideoMode_2kDCI25)
		mode = bmdMode2kDCI25;
	else if (value == VuoBlackmagicVideoMode_4K2160p2398)
		mode = bmdMode4K2160p2398;
	else if (value == VuoBlackmagicVideoMode_4K2160p24)
		mode = bmdMode4K2160p24;
	else if (value == VuoBlackmagicVideoMode_4K2160p25)
		mode = bmdMode4K2160p25;
	else if (value == VuoBlackmagicVideoMode_4K2160p2997)
		mode = bmdMode4K2160p2997;
	else if (value == VuoBlackmagicVideoMode_4K2160p30)
		mode = bmdMode4K2160p30;
	else if (value == VuoBlackmagicVideoMode_4K2160p50)
		mode = bmdMode4K2160p50;
	else if (value == VuoBlackmagicVideoMode_4K2160p5994)
		mode = bmdMode4K2160p5994;
	else if (value == VuoBlackmagicVideoMode_4K2160p60)
		mode = bmdMode4K2160p60;
	else if (value == VuoBlackmagicVideoMode_4kDCI2398)
		mode = bmdMode4kDCI2398;
	else if (value == VuoBlackmagicVideoMode_4kDCI24)
		mode = bmdMode4kDCI24;
	else if (value == VuoBlackmagicVideoMode_4kDCI25)
		mode = bmdMode4kDCI25;

	return mode;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoBlackmagicVideoMode_areEqual(const VuoBlackmagicVideoMode valueA, const VuoBlackmagicVideoMode valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoBlackmagicVideoMode_isLessThan(const VuoBlackmagicVideoMode valueA, const VuoBlackmagicVideoMode valueB)
{
	return valueA < valueB;
}
