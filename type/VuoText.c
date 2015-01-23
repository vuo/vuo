/**
 * @file
 * VuoText implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoText.h"
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

	#include <unicode/utext.h>
	#include <unicode/ubrk.h>

#pragma clang diagnostic pop


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text",
					 "description" : "A Unicode (UTF-8) text string.",
					 "keywords" : [ "char *", "character" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c",
						 "icuuc",
						 "icudata"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoText
 * The stpncpy function is not provided by Mac OS X implementation of <string.h>.
 * Rather than adding a dependency, just define it here.
 * This is getting replaced anyway with Unicode-aware code.
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/strncpy.html
 */
char *VuoText_stpncpy(char *dest, const char *src, size_t n)
{
	char * sp = strncpy(dest,src,n);
	if (strlen(src) < n)
		return sp + strlen(src);
	return sp + n;
}

/**
 * @ingroup VuoText
 * Decodes the JSON object @c js, expected to contain a string, to create a new value.
 */
VuoText VuoText_valueFromJson(json_object * js)
{
	const char *textString = "";
	if (json_object_get_type(js) == json_type_string)
		textString = json_object_get_string(js);
	return VuoText_make(textString);
}

/**
 * @ingroup VuoText
 * Encodes @c value as a JSON object.
 */
json_object * VuoText_jsonFromValue(const VuoText value)
{
	if (!value)
		return json_object_new_string("");

	return json_object_new_string(value);
}

/**
 * @ingroup VuoText
 * Creates a new UTF-8 C string from @c value, or, if it's more than 32 characters long, creates an aposiopesis.
 *
 * @eg{Hello World!}
 * @eg{I would like to convey my greeti...}
 */
char * VuoText_summaryFromValue(const VuoText value)
{
	if (!value)
		return strdup("");

	/// @todo handle unicode
	char * summary = (char *)malloc(32+3+1);

	char * sp = summary;
	sp = VuoText_stpncpy(sp,value,32);

	// Add ellipsis if trimmed.
	if (strlen(value) > 32)
	{
		*sp++ = '.';
		*sp++ = '.';
		*sp++ = '.';
	}

	*sp++ = 0;
	return summary;
}

/**
 * @ingroup VuoText
 * Creates a VuoText value from an unquoted string (unlike @c VuoText_valueFromString(), which expects a quoted string).
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
 * @ingroup VuoText
 * Returns the byte offset of @c logicalCharIndex into @c text's UTF-8 representation.
 */
size_t VuoText_convertUTF8IndexToByteIndex(UText *text, int logicalCharIndex)
{
	UErrorCode status = U_ZERO_ERROR;
	UBreakIterator *bi = ubrk_open(UBRK_CHARACTER, uloc_getDefault(), NULL, 0, &status);
	ubrk_setUText(bi, text, &status);

	int curLogicalCharIndex = 0;
	int curNativeCharIndex = 0;
	while((curLogicalCharIndex < logicalCharIndex) && (curNativeCharIndex != UBRK_DONE))
	{
		curNativeCharIndex = ubrk_next(bi);
		curLogicalCharIndex++;
	}

	return curNativeCharIndex;
}

/**
 * @ingroup VuoText
 * Returns the number of UTF-8 characters in the string.
 */
size_t VuoText_length(const VuoText string)
{
	if (!string)
		return 0;

	UErrorCode status = U_ZERO_ERROR;
	UText *text = utext_openUTF8(NULL, string, -1, &status);

	UBreakIterator *bi = ubrk_open(UBRK_CHARACTER, uloc_getDefault(), NULL, 0, &status);
	ubrk_setUText(bi, text, &status);

	int charCount = 0;
	while (ubrk_next(bi) != UBRK_DONE)
		++charCount;

	utext_close(text);

	return charCount;
}

/**
 * @ingroup VuoText
 * Returns the substring of @c string starting at index @c startIndex and spanning @c length UTF-8 characters.
 *
 * @c startIndex is indexed from 1, not 0.
 *
 * If @c startIndex is past the end of the string, returns the empty string.
 *
 * If @c string has fewer than @c length characters from @c startIndex to the end of @c string,
 * returns all characters from @c startIndex to the end of @c string.
 */
VuoText VuoText_substring(const VuoText string, int startIndex, int length)
{
	if (startIndex < 1)
	{
		length -= 1 - startIndex;
		startIndex = 1;
	}

	if (!string || length < 0)
	{
		VuoText text = strdup("");
		VuoRegister(text, free);
		return text;
	}

	UErrorCode status = U_ZERO_ERROR;
	UText *text = utext_openUTF8(NULL, string, -1, &status);

	size_t startIndexFrom0 = startIndex - 1;

	size_t startAsByteIndex = VuoText_convertUTF8IndexToByteIndex(text, startIndexFrom0);
	if (startAsByteIndex > strlen(string) - 1)
		return VuoText_make("");

	size_t endAsByteIndex = VuoText_convertUTF8IndexToByteIndex(text, startIndexFrom0 + length);
	if (endAsByteIndex > strlen(string))
		endAsByteIndex = strlen(string);

	size_t lengthInBytes = endAsByteIndex - startAsByteIndex;
	if (lengthInBytes == 0)
		return VuoText_make("");

	char *substr = (char *)malloc((lengthInBytes + 1) * sizeof(char));
	VuoRegister(substr, free);
	memcpy(substr, string + startAsByteIndex, lengthInBytes);
	substr[lengthInBytes] = '\0';

	utext_close(text);

	return substr;
}

/**
 * @ingroup VuoText
 * Returns a string consisting of the elements in the @c texts array concatenated together.
 */
VuoText VuoText_append(VuoText *texts, unsigned long textsCount)
{
	// Even though the inputs are UTF-8, they're still null-terminated, so we can append them as C strings.

	size_t textsLen = 0;
	for (unsigned long i = 0; i < textsCount; ++i)
		if (texts[i])
			textsLen += strlen(texts[i]);

	char *composite = (char *) malloc(textsLen + 1);

	size_t offset = 0;
	for (unsigned long i = 0; i < textsCount; ++i)
		if (texts[i])
		{
			strcpy(composite + offset, texts[i]);
			offset += strlen(texts[i]);
		}
	composite[textsLen] = 0;

	VuoText compositeText = VuoText_make(composite);
	free(composite);

	return compositeText;
}
