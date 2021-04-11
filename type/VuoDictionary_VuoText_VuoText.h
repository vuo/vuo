/**
 * @file
 * VuoDictionary_VuoText_VuoText C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoDictionary_VuoText_VuoText VuoDictionary_VuoText_VuoText
 * A mapping from keys to values. (To be replaced with a generic VuoDictionary type in the future.)
 *
 * @{
 */

#include "VuoText.h"
#include "VuoText.h"
#include "VuoList_VuoText.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A mapping from keys to values.
 */
typedef struct
{
	VuoList_VuoText keys;
	VuoList_VuoText values;
} VuoDictionary_VuoText_VuoText;

VuoDictionary_VuoText_VuoText VuoDictionary_VuoText_VuoText_makeFromJson(struct json_object *js);
struct json_object * VuoDictionary_VuoText_VuoText_getJson(const VuoDictionary_VuoText_VuoText value);
char * VuoDictionary_VuoText_VuoText_getSummary(const VuoDictionary_VuoText_VuoText value);

VuoDictionary_VuoText_VuoText VuoDictionaryCreate_VuoText_VuoText(void);
VuoDictionary_VuoText_VuoText VuoDictionaryCreateWithLists_VuoText_VuoText(const VuoList_VuoText keys, const VuoList_VuoText values);
VuoText VuoDictionaryGetValueForKey_VuoText_VuoText(VuoDictionary_VuoText_VuoText d, VuoText key);
void VuoDictionarySetKeyValue_VuoText_VuoText(VuoDictionary_VuoText_VuoText d, VuoText key, VuoText value);

/// @{
/**
 * Automatically generated function.
 */
VuoDictionary_VuoText_VuoText VuoDictionary_VuoText_VuoText_makeFromString(const char *str);
char * VuoDictionary_VuoText_VuoText_getString(const VuoDictionary_VuoText_VuoText value);
void VuoDictionary_VuoText_VuoText_retain(VuoDictionary_VuoText_VuoText value);
void VuoDictionary_VuoText_VuoText_release(VuoDictionary_VuoText_VuoText value);
/// @}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
