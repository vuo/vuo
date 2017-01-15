/**
 * @file
 * VuoText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <Carbon/Carbon.h>
#include "type.h"
#include "VuoText.h"
#include "VuoData.h"


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text",
					 "description" : "A Unicode (UTF-8) text string.",
					 "keywords" : [ "char *", "character" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"Carbon.framework"
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
VuoText VuoText_truncateWithEllipsis(const VuoText subject, int maxLength)
{
	if (!subject)
		return VuoText_make("");

	VuoText valueWithReturn = VuoText_replace(subject, "\n", "⏎");

	if (VuoText_length(subject) <= maxLength)
		return valueWithReturn;
	else
	{
		VuoText abbreviation = VuoText_substring(valueWithReturn, 1, maxLength);
		VuoText ellipsis = VuoText_make("…");
		VuoText summaryParts[2] = { abbreviation, ellipsis };
		VuoText summaryWhole = VuoText_append(summaryParts, 2);

		VuoRetain(valueWithReturn);
		VuoRelease(valueWithReturn);
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
	VuoText truncatedText = VuoText_truncateWithEllipsis(value, 50);
	VuoRetain(truncatedText);
	VuoText escapedText = VuoText_replace(truncatedText, "<", "&lt;");
	VuoRetain(escapedText);
	VuoRelease(truncatedText);
	char *summary = VuoText_format("<code>%s</code>", escapedText);
	VuoRelease(escapedText);
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
 * This function scans up to `maxLength` bytes in `data`.
 * If it finds a NULL terminator, it returns text up to and including the NULL terminator.
 * If it doesn't find a NULL terminator, it returns `maxLength` characters followed by a NULL terminator.
 *
 * This function does not yet respect multi-byte UTF8 characters.
 * For now, only give it ASCII-7 input data.
 */
VuoText VuoText_makeWithMaxLength(const void *data, const size_t maxLength)
{
	char *text;
	if (data)
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
	CFStringRef cfString = (CFStringRef)cfs;

	// http://stackoverflow.com/questions/1609565/whats-the-cfstring-equiv-of-nsstrings-utf8string

	const char *useUTF8StringPtr = NULL;
	char *freeUTF8StringPtr = NULL;

	if ((useUTF8StringPtr = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8)) == NULL)
	{
		CFIndex stringLength = CFStringGetLength(cfString);
		CFIndex maxBytes = 4 * stringLength + 1;
		freeUTF8StringPtr = malloc(maxBytes);
		CFStringGetCString(cfString, freeUTF8StringPtr, maxBytes, kCFStringEncodingUTF8);
		useUTF8StringPtr = freeUTF8StringPtr;
	}

	VuoText text = VuoText_make(useUTF8StringPtr);

	if (freeUTF8StringPtr != NULL)
		free(freeUTF8StringPtr);

	return text;
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

	CFDataRef cfd = CFDataCreate(NULL, data, size);
	if (!cfd)
		return NULL;

	CFStringRef cfs = CFStringCreateFromExternalRepresentation(NULL, cfd, kCFStringEncodingUTF8);
	if (!cfs)
	{
		CFRelease(cfd);
		return NULL;
	}

	VuoText text = VuoText_makeFromCFString(cfs);
	CFRelease(cfs);
	CFRelease(cfd);

	return text;
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
 * Returns true if the two texts represent the same Unicode string (even if they use different UTF-8 encodings
 * or Unicode character decompositions).
 *
 * Either or both of the values may be NULL.  NULL values are not equal to any string (including emptystring).
 */
bool VuoText_areEqual(const VuoText text1, const VuoText text2)
{
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
 * @ingroup VuoText
 * Returns true if @a text1 is ordered before @a text2 in a case-sensitive lexicographic ordering (which treats
 * different UTF-8 encodings and Unicode character decompositions as equivalent).
 */
bool VuoText_isLessThan(const VuoText text1, const VuoText text2)
{
	if (! text1 || ! text2)
		return text1 && ! text2;

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
	return (result == kCFCompareLessThan);
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
	if (!substring || (substring && substring[0] == 0))
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
	if (!substring || (substring && substring[0] == 0))
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
 * Splits @a text into parts (basically the inverse of VuoText_append()).
 */
VuoText * VuoText_split(VuoText text, VuoText separator, bool includeEmptyParts, size_t *partsCount)
{
	CFMutableArrayRef splitTexts = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	size_t textLength = VuoText_length(text);
	size_t separatorLength = VuoText_length(separator);

	if (separatorLength > 0)
	{
		size_t startIndex = 1;
		size_t separatorIndex = 0;

		while (startIndex <= textLength)
		{
			separatorIndex = VuoText_findFirstOccurrence(text, separator, startIndex);
			if (separatorIndex == 0)
				separatorIndex = textLength + 1;

			if (separatorIndex > startIndex || includeEmptyParts)
			{
				VuoText part = VuoText_substring(text, startIndex, separatorIndex - startIndex);
				CFStringRef partStr = CFStringCreateWithCString(kCFAllocatorDefault, part, kCFStringEncodingUTF8);
				if (partStr)
				{
					CFArrayAppendValue(splitTexts, partStr);
					CFRelease(partStr);
				}
				VuoRetain(part);
				VuoRelease(part);
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
			VuoText part = VuoText_substring(text, i, 1);
			CFStringRef partStr = CFStringCreateWithCString(kCFAllocatorDefault, part, kCFStringEncodingUTF8);
			if (partStr)
			{
				CFArrayAppendValue(splitTexts, partStr);
				CFRelease(partStr);
			}
			VuoRetain(part);
			VuoRelease(part);
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
	CFMutableStringRef subjectCF = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(subjectCF, subject, kCFStringEncodingUTF8);

	CFStringRef stringToFindCF = CFStringCreateWithCString(NULL, stringToFind, kCFStringEncodingUTF8);
	if (!stringToFindCF)
	{
		CFRelease(subjectCF);
		return subject;
	}

	CFStringRef replacementCF = CFStringCreateWithCString(NULL, replacement, kCFStringEncodingUTF8);
	if (!replacementCF)
	{
		CFRelease(stringToFindCF);
		CFRelease(subjectCF);
		return subject;
	}

	CFStringFindAndReplace(subjectCF, stringToFindCF, replacementCF, CFRangeMake(0,CFStringGetLength(subjectCF)), kCFCompareNonliteral);

	VuoText replacedSubject = VuoText_makeFromCFString(subjectCF);
	CFRelease(replacementCF);
	CFRelease(stringToFindCF);
	CFRelease(subjectCF);

	return replacedSubject;
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
