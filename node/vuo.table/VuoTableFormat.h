/**
 * @file
 * VuoTableFormat C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ List type.
typedef const struct VuoList_VuoTableFormat_struct { void *l; } * VuoList_VuoTableFormat;
#define VuoList_VuoTableFormat_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoTableFormat VuoTableFormat
 * Text formats for parsing/serializing a VuoTable.
 *
 * @{
 */

/**
 * Text formats for parsing/serializing a VuoTable.
 */
typedef enum {
	VuoTableFormat_Csv,
	VuoTableFormat_Tsv
} VuoTableFormat;

VuoTableFormat VuoTableFormat_makeFromJson(struct json_object * js);
struct json_object * VuoTableFormat_getJson(const VuoTableFormat value);
VuoList_VuoTableFormat VuoTableFormat_getAllowedValues(void);
char * VuoTableFormat_getSummary(const VuoTableFormat value);

/// @{
/**
 * Automatically generated functions.
 */
VuoTableFormat VuoTableFormat_makeFromString(const char *str);
char * VuoTableFormat_getString(const VuoTableFormat value);
void VuoTableFormat_retain(VuoTableFormat value);
void VuoTableFormat_release(VuoTableFormat value);
/// @}

/**
 * @}
*/
