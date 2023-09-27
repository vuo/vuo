/**
 * @file
 * VuoTextCase C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoTextCase_h
#define VuoTextCase_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoTextCase VuoTextCase
 * Describes different text casing styles.
 *
 * @{
 */

/**
 * Describes different text casing styles.
 */
typedef enum {
	VuoTextCase_LowercaseAll, 					///< all characters are lowercase.
	VuoTextCase_UppercaseAll, 					///< ALL CHARACTERS ARE UPPERCASE.
	VuoTextCase_UppercaseFirstLetterWord, 		///< The First Letter In Each Word Is Uppercase.
	VuoTextCase_UppercaseFirstLetterSentence 	///< The first letter in every sentence is uppercase.
} VuoTextCase;

#include "VuoList_VuoTextCase.h"

VuoTextCase VuoTextCase_makeFromJson(struct json_object * js);
struct json_object * VuoTextCase_getJson(const VuoTextCase value);
VuoList_VuoTextCase VuoTextCase_getAllowedValues(void);
char * VuoTextCase_getSummary(const VuoTextCase value);

/**
 * Automatically generated function.
 */
///@{
char * VuoTextCase_getString(const VuoTextCase value);
void VuoTextCase_retain(VuoTextCase value);
void VuoTextCase_release(VuoTextCase value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
