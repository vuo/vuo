/**
 * @file
 * VuoScribbleStrip interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * A group of physical controls in a channel strip.
 */
typedef enum
{
	VuoScribbleStrip_Knobs,
	VuoScribbleStrip_ButtonRow1,
	VuoScribbleStrip_ButtonRow2,
	VuoScribbleStrip_Faders,
} VuoScribbleStripSection;

void VuoScribbleStrip_set(VuoScribbleStripSection section, VuoInteger channel, VuoText text);
VuoText VuoScribbleStrip_get(VuoScribbleStripSection section, VuoInteger channel);
