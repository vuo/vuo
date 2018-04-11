/**
 * @file
 * VuoApp interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C" {
#endif

bool VuoApp_isMainThread(void);
void VuoApp_executeOnMainThread(void (^block)(void));
char *VuoApp_getName(void);
const char *VuoApp_getVuoFrameworkPath(void);

#ifdef __cplusplus
}
#endif
