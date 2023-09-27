/**
 * @file
 * VuoIoReturn interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <IOKit/IOReturn.h>

char *VuoIoReturn_getText(IOReturn ret);

#ifdef __cplusplus
}
#endif
