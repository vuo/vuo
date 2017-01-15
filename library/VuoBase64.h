/**
 * @file
 * VuoBase64 interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *VuoBase64_encode(long long length, char const *data);
char *VuoBase64_decode(char const *text, long long *outputLength);

#ifdef __cplusplus
}
#endif
