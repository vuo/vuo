/**
 * @file
 * VuoScreenCommon interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoScreen.h"
#include "VuoList_VuoScreen.h"

void *VuoScreen_getNSScreen(VuoScreen screen);
VuoList_VuoScreen VuoScreen_getList(void);
VuoScreen VuoScreen_getActive(void);
VuoScreen VuoScreen_getPrimary(void);
VuoScreen VuoScreen_getSecondary(void);
