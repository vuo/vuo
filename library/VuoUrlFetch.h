/**
 * @file
 * VuoUrlFetch interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoText.h"

bool VuoUrl_fetch(const char *url, void **data, unsigned int *dataLength);

#ifdef __cplusplus
}
#endif
