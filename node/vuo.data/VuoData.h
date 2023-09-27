/**
 * @file
 * VuoData C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoData VuoData
 * A blob of 8-bit binary data.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * A blob of 8-bit binary data.
 */
typedef struct
{
	VuoInteger size;	///< Number of bytes in `data`.
	char *data;	///< 8-bit data.
} VuoData;

#define VuoData_SUPPORTS_COMPARISON

VuoData VuoData_makeFromJson(struct json_object *js);
struct json_object *VuoData_getJson(const VuoData value);
char *VuoData_getSummary(const VuoData value);

bool VuoData_areEqual(const VuoData valueA, const VuoData valueB);
bool VuoData_isLessThan(const VuoData valueA, const VuoData valueB);

VuoData VuoData_make(VuoInteger size, unsigned char *data);
VuoData VuoData_makeFromText(const VuoText text);
char * VuoData_copyToString(const VuoData data);

void VuoData_retain(VuoData value);
void VuoData_release(VuoData value);

/**
 * Automatically generated function.
 */
///@{
char *VuoData_getString(const VuoData value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
