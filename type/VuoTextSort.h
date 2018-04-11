/**
 * @file
 * VuoTextSort C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef const struct VuoList_VuoTextSort_struct { void *l; } * VuoList_VuoTextSort;
#define VuoList_VuoTextSort_TYPE_DEFINED
/// @}

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

VuoTextSort VuoTextSort_makeFromJson(struct json_object * js);
struct json_object * VuoTextSort_getJson(const VuoTextSort value);
VuoList_VuoTextSort VuoTextSort_getAllowedValues(void);
char * VuoTextSort_getSummary(const VuoTextSort value);

/// @{
/**
 * Automatically generated function.
 */
VuoTextSort VuoTextSort_makeFromString(const char *str);
char * VuoTextSort_getString(const VuoTextSort value);
/// @}

/**
 * @}
 */
