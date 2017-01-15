/**
 * @file
 * VuoStringUtilities implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
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
 *  - Replace whitespace and '.'s with '_'s
 *  - Make sure all characters match [A-Za-z0-9_]
 */
string VuoStringUtilities::transcodeToIdentifier(string str)
{
	string sanitizedStr = str;
	int strLen = sanitizedStr.length();
	for (int i=0; i<strLen; i++) {
		if (sanitizedStr[i] == '.' || isspace(sanitizedStr[i])) {sanitizedStr[i] = '_';}
		else if (!(isValidCharInIdentifier(sanitizedStr[i]))) {
			// For now, replace invalid characters with '0'
			sanitizedStr[i] = '0';
		}
	}
	return sanitizedStr;
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
 * Escapes backslashes, quotes, angle brackets, and pipes in the string, making it a valid identifier in the Graphviz DOT language.
 */
string VuoStringUtilities::transcodeToGraphvizIdentifier(const string &originalString)
{
	string escapedString = originalString;
	for (string::size_type i = 0; (i = escapedString.find("\\", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\\");
	for (string::size_type i = 0; (i = escapedString.find("\"", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\"");
	for (string::size_type i = 0; (i = escapedString.find("<", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\<");
	for (string::size_type i = 0; (i = escapedString.find(">", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\>");
	for (string::size_type i = 0; (i = escapedString.find("|", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\|");
	return escapedString;
}

/**
 * Removes escapes for backslashes, quotes, angle brackets, and pipes in the Graphviz identifier, making it a normal string.
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
		hash[i] = alphanum[arc4random_uniform(sizeof(alphanum))];

	return hash;
}
