/**
 * @file
 * VuoScreenCommon interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoScreen.h"
#include "VuoList_VuoScreen.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "node_header.h"

void *VuoScreen_getNSScreen(VuoScreen screen);
VuoScreen VuoScreen_makeFromNSScreen(void *screen);

VuoList_VuoScreen VuoScreen_getList(void);
VuoScreen VuoScreen_getActive(void);
VuoScreen VuoScreen_getPrimary(void);
VuoScreen VuoScreen_getSecondary(void);

void VuoScreen_use(void);
void VuoScreen_disuse(void);
void VuoScreen_addDevicesChangedTriggers   (VuoOutputTrigger(screens, VuoList_VuoScreen));
void VuoScreen_removeDevicesChangedTriggers(VuoOutputTrigger(screens, VuoList_VuoScreen));

#ifdef __cplusplus
}
#endif
