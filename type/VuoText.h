/**
 * @file
 * VuoText C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSTRING_H
#define VUOSTRING_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoText VuoText
 * A Unicode (UTF-8) text string.
 *
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * A Unicode (UTF-8) text string.
 */
typedef const char * VuoText;

VuoText VuoText_valueFromJson(struct json_object * js);
struct json_object * VuoText_jsonFromValue(const VuoText value);
char * VuoText_summaryFromValue(const VuoText value);

VuoText VuoText_make(const char * unquotedString);
size_t VuoText_length(const VuoText string);
size_t VuoText_getIndexOfLastCharacter(const VuoText string, const VuoText character);
VuoText VuoText_substring(const VuoText string, int startIndex, int length);
VuoText VuoText_append(VuoText *texts, unsigned long textsCount);

/// @{
/**
 * Automatically generated function.
 */
VuoText VuoText_valueFromString(const char *str);
char * VuoText_stringFromValue(const VuoText value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
