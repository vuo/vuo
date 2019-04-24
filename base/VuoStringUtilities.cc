/**
 * @file
 * VuoStringUtilities implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoStringUtilities.hh"

#if defined(__x86_64__) || defined(DOXYGEN)
extern "C" {
#include "mkdio.h"
}
#endif

/**
 * Returns true if @c wholeString begins with @c beginning.
 */
bool VuoStringUtilities::beginsWith(string wholeString, string beginning)
{
	return wholeString.length() >= beginning.length() && wholeString.substr(0, beginning.length()) == beginning;
}

/**
 * Returns true if @c wholeString ends with @c ending.
 */
bool VuoStringUtilities::endsWith(string wholeString, string ending)
{
	if (wholeString.length() < ending.length())
		return false;

	return wholeString.compare(wholeString.length()-ending.length(), ending.length(), ending) == 0;
}

/**
 * Returns the substring of @c wholeString that follows @c beginning,
 * or an empty string if @c wholeString does not begin with @c beginning.
 */
string VuoStringUtilities::substrAfter(string wholeString, string beginning)
{
	if (! beginsWith(wholeString, beginning))
		return "";

	return wholeString.substr(beginning.length());
}

/**
 * Returns the substring of @c wholeString that precedes @c ending,
 * or an empty string if @c wholeString does not end with @c ending.
 */
string VuoStringUtilities::substrBefore(string wholeString, string ending)
{
	if (! endsWith(wholeString, ending))
		return "";

	return wholeString.substr(0, wholeString.length()-ending.length());
}

/**
 * Returns a string constructed by replacing all instances of @c originalChar with
 * @c replacementChar in @c wholeString.
 */
string VuoStringUtilities::replaceAll(string wholeString, char originalChar, char replacementChar)
{
	string outString = wholeString;
	size_t pos = 0;
	while ((pos = wholeString.find_first_of(originalChar, pos)) != string::npos)
	{
		outString[pos] = replacementChar;
		pos = pos + 1;
	}
	return outString;
}

/**
 * Replaces all instances of @a originalSubstring with @a replacementSubstring in @a wholeString.
 *
 * Returns the number of instances replaced.
 */
size_t VuoStringUtilities::replaceAll(string &wholeString, string originalSubstring, string replacementSubstring)
{
	size_t replacementCount = 0;
	size_t startPos = 0;
	while ((startPos = wholeString.find(originalSubstring, startPos)) != string::npos)
	{
		wholeString.replace(startPos, originalSubstring.length(), replacementSubstring);
		startPos += replacementSubstring.length();
		++replacementCount;
	}
	return replacementCount;
}

/**
 * Splits @a wholeString into parts, as separated by @a delimiter.
 */
vector<string> VuoStringUtilities::split(const string &wholeString, char delimiter)
{
	vector<string> tokens;
	istringstream iss(wholeString);
	string token;
	while( getline(iss, token, delimiter) )
		tokens.push_back(token);
	return tokens;
}

/**
 * Combines @a partialStrings, separated by @a delimiter, into one string.
 */
string VuoStringUtilities::join(vector<string> partialStrings, char delimiter)
{
	string wholeString;
	for (vector<string>::iterator i = partialStrings.begin(); i != partialStrings.end(); )
	{
		wholeString += *i;
		if (++i != partialStrings.end())
			wholeString += delimiter;
	}
	return wholeString;
}

/**
 * Returns a string that starts with @a moduleKey and ends with @a symbolName.
 * Useful for adding a unique prefix to a symbol name.
 */
string VuoStringUtilities::prefixSymbolName(string symbolName, string moduleKey)
{
	return transcodeToIdentifier(moduleKey) + "__" + symbolName;
}

/**
 * Transforms a string into a valid identifier:
 *  - Replaces whitespace and '.'s with '_'s
 *  - Strips out characters not in [A-Za-z0-9_]
 */
string VuoStringUtilities::transcodeToIdentifier(string str)
{
	CFMutableStringRef strCF = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(strCF, str.c_str(), kCFStringEncodingUTF8);

	CFStringNormalize(strCF, kCFStringNormalizationFormD);  // decomposes combining characters, so accents/diacritics are separated from their letters

	CFStringRef empty = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8);
	CFStringRef underscore = CFStringCreateWithCString(NULL, "_", kCFStringEncodingUTF8);

	CFIndex strLength = CFStringGetLength(strCF);
	UniChar *strBuf = (UniChar *)malloc(strLength * sizeof(UniChar));
	CFStringGetCharacters(strCF, CFRangeMake(0, strLength), strBuf);

	for (CFIndex i = strLength-1; i >= 0; --i)
	{
		UniChar c = strBuf[i];

		CFStringRef replacement = NULL;
		if (c > 127)  // non-ASCII
			replacement = empty;
		else if (c == '.' || isspace(c))
			replacement = underscore;
		else if (! isValidCharInIdentifier(c))
			replacement = empty;

		if (replacement)
			CFStringReplace(strCF, CFRangeMake(i, 1), replacement);
	}

	// http://stackoverflow.com/questions/1609565/whats-the-cfstring-equiv-of-nsstrings-utf8string

	const char *useUTF8StringPtr = NULL;
	char *freeUTF8StringPtr = NULL;

	if ((useUTF8StringPtr = CFStringGetCStringPtr(strCF, kCFStringEncodingUTF8)) == NULL)
	{
		CFIndex maxBytes = 4 * strLength + 1;
		freeUTF8StringPtr = (char *)malloc(maxBytes);
		CFStringGetCString(strCF, freeUTF8StringPtr, maxBytes, kCFStringEncodingUTF8);
		useUTF8StringPtr = freeUTF8StringPtr;
	}

	string ret = useUTF8StringPtr;

	if (freeUTF8StringPtr != NULL)
		free(freeUTF8StringPtr);

	CFRelease(strCF);
	CFRelease(empty);
	CFRelease(underscore);
	free(strBuf);

	return ret;
}

/**
 * Check whether a character is valid for an
 * identifier, i.e., matches [A-Za-z0-9_]
 */
bool VuoStringUtilities::isValidCharInIdentifier(char ch)
{
	bool valid = (isalnum(ch) || ch=='_');
	return valid;
}

/**
 * Escapes backslashes, quotes, curly braces, angle brackets, and pipes in the string, making it a valid identifier in the Graphviz DOT language.
 */
string VuoStringUtilities::transcodeToGraphvizIdentifier(const string &originalString)
{
	string escapedString = originalString;
	for (string::size_type i = 0; (i = escapedString.find("\\", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\\");
	for (string::size_type i = 0; (i = escapedString.find("\"", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\"");
	for (string::size_type i = 0; (i = escapedString.find("{", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\{");
	for (string::size_type i = 0; (i = escapedString.find("}", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\}");
	for (string::size_type i = 0; (i = escapedString.find("<", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\<");
	for (string::size_type i = 0; (i = escapedString.find(">", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\>");
	for (string::size_type i = 0; (i = escapedString.find("|", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\|");
	return escapedString;
}

/**
 * Removes escapes for backslashes, quotes, curly braces, angle brackets, and pipes in the Graphviz identifier, making it a normal string.
 */
string VuoStringUtilities::transcodeFromGraphvizIdentifier(const string &graphvizIdentifier)
{
	string unescapedString;
	bool inEscape = false;
	for (string::const_iterator i = graphvizIdentifier.begin(); i != graphvizIdentifier.end(); ++i)
	{
		if (inEscape)
		{
			inEscape = false;
			unescapedString += *i;
			continue;
		}

		if (*i == '\\')
		{
			inEscape = true;
			continue;
		}

		unescapedString  += *i;
	}
	return unescapedString;
}

#if defined(__x86_64__) || defined(DOXYGEN)
/**
 * Converts @c markdownString (a Markdown document) to HTML.
 *
 * The returned HTML includes a paragraph wrapper around each line of text.
 */
string VuoStringUtilities::generateHtmlFromMarkdown(const string &markdownString)
{
	MMIOT *doc = mkd_string(markdownString.c_str(), markdownString.length(), MKD_NOPANTS);
	mkd_compile(doc, 0);
	char *html;
	mkd_document(doc, &html);
	string htmlString(html);

	// Remove the final linebreak from code blocks,
	// since Qt (unlike typical browser rendering engines) considers that whitespace significant.
	replaceAll(htmlString, "\n</code></pre>", "</code></pre>");

	return htmlString;
}

/**
 * Converts @c markdownString (a single line of Markdown text) to HTML.
 *
 * The returned HTML does not include a paragraph wrapper.
 */
string VuoStringUtilities::generateHtmlFromMarkdownLine(const string &markdownString)
{
	size_t length = markdownString.length();
	if (!length)
		return "";

	char *html;
	mkd_line((char *)markdownString.c_str(), length, &html, MKD_NOPANTS);
	string htmlString(html);
	return htmlString;
}
#endif

/**
 * Inserts spaces at CamelCase transitions within the input `camelCaseString`,
 * capitalizes the first letter of the string, and returns the result.
 */
string VuoStringUtilities::expandCamelCase(string camelCaseString)
{
	string out;
	out += toupper(camelCaseString[0]);

	size_t length = camelCaseString.length();
	for (int i = 1; i < length; ++i)
	{
		char c = camelCaseString[i];
		if (isupper(c) || (isdigit(c) && !isdigit(camelCaseString[i-1])))
			out += " ";
		out += c;
	}

	return out;
}

/**
 * Returns a random sequence of alphanumeric characters.
 */
string VuoStringUtilities::makeRandomHash(int length)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	string hash(length, 0);
	for (int i = 0; i < length; ++i)
		hash[i] = alphanum[arc4random_uniform(sizeof(alphanum)-1)];

	return hash;
}

const std::locale VuoStringUtilities::locale;
const std::collate<char> &VuoStringUtilities::collate = std::use_facet<std::collate<char> >(VuoStringUtilities::locale);
