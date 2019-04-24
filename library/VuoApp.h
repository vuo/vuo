/**
 * @file
 * VuoApp interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool VuoApp_isMainThread(void);
void VuoApp_executeOnMainThread(void (^block)(void));
char *VuoApp_getName(void);
const char *VuoApp_getVuoFrameworkPath(void);

void VuoApp_init(void);
void *VuoApp_setMenuItems(void *items);
void VuoApp_setMenu(void *menu);

#ifdef __cplusplus
}
#endif
