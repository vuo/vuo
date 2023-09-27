/**
 * @file
 * VuoText C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoText_h
#define VuoText_h

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

#include "VuoInteger.h"
#include "VuoTextCase.h"
#include "VuoTextComparison.h"

#include <stdint.h>
#include <string.h>
#include <xlocale.h>

/**
 * A Unicode (UTF-8) text string.
 */
typedef const char * VuoText;

#define VuoText_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoText.h"

/**
 * Where to truncate text.
 */
typedef enum
{
	VuoTextTruncation_Beginning,
	VuoTextTruncation_End
} VuoTextTruncation;

void VuoText_performWithUTF8Locale(void (^function)(locale_t utf8Locale));
void VuoText_performWithSystemLocale(void (^function)(locale_t systemLocale));

VuoText VuoText_makeFromJson(struct json_object * js);
struct json_object * VuoText_getJson(const VuoText value);
char * VuoText_getSummary(const VuoText value);

VuoText VuoText_make(const char *string);
VuoText VuoText_makeWithoutCopying(const char *string);
VuoText VuoText_makeWithMaxLength(const void *data, const size_t maxLength);
VuoText VuoText_makeFromCFString(const void *cfString);
VuoText VuoText_makeFromData(const unsigned char *data, const unsigned long size);
VuoText VuoText_makeFromUtf32(const uint32_t* data, size_t length);
VuoText VuoText_makeFromMacRoman(const char *string);
size_t VuoText_length(const VuoText text);
size_t VuoText_byteCount(const VuoText text);
bool VuoText_isEmpty(const VuoText text);
bool VuoText_isPopulated(const VuoText text);
bool VuoText_areEqual(const VuoText text1, const VuoText text2);
bool VuoText_isLessThan(const VuoText text1, const VuoText text2);
bool VuoText_isLessThanCaseInsensitive(const VuoText text1, const VuoText text2);
bool VuoText_isLessThanNumeric(const VuoText text1, const VuoText text2);
bool VuoText_compare(VuoText a, VuoTextComparison comparison, VuoText b);
size_t VuoText_findFirstOccurrence(const VuoText string, const VuoText substring, const size_t startIndex);
size_t VuoText_findLastOccurrence(const VuoText string, const VuoText substring);
VuoList_VuoInteger VuoText_findOccurrences(const VuoText string, const VuoText substring);
VuoText VuoText_substring(const VuoText string, int startIndex, int length);
VuoText VuoText_insert(const VuoText string, int startIndex, const VuoText newText);
VuoText VuoText_removeAt(const VuoText string, int startIndex, int length);
VuoText VuoText_append(VuoText *texts, size_t textsCount);
VuoText VuoText_appendWithSeparator(VuoList_VuoText texts, VuoText separator, bool includeEmptyParts);
VuoText * VuoText_split(VuoText text, VuoText separator, bool includeEmptyParts, size_t *partsCount);
VuoText VuoText_replace(VuoText subject, VuoText stringToFind, VuoText replacement);
VuoText VuoText_truncateWithEllipsis(const VuoText subject, int maxLength, VuoTextTruncation where);
VuoText VuoText_trim(const VuoText text);
VuoText VuoText_changeCase(const VuoText text, VuoTextCase textCase);
uint32_t* VuoText_getUtf32Values(const VuoText text, size_t* length);

#ifndef DOXYGEN
	#define VUOTEXT_FORMAT_ATTRIBUTE __attribute__((format(printf, 1, 2)))
#else
	/// Work around spurious Doxygen warning about VuoText_format() not being documented.
	#define VUOTEXT_FORMAT_ATTRIBUTE
#endif
char *VuoText_format(const char *format, ...) VUOTEXT_FORMAT_ATTRIBUTE;

/// @{
/**
 * Automatically generated function.
 */
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

#endif
