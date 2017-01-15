/**
 * @file
 * VuoOsStatus interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <CoreFoundation/CoreFoundation.h>

char *VuoOsStatus_getText(OSStatus error);

#ifdef __cplusplus
}
#endif
