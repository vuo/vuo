/**
 * @file
 * VuoDictionary_VuoText_VuoReal C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoDictionary_VuoText_VuoReal_h
#define VuoDictionary_VuoText_VuoReal_h

/**
 * @ingroup VuoTypes
 * @defgroup VuoDictionary_VuoText_VuoReal VuoDictionary_VuoText_VuoReal
 * A mapping from keys to values. (To be replaced with a generic VuoDictionary type in the future.)
 *
 * @{
 */

#include "VuoReal.h"
#include "VuoText.h"
#include "VuoList_VuoReal.h"
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
	VuoList_VuoReal values;
} VuoDictionary_VuoText_VuoReal;

VuoDictionary_VuoText_VuoReal VuoDictionary_VuoText_VuoReal_makeFromJson(struct json_object * js);
struct json_object * VuoDictionary_VuoText_VuoReal_getJson(const VuoDictionary_VuoText_VuoReal value);
char * VuoDictionary_VuoText_VuoReal_getSummary(const VuoDictionary_VuoText_VuoReal value);

VuoDictionary_VuoText_VuoReal VuoDictionaryCreate_VuoText_VuoReal(void);
VuoDictionary_VuoText_VuoReal VuoDictionaryCreateWithLists_VuoText_VuoReal(const VuoList_VuoText keys, const VuoList_VuoReal values);
VuoReal VuoDictionaryGetValueForKey_VuoText_VuoReal(VuoDictionary_VuoText_VuoReal d, VuoText key);
void VuoDictionarySetKeyValue_VuoText_VuoReal(VuoDictionary_VuoText_VuoReal d, VuoText key, VuoReal value);

void VuoDictionary_VuoText_VuoReal_retain(VuoDictionary_VuoText_VuoReal value);
void VuoDictionary_VuoText_VuoReal_release(VuoDictionary_VuoText_VuoReal value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoDictionary_VuoText_VuoReal_getString(const VuoDictionary_VuoText_VuoReal value);
/// @}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
