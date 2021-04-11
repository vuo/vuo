/**
 * @file
 * VuoApp interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool VuoApp_isMainThread(void);
void VuoApp_executeOnMainThread(void (^block)(void));
char *VuoApp_getName(void);

void VuoApp_init(bool requiresDockIcon);
void *VuoApp_setMenuItems(void *items);
void VuoApp_setMenu(void *menu);

extern const double VuoApp_windowFadeSeconds;

#ifdef __cplusplus
}
#endif
