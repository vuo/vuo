/**
 * @file
 * VuoText implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <Carbon/Carbon.h>
#include <fnmatch.h>
#include <regex.h>
#include <string.h>
#include <xlocale.h>

#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text",
					 "description" : "A Unicode (UTF-8) text string.",
					 "keywords" : [ "char *", "character" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"Carbon.framework",
						"VuoInteger",
						"VuoReal",
						"VuoTextCase",
						"VuoList_VuoInteger",
						"VuoList_VuoText",
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoText
 * Decodes the JSON object @c js, expected to contain a UTF-8 string, to create a new value.
 */
VuoText VuoText_makeFromJson(json_object * js)
{
	const char *textString = NULL;
	if (json_object_get_type(js) == json_type_string)
		textString = VuoText_make(json_object_get_string(js));
	return textString;
}

/**
 * @ingroup VuoText
 * Encodes @c value as a JSON object.
 */
json_object * VuoText_getJson(const VuoText value)
{
	if (!value)
		return NULL;

	return json_object_new_string(value);
}

/**
 * If `subject` is less than or equal to `maxLength` Unicode characters long, returns `subject`.
 *
 * If `subject` is greater than `maxLength`, truncates `subject` to `maxLength` characters and adds a Unicode ellipsis.
 */
VuoText VuoText_truncateWithEllipsis(const VuoText subject, int maxLength, VuoTextTruncation where)
{
	if (!subject)
		return VuoText_make("");

	size_t length = VuoText_length(subject);
	if (length <= maxLength)
		return subject;
	else
	{
		VuoText abbreviation = VuoText_substring(subject, (where == VuoTextTruncation_End ? 1 : 1 + length - maxLength), maxLength);
		VuoText ellipsis = VuoText_make("…");
		VuoText summaryParts[2] = { abbreviation, ellipsis };
		if (where == VuoTextTruncation_Beginning)
		{
			summaryParts[0] = ellipsis;
			summaryParts[1] = abbreviation;
		}
		VuoText summaryWhole = VuoText_append(summaryParts, 2);

		VuoRetain(abbreviation);
		VuoRelease(abbreviation);
		VuoRetain(ellipsis);
		VuoRelease(ellipsis);
		return summaryWhole;
	}
}

/**
 * @ingroup VuoText
 * Creates a new UTF-8 C string from @c value, or, if it's more than 50 Unicode characters long, creates an aposiopesis.
 *
 * @eg{Hello World!}
 * @eg{I would like to convey my gree...}
 */
char * VuoText_getSummary(const VuoText value)
{
	if (VuoText_isEmpty(value))
		return strdup("<code>&#0;</code>");

	VuoText truncatedText = VuoText_truncateWithEllipsis(value, 1024, VuoTextTruncation_End);

	// VuoText_truncateWithEllipsis() may return the same string passed in.
	// Only dealloc it if new text was actually created.
	if (truncatedText != value)
		VuoRetain(truncatedText);

	VuoText escapedText = VuoText_replace(truncatedText, "&", "&amp;");
	if (truncatedText != value)
		VuoRelease(truncatedText);

	VuoLocal(escapedText);
	VuoText escapedText2 = VuoText_replace(escapedText, "<", "&lt;");
	VuoLocal(escapedText2);
	char *summary = VuoText_format("<code>%s</code>", escapedText2);
	return summary;
}

/**
 * @ingroup VuoText
 * Creates a VuoText value from an unquoted string (unlike @c VuoText_makeFromString(), which expects a quoted string).
 */
VuoText VuoText_make(const char * unquotedString)
{
	VuoText text;
	if (unquotedString)
		text = strdup(unquotedString);
	else
		text = strdup("");
	VuoRegister(text, free);
	return text;
}

/**
 * Creates a VuoText value from an untrusted source (one that might not contain a NULL terminator within its memory page).
 *
 * Use this, for example, to safely handle a string that's part of a network packet.
 *
 * If `data` ends with a NULL terminator, this function simply copies it into a new VuoText instance.
 *
 * If `data` does not end with a NULL terminator, this function scans up to `maxLength` bytes in `data`.
 * If it finds a NULL terminator, it returns text up to and including the NULL terminator.
 * If it doesn't find a NULL terminator, it returns `maxLength` characters followed by a NULL terminator.
 *
 * If `data` is in a multi-byte UTF8 character sequence when `maxLength` is reached,
 * the sequence will be terminated partway, probably resulting in an invalid UTF8 sequence.
 */
VuoText VuoText_makeWithMaxLength(const void *data, const size_t maxLength)
{
	char *text;
	if (data && ((char *)data)[maxLength-1] == 0)
	{
		// Faster than scanning through the array byte-by-byte.
		text = (char *)calloc(1, maxLength);
		memcpy(text, data, maxLength);
	}
	else if (data)
	{
		text = (char *)calloc(1, maxLength+1);
		for (unsigned int i = 0; i < maxLength; ++i)
		{
			text[i] = ((char *)data)[i];
			if (((char *)data)[i] == 0)
				break;
		}
	}
	else
		text = strdup("");
	VuoRegister(text, free);
	return text;
}

/**
 * Creates a @ref VuoText value from a `CFStringRef`.
 */
VuoText VuoText_makeFromCFString(const void *cfs)
{
	if (!cfs)
		return NULL;

	CFStringRef cfString = (CFStringRef)cfs;

	// https://stackoverflow.com/questions/1609565/whats-the-cfstring-equiv-of-nsstrings-utf8string
	const char *utf8StringPtr = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8);
	if (utf8StringPtr)
		return VuoText_make(utf8StringPtr);
	else
	{
		CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfString), kCFStringEncodingUTF8) + 1;
		char *t = calloc(1, maxBytes);
		CFStringGetCString(cfString, t, maxBytes, kCFStringEncodingUTF8);
		VuoRegister(t, free);
		return t;
	}
}

/**
 * @ref VuoText_isValidUtf8 is based on code by Bjoern Hoehrmann.
 *
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See https://web.archive.org/web/20190528032751/http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define UTF8_ACCEPT 0	///< enough bytes have been read for a character
#define UTF8_REJECT 1	///< the byte is not allowed to occur at its position

/**
 * The first part maps bytes to character classes, the second part encodes a deterministic finite automaton using these character classes as transitions.
 */
static const uint8_t utf8d[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
	8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
	0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
	0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
	0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
	1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
	1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
	1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

/**
 * Returns true if `data` is valid UTF-8 text.
 */
static bool VuoText_isValidUtf8(const unsigned char *data, unsigned long size)
{
	// Faster than CFStringCreateFromExternalRepresentation.
	uint32_t codepoint;
	for (uint32_t pos = 0, state = 0; *data && pos++ < size; ++data)
	{
		uint32_t byte = *data;
		uint32_t type = utf8d[byte];

		codepoint = (state != UTF8_ACCEPT) ?
		(byte & 0x3fu) | (codepoint << 6) :
		(0xff >> type) & (byte);

		state = utf8d[256 + state*16 + type];

		if (state == UTF8_REJECT)
			return false;
	}

	return true;
}

/**
 * Attempts to interpret `data` as UTF-8 text.
 *
 * Returns NULL if `data` is not valid UTF-8 text (e.g., if it contains byte 0xfe or 0xff).
 *
 * `data` is copied.
 */
VuoText VuoText_makeFromData(const unsigned char *data, const unsigned long size)
{
	if (!size || !data)
		return NULL;

	if (!VuoText_isValidUtf8(data, size))
		return NULL;

	return VuoText_makeWithMaxLength(data, size);
}

/**
 * Create a new VuoText string from an array of UTF-32 values.
 *
 * Returns NULL if `data` is not valid UTF-32 text.
 *
 * `data` is copied.
 */
VuoText VuoText_makeFromUtf32(const uint32_t* data, size_t size)
{
	CFStringRef cf_str = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*) data, size * sizeof(uint32_t), kCFStringEncodingUTF32LE, false);

	if(cf_str != NULL)
	{
		CFIndex str_len = CFStringGetLength(cf_str);
		CFRange range = CFRangeMake(0, str_len);
		CFIndex usedBufLen;
		CFIndex length = CFStringGetBytes(cf_str, range, kCFStringEncodingUTF8, '?', false, NULL, str_len, &usedBufLen );

		if(length > 0)
		{
			char* buffer = (char*) malloc(sizeof(char) * usedBufLen + 1);
			CFIndex used;
			CFStringGetBytes(cf_str, range, kCFStringEncodingUTF8, '?', false, (uint8_t*) buffer, usedBufLen + 1, &used);
			buffer[used] = '\0';
			VuoText text = VuoText_make(buffer);
			free(buffer);
			CFRelease(cf_str);
			return text;
		}

		CFRelease(cf_str);
	}

	return VuoText_make("");
}

/**
 * Creates a new VuoText from a MacRoman-encoded string.
 */
VuoText VuoText_makeFromMacRoman(const char *string)
{
	if (!string)
		return NULL;

	size_t len = strlen(string);
	CFStringRef cf = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)string, len, kCFStringEncodingMacRoman, false);
	if (!cf)
		return NULL;

	VuoText t = VuoText_makeFromCFString(cf);

	CFRelease(cf);
	return t;
}

/**
 * @ingroup VuoText
 * Returns the number of Unicode characters in the text.
 */
size_t VuoText_length(const VuoText text)
{
	if (! text)
		return 0;

	CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
	if (!s)
		return 0;

	size_t length = CFStringGetLength(s);
	CFRelease(s);
	return length;
}

/**
 * Returns the number of bytes in the text, not including the null terminator.
 */
size_t VuoText_byteCount(const VuoText text)
{
	if (! text)
		return 0;

	return strlen(text);
}

/**
 * Returns true if `text` is empty (is NULL or is non-NULL with zero length).
 */
bool VuoText_isEmpty(const VuoText text)
{
	return !text || text[0] == 0;
}

/**
 * Returns true if `text` is not empty.
 */
bool VuoText_isPopulated(const VuoText text)
{
    return !VuoText_isEmpty(text);
}

/**
 * Returns true if the two texts represent the same Unicode string (even if they use different UTF-8 encodings
 * or Unicode character decompositions).
 *
 * Either or both of the values may be NULL.  NULL values are not equal to any string (including emptystring).
 */
bool VuoText_areEqual(const VuoText text1, const VuoText text2)
{
	if (text1 == text2)
		return true;

	if (! text1 || ! text2)
		return (! text1 && ! text2);

	CFStringRef s1 = CFStringCreateWithCString(kCFAllocatorDefault, text1, kCFStringEncodingUTF8);
	if (!s1)
		return false;

	CFStringRef s2 = CFStringCreateWithCString(kCFAllocatorDefault, text2, kCFStringEncodingUTF8);
	if (!s2)
	{
		CFRelease(s1);
		return false;
	}

	CFComparisonResult result = CFStringCompare(s1, s2, kCFCompareNonliteral | kCFCompareWidthInsensitive);

	CFRelease(s1);
	CFRelease(s2);
	return (result == kCFCompareEqualTo);
}

/**
 * Helper for `VuoText_isLessThan*()`.
 */
static bool isLessThan(const VuoText text1, const VuoText text2, CFStringCompareFlags flags)
{
	// Treat null text as greater than non-null text,
	// so the more useful non-null text sorts to the beginning of the list.
	if (! text1 || ! text2)
		return text1 && !text2;

	CFStringRef s1 = CFStringCreateWithCString(kCFAllocatorDefault, text1, kCFStringEncodingUTF8);
	if (!s1)
		return false;

	CFStringRef s2 = CFStringCreateWithCString(kCFAllocatorDefault, text2, kCFStringEncodingUTF8);
	if (!s2)
	{
		CFRelease(s1);
		return false;
	}

	CFComparisonResult result = CFStringCompare(s1, s2, kCFCompareNonliteral | kCFCompareWidthInsensitive | flags);

	CFRelease(s1);
	CFRelease(s2);
	return (result == kCFCompareLessThan);
}

/**
 * @ingroup VuoText
 * Returns true if @a text1 is ordered before @a text2 in a case-sensitive lexicographic ordering (which treats
 * different UTF-8 encodings and Unicode character decompositions as equivalent).
 */
bool VuoText_isLessThan(const VuoText text1, const VuoText text2)
{
	return isLessThan(text1, text2, 0);
}

/**
 * @ingroup VuoText
 * Returns true if @a text1 is ordered before @a text2 in a case-insensitive lexicographic ordering (which treats
 * different UTF-8 encodings and Unicode character decompositions as equivalent).
 */
bool VuoText_isLessThanCaseInsensitive(const VuoText text1, const VuoText text2)
{
	return isLessThan(text1, text2, kCFCompareCaseInsensitive);
}

/**
 * @ingroup VuoText
 * Returns true if the number in @a text1 is less than the number in @a text2.
 */
bool VuoText_isLessThanNumeric(const VuoText text1, const VuoText text2)
{
	VuoReal real1 = (text1 ? VuoReal_makeFromString(text1) : 0);
	VuoReal real2 = (text2 ? VuoReal_makeFromString(text2) : 0);
	return real1 < real2;
}

/**
 * @ingroup VuoText
 * Returns true if @a text1 matches @a text2 based on @a comparison.
 *
 * @a text1 is the subject of the comparison and @a text2 is the object. For example, if the comparison is
 * "begins with", this function checks if @a text1 begins with @a text2.
 */
bool VuoText_compare(VuoText text1, VuoTextComparison comparison, VuoText text2)
{
	if (! comparison.isCaseSensitive)
	{
		text1 = VuoText_changeCase(text1, VuoTextCase_LowercaseAll);
		text2 = VuoText_changeCase(text2, VuoTextCase_LowercaseAll);
	}

	bool match = false;
	if (comparison.type == VuoTextComparison_Equals)
	{
		match = VuoText_areEqual(text1, text2);
	}
	else if (comparison.type == VuoTextComparison_Contains)
	{
		match = (VuoText_findFirstOccurrence(text1, text2, 1) > 0);
	}
	else if (comparison.type == VuoTextComparison_BeginsWith || comparison.type == VuoTextComparison_EndsWith)
	{
		size_t aLength = VuoText_length(text1);
		size_t bLength = VuoText_length(text2);

		if (bLength == 0)
		{
			match = true;
		}
		else
		{
			int startIndex = (comparison.type == VuoTextComparison_BeginsWith ? 1 : aLength - bLength + 1);
			VuoText aSub = VuoText_substring(text1, startIndex, bLength);
			match = VuoText_areEqual(aSub, text2);

			VuoRetain(aSub);
			VuoRelease(aSub);
		}
	}

	else if (comparison.type == VuoTextComparison_MatchesWildcard)
	{
		bool t1empty = VuoText_isEmpty(text1);
		bool t2empty = VuoText_isEmpty(text2);
		if (t2empty)
			return t1empty == t2empty;

		locale_t locale = newlocale(LC_ALL_MASK, "en_US.UTF-8", NULL);
		locale_t oldLocale = uselocale(locale);
		if (oldLocale != LC_GLOBAL_LOCALE)
			freelocale(oldLocale);

		match = fnmatch(text2, text1, 0) != FNM_NOMATCH;
	}

	else if (comparison.type == VuoTextComparison_MatchesRegEx)
	{
		bool t1empty = VuoText_isEmpty(text1);
		bool t2empty = VuoText_isEmpty(text2);
		if (t2empty)
			return t1empty == t2empty;

		regex_t re;
		int ret = regcomp(&re, text2, REG_EXTENDED);
		if (ret)
		{
			char errstr[256];
			regerror(ret, &re, errstr, sizeof(errstr));
			VUserLog("Error compiling regular expression: %s", errstr);
			return false;
		}

		ret = regexec(&re, text1, 0, NULL, 0);
		if (ret == 0)
			match = true;
		else if (ret == REG_NOMATCH)
			match = false;
		else
		{
			char errstr[256];
			regerror(ret, &re, errstr, sizeof(errstr));
			VUserLog("Error executing regular expression: %s", errstr);
		}
		regfree(&re);
	}

	if (! comparison.isCaseSensitive)
	{
		VuoRetain(text1);
		VuoRelease(text1);
		VuoRetain(text2);
		VuoRelease(text2);
	}

	return match;
}

/**
 * @ingroup VuoText
 * Returns the index (starting at 1) of the first instance of @a substring in @a string
 * at index >= startIndex. Returns 0 if @a substring is not found.
 *
 * This function will find occurrences that consist of the same Unicode characters as @a substring, but won't find
 * occurrences that consist of the same Unicode string decomposed into a different number of Unicode characters.
 */
size_t VuoText_findFirstOccurrence(const VuoText string, const VuoText substring, const size_t startIndex)
{
	if (VuoText_isEmpty(substring))
		return 1;

	if (! string)
		return 0;

	size_t stringLength = VuoText_length(string);
	size_t substringLength = VuoText_length(substring);
	if (stringLength < substringLength)
		return 0;

	for (size_t i = startIndex; i <= stringLength - substringLength + 1; ++i)
	{
		VuoText currSubstring = VuoText_substring(string, i, substringLength);
		bool found = VuoText_areEqual(substring, currSubstring);
		VuoRetain(currSubstring);
		VuoRelease(currSubstring);
		if (found)
			return i;
	}

	return 0;
}

/**
 * @ingroup VuoText
 * Returns the index (starting at 1) of the last instance of @a substring in @a string.
 * Returns 0 if @a substring is not found.
 *
 * This function will find occurrences that consist of the same Unicode characters as @a substring, but won't find
 * occurrences that consist of the same Unicode string decomposed into a different number of Unicode characters.
 */
size_t VuoText_findLastOccurrence(const VuoText string, const VuoText substring)
{
	if (VuoText_isEmpty(substring))
	{
		if (string)
			return VuoText_length(string) + 1;
		else
			return 1;
	}

	if (! string)
		return 0;

	size_t foundIndex = 0;

	size_t stringLength = VuoText_length(string);
	size_t substringLength = VuoText_length(substring);
	if (stringLength < substringLength)
		return 0;

	for (size_t i = 1; i <= stringLength - substringLength + 1; ++i)
	{
		VuoText currSubstring = VuoText_substring(string, i, substringLength);
		if (VuoText_areEqual(substring, currSubstring))
			foundIndex = i;
		VuoRetain(currSubstring);
		VuoRelease(currSubstring);
	}

	return foundIndex;
}

/**
 * Returns a list containing all occurrences of `substring` in `string`.
 *
 * This function will find occurrences that consist of the same Unicode characters as @a substring, but won't find
 * occurrences that consist of the same Unicode string decomposed into a different number of Unicode characters.
 */
VuoList_VuoInteger VuoText_findOccurrences(const VuoText string, const VuoText substring)
{
	if (VuoText_isEmpty(string) || VuoText_isEmpty(substring))
		return NULL;

	size_t stringLength = VuoText_length(string);
	size_t substringLength = VuoText_length(substring);
	if (stringLength < substringLength)
		return 0;

	VuoList_VuoInteger found = VuoListCreate_VuoInteger();
	for (size_t i = 1; i <= stringLength - substringLength + 1; ++i)
	{
		VuoText currSubstring = VuoText_substring(string, i, substringLength);
		VuoLocal(currSubstring);
		if (VuoText_areEqual(substring, currSubstring))
			VuoListAppendValue_VuoInteger(found, i);
	}

	return found;
}

/**
 * @ingroup VuoText
 * Returns the substring of @c string starting at index @c startIndex and spanning @c length Unicode characters.
 *
 * @c startIndex is indexed from 1, not 0.
 *
 * If @a startIndex is past the end of @a string, returns the empty string.
 *
 * If @a startIndex is before the beginning of @a string, deducts the number of characters before
 * the beginning from @a length, and returns characters starting at the beginning of @a string.
 *
 * If @a string has fewer than @a length characters from @a startIndex to the end of @a string,
 * returns all characters from @a startIndex to the end of @a string.
 */
VuoText VuoText_substring(const VuoText string, int startIndex, int length)
{
	if (! string)
		return VuoText_make("");

	int originalLength = VuoText_length(string);
	if (startIndex > originalLength)
		return VuoText_make("");

	if (startIndex < 1)
	{
		length -= 1 - startIndex;
		startIndex = 1;
	}

	if (length < 0)
		return VuoText_make("");

	if (startIndex + length - 1 > originalLength)
		length = originalLength - startIndex + 1;

	size_t startIndexFromZero = startIndex - 1;

	CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, string, kCFStringEncodingUTF8);
	if (!s)
		return VuoText_make("");

	CFStringRef ss = CFStringCreateWithSubstring(kCFAllocatorDefault, s, CFRangeMake(startIndexFromZero, length));
	if (!ss)
		return VuoText_make("");

	VuoText substring = VuoText_makeFromCFString(ss);

	CFRelease(s);
	CFRelease(ss);
	return substring;
}

/**
 * @ingroup VuoText
 * Returns a string consisting of the elements in the @c texts array concatenated together.
 */
VuoText VuoText_append(VuoText *texts, size_t textsCount)
{
	if (!textsCount)
		return NULL;

	CFMutableArrayRef a = CFArrayCreateMutable(kCFAllocatorDefault, textsCount, &kCFTypeArrayCallBacks);
	for (size_t i = 0; i < textsCount; ++i)
	{
		if (texts[i])
		{
			CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, texts[i], kCFStringEncodingUTF8);
			if (!s)
				continue;
			CFArrayAppendValue(a, s);
			CFRelease(s);
		}
	}

	CFStringRef s = CFStringCreateByCombiningStrings(kCFAllocatorDefault, a, CFSTR(""));
	VuoText compositeString = VuoText_makeFromCFString(s);

	CFRelease(s);
	CFRelease(a);
	return compositeString;
}

/**
 * @ingroup VuoText
 * Returns a string consisting of the elements in `texts` array concatenated together, with `separator` between them.
 */
VuoText VuoText_appendWithSeparator(VuoList_VuoText texts, VuoText separator, bool includeEmptyParts)
{
	unsigned long textsCount = VuoListGetCount_VuoText(texts);
	if (!textsCount)
		return NULL;

	VuoText *textsArray = (VuoText *)malloc((textsCount * 2 - 1) * sizeof(VuoText));
	unsigned long outputIndex = 0;
	bool previousText = false;
	for (unsigned long inputIndex = 1; inputIndex <= textsCount; ++inputIndex)
	{
		VuoText t = VuoListGetValue_VuoText(texts, inputIndex);
		if (includeEmptyParts)
		{
			textsArray[outputIndex++] = t;
			if (inputIndex < textsCount)
				textsArray[outputIndex++] = separator;
		}
		else if (VuoText_isPopulated(t))
		{
			if (previousText && inputIndex <= textsCount)
				textsArray[outputIndex++] = separator;
			textsArray[outputIndex++] = t;
			previousText = true;
		}
	}

	VuoText compositeText = VuoText_append(textsArray, outputIndex);

	free(textsArray);

	return compositeText;
}

/**
 * @ingroup VuoText
 * Splits @a text into parts (basically the inverse of VuoText_append()).
 */
VuoText * VuoText_split(VuoText text, VuoText separator, bool includeEmptyParts, size_t *partsCount)
{
	if (!text || !separator)
		return NULL;

	CFMutableArrayRef splitTexts = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

	CFStringRef textCF = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
	if (!textCF)
		return NULL;
	VuoDefer(^{ CFRelease(textCF); });

	size_t textLength = CFStringGetLength(textCF);

	CFStringRef separatorCF = CFStringCreateWithCString(kCFAllocatorDefault, separator, kCFStringEncodingUTF8);
	if (!separatorCF)
		return NULL;
	VuoDefer(^{ CFRelease(separatorCF); });

	size_t separatorLength = CFStringGetLength(separatorCF);

	if (separatorLength > 0)
	{
		size_t startIndex = 1;
		size_t separatorIndex = 0;

		while (startIndex <= textLength)
		{
			CFRange rangeToSearch = CFRangeMake(startIndex - 1, textLength - (startIndex - 1));
			CFRange foundRange;
			Boolean found = CFStringFindWithOptions(textCF, separatorCF, rangeToSearch, 0, &foundRange);
			separatorIndex = foundRange.location + 1;
			if (!found)
				separatorIndex = textLength + 1;

			if (separatorIndex > startIndex || includeEmptyParts)
			{
				CFStringRef partStr = CFStringCreateWithSubstring(kCFAllocatorDefault, textCF, CFRangeMake(startIndex - 1, separatorIndex - startIndex));
				if (partStr)
				{
					CFArrayAppendValue(splitTexts, partStr);
					CFRelease(partStr);
				}
			}

			startIndex = separatorIndex + separatorLength;
		}

		if (includeEmptyParts && textLength > 0 && separatorIndex + separatorLength - 1 == textLength)
		{
			CFStringRef emptyPartStr = CFStringCreateWithCString(kCFAllocatorDefault, "", kCFStringEncodingUTF8);
			if (emptyPartStr)
			{
				CFArrayAppendValue(splitTexts, emptyPartStr);
				CFRelease(emptyPartStr);
			}
		}
	}
	else
	{
		for (size_t i = 1; i <= textLength; ++i)
		{
			CFStringRef partStr = CFStringCreateWithSubstring(kCFAllocatorDefault, textCF, CFRangeMake(i - 1, 1));
			if (partStr)
			{
				CFArrayAppendValue(splitTexts, partStr);
				CFRelease(partStr);
			}
		}
	}

	*partsCount = CFArrayGetCount(splitTexts);
	VuoText *splitTextsArr = (VuoText *)malloc(*partsCount * sizeof(VuoText));
	for (size_t i = 0; i < *partsCount; ++i)
	{
		CFStringRef part = CFArrayGetValueAtIndex(splitTexts, i);
		splitTextsArr[i] = VuoText_makeFromCFString(part);
	}
	CFRelease(splitTexts);

	return splitTextsArr;
}

/**
 * Returns a new string in which each occurrence of @c stringToFind in @c subject has been replaced with @c replacement.
 *
 * @c stringToFind matches even if a different UTF-8 encoding or Unicode character decomposition is used.
 */
VuoText VuoText_replace(VuoText subject, VuoText stringToFind, VuoText replacement)
{
	if (!subject)
		return NULL;
	if (!stringToFind)
		return subject;

	CFMutableStringRef subjectCF = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(subjectCF, subject, kCFStringEncodingUTF8);

	CFStringRef stringToFindCF = CFStringCreateWithCString(NULL, stringToFind, kCFStringEncodingUTF8);
	if (!stringToFindCF)
	{
		CFRelease(subjectCF);
		return subject;
	}

	CFStringRef replacementCF = nil;
	if (replacement)
	{
		replacementCF = CFStringCreateWithCString(NULL, replacement, kCFStringEncodingUTF8);
		if (!replacementCF)
		{
			CFRelease(stringToFindCF);
			CFRelease(subjectCF);
			return subject;
		}
	}

	CFStringFindAndReplace(subjectCF, stringToFindCF, replacementCF, CFRangeMake(0,CFStringGetLength(subjectCF)), kCFCompareNonliteral);

	VuoText replacedSubject = VuoText_makeFromCFString(subjectCF);
	if (replacementCF)
		CFRelease(replacementCF);
	CFRelease(stringToFindCF);
	CFRelease(subjectCF);

	return replacedSubject;
}

/**
 * Returns a new string with @c newText inserted at the @c startIndex.
 * @c startIndex is 1 indexed, not 0.
 * If @c startIndex is less than 1, @c newText is inserted at the beginning of @c string.
 * If @c startIndex is greater than @c string length, @c newText is appended to the ending of @c string.
 */
VuoText VuoText_insert(const VuoText string, int startIndex, const VuoText newText)
{
	if (!newText)
		return string;
	if (!string)
		return newText;

	int len = VuoText_length(string);

	if(startIndex > len) {
		const char *append[2] = { string, newText };
		return VuoText_append(append, 2);
	} else if(startIndex <= 1) {
		const char *append[2] = { newText, string };
		return VuoText_append(append, 2);
	}

	VuoText left = VuoText_substring(string, 1, startIndex - 1);
	VuoText right = VuoText_substring(string, startIndex, (len + 1) - startIndex);

	VuoLocal(left);
	VuoLocal(right);

	const char *append[3] = { left, newText, right };
	return VuoText_append(append, 3);
}

/**
 * Returns a new string where characters from @c startIndex to @c startIndex + @c length are removed.
 *
 * @c startIndex is 1 indexed, not 0.
 */
VuoText VuoText_removeAt(const VuoText string, int startIndex, int length)
{
	int len = VuoText_length(string);

	if(startIndex < 1)
	{
		length -= (1 - startIndex);
		startIndex = 1;
	}

	// if start is greater than original length or start + len is the whole array
	if(startIndex > len || length < 1)
		return VuoText_make(string);

	VuoText left = VuoText_substring(string, 1, startIndex - 1);
	VuoText right = VuoText_substring(string, startIndex + length, (len + 1) - startIndex);

	VuoLocal(left);
	VuoLocal(right);

	return VuoText_make(VuoText_format("%s%s", left, right));
}

/**
 * Returns a new string formatted using the printf-style `format` string.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoText_format(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int size = vsnprintf(NULL, 0, format, args);
	va_end(args);

	char *formattedString = (char *)malloc(size+1);
	va_start(args, format);
	vsnprintf(formattedString, size+1, format, args);
	va_end(args);

	return formattedString;
}

/**
 * Returns a new string consisting of `text` without the whitespace at the beginning and end.
 *
 * This function trims ASCII spaces, tabs, and linebreaks, but not other Unicode whitespace characters.
 *
 * @see VuoStringUtilities::trim
 */
VuoText VuoText_trim(const VuoText text)
{
	if (!text)
		return NULL;

	size_t len = strlen(text);
	size_t firstNonSpace;
	for (firstNonSpace = 0; firstNonSpace < len && isspace(text[firstNonSpace]); ++firstNonSpace);

	if (firstNonSpace == len)
		return VuoText_make("");

	size_t lastNonSpace;
	for (lastNonSpace = len-1; lastNonSpace > firstNonSpace && isspace(text[lastNonSpace]); --lastNonSpace);

	return VuoText_makeWithMaxLength(text + firstNonSpace, lastNonSpace - firstNonSpace + 1);
}

/**
 * Returns true if all byte values in `text` are between 0 and 127.
 */
static bool VuoText_isASCII7(VuoText text)
{
	size_t len = strlen(text);
	for (size_t i = 0; i < len; ++i)
		if (((unsigned char *)text)[i] > 127)
			return false;

	return true;
}

/**
 * Returns a new string with the @c text characters cased in the @c textCase style.
 */
VuoText VuoText_changeCase(const VuoText text, VuoTextCase textCase)
{
	if (!text)
		return NULL;

	// Optimized conversion for plain ASCII7 text.
	if (VuoText_isASCII7(text))
	{
		if (textCase == VuoTextCase_LowercaseAll)
		{
			size_t len = strlen(text);
			char *processedString = malloc(len + 1);
			for (size_t i = 0; i < len; ++i)
				processedString[i] = tolower(text[i]);
			processedString[len] = 0;
			VuoRegister(processedString, free);
			return processedString;
		}
	}

	CFMutableStringRef mutable_str = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(mutable_str, text, kCFStringEncodingUTF8);
	CFLocaleRef locale = CFLocaleCopyCurrent();

	switch( textCase )
	{
		case VuoTextCase_LowercaseAll:
			CFStringLowercase(mutable_str, locale);
			break;

		case VuoTextCase_UppercaseAll:
			CFStringUppercase(mutable_str, locale);
			break;

		case VuoTextCase_UppercaseFirstLetterWord:
			CFStringCapitalize(mutable_str, locale);
			break;

		case VuoTextCase_UppercaseFirstLetterSentence:
		{
			// The rest of the string functions lower-case everything by default
			CFStringLowercase(mutable_str, locale);

			// the sentence tokenizer does a better job when all characters are capitalized
			CFStringRef tmp = CFStringCreateWithSubstring(kCFAllocatorDefault, mutable_str, CFRangeMake(0, CFStringGetLength(mutable_str)));
			CFMutableStringRef all_upper = CFStringCreateMutableCopy(NULL, 0, tmp);
			CFRelease(tmp);
			CFStringUppercase(all_upper, locale);

			CFStringTokenizerRef tokenizer = CFStringTokenizerCreate( 	kCFAllocatorDefault,
																		all_upper,
																		CFRangeMake(0, CFStringGetLength(all_upper)),
																		kCFStringTokenizerUnitSentence,
																		locale);

			CFStringTokenizerTokenType tokenType = kCFStringTokenizerTokenNone;

			// https://stackoverflow.com/questions/15515128/capitalize-first-letter-of-every-sentence-in-nsstring
			while(kCFStringTokenizerTokenNone != (tokenType = CFStringTokenizerAdvanceToNextToken(tokenizer)))
			{
				CFRange tokenRange = CFStringTokenizerGetCurrentTokenRange(tokenizer);

				if (tokenRange.location != kCFNotFound && tokenRange.length > 0)
				{
					CFRange firstCharRange = CFRangeMake(tokenRange.location, 1);
					CFStringRef firstLetter = CFStringCreateWithSubstring(kCFAllocatorDefault, mutable_str, firstCharRange);
					CFMutableStringRef upperFirst = CFStringCreateMutableCopy(NULL, 0, firstLetter);
					CFRelease(firstLetter);
					CFStringCapitalize(upperFirst, locale);
					CFStringReplace(mutable_str, firstCharRange, upperFirst);
					CFRelease(upperFirst);
				}
			}

			CFRelease(all_upper);
			CFRelease(tokenizer);
		}
		break;
	}

	VuoText processedString = VuoText_makeFromCFString(mutable_str);

	CFRelease(locale);
	CFRelease(mutable_str);

	return processedString;
}

/**
 * Returns an array of unicode 32 bit decimal values for each character in a string.
 *
 * If conversion fails, @c length will be set to 0 null is returned.  Otherwise an
 * array is returned and @c length is set to the size of the array. Caller is
 * responsible for freeing the returned array.
 *
 * \sa VuoText_makeFromUtf32
 */
uint32_t* VuoText_getUtf32Values(const VuoText text, size_t* length)
{
	CFMutableStringRef cf_str = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(cf_str, text, kCFStringEncodingUTF8);

	size_t str_len = CFStringGetLength(cf_str);

	CFRange range = CFRangeMake(0, str_len);
	CFIndex usedBufLen;
	*length = (size_t) CFStringGetBytes(cf_str, range, kCFStringEncodingUTF32, '?', false, NULL, str_len, &usedBufLen );

	uint32_t* decimal = (uint32_t*) NULL;

	if(*length > 0)
	{
		decimal = (uint32_t*) malloc( sizeof(uint32_t) * usedBufLen );
		CFStringGetBytes(cf_str, range, kCFStringEncodingUTF32, '?', false, (uint8_t*) decimal, usedBufLen, NULL);
	}

	CFRelease(cf_str);

	return decimal;
}
