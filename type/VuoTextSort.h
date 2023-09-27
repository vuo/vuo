/**
 * @file
 * VuoTextSort C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoTextSort_h
#define VuoTextSort_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoTextSort VuoTextSort
 * Ways that text can be interpreted for sorting.
 *
 * @{
 */

/**
 * Ways that text can be interpreted for sorting.
 */
typedef enum
{
	VuoTextSort_Text,
	VuoTextSort_TextCaseSensitive,
	VuoTextSort_Number,
	VuoTextSort_Date
} VuoTextSort;

#include "VuoList_VuoTextSort.h"

VuoTextSort VuoTextSort_makeFromJson(struct json_object * js);
struct json_object * VuoTextSort_getJson(const VuoTextSort value);
VuoList_VuoTextSort VuoTextSort_getAllowedValues(void);
char * VuoTextSort_getSummary(const VuoTextSort value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoTextSort_getString(const VuoTextSort value);
void VuoTextSort_retain(VuoTextSort value);
void VuoTextSort_release(VuoTextSort value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
