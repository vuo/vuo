/**
 * @file
 * VuoText C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

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

#include <stdbool.h>
#include <string.h>
struct json_object;

/**
 * A Unicode (UTF-8) text string.
 */
typedef const char * VuoText;

VuoText VuoText_makeFromJson(struct json_object * js);
struct json_object * VuoText_getJson(const VuoText value);
char * VuoText_getSummary(const VuoText value);

VuoText VuoText_make(const char * unquotedString);
VuoText VuoText_makeWithMaxLength(const void *data, const size_t maxLength);
VuoText VuoText_makeFromCFString(const void *cfString);
VuoText VuoText_makeFromData(const unsigned char *data, const unsigned long size);
size_t VuoText_length(const VuoText text);

#define VuoText_SUPPORTS_COMPARISON
bool VuoText_areEqual(const VuoText text1, const VuoText text2);
bool VuoText_isLessThan(const VuoText text1, const VuoText text2);

size_t VuoText_findFirstOccurrence(const VuoText string, const VuoText substring, const size_t startIndex);
size_t VuoText_findLastOccurrence(const VuoText string, const VuoText substring);
VuoText VuoText_substring(const VuoText string, int startIndex, int length);
VuoText VuoText_append(VuoText *texts, size_t textsCount);
VuoText * VuoText_split(VuoText text, VuoText separator, bool includeEmptyParts, size_t *partsCount);
VuoText VuoText_replace(VuoText subject, VuoText stringToFind, VuoText replacement);
VuoText VuoText_truncateWithEllipsis(const VuoText subject, int maxLength);
VuoText VuoText_trim(const VuoText text);

#ifndef DOXYGEN
	#define VUOTEXT_FORMAT_ATTRIBUTE __attribute__((format(printf, 1, 2)))
#else
	#define VUOTEXT_FORMAT_ATTRIBUTE
#endif
char *VuoText_format(const char *format, ...) VUOTEXT_FORMAT_ATTRIBUTE;

/// @{
/**
 * Automatically generated function.
 */
VuoText VuoText_makeFromString(const char *str);
char * VuoText_getString(const VuoText value);
void VuoText_retain(VuoText value);
void VuoText_release(VuoText value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
