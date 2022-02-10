/**
 * @file
 * VuoStringUtilities implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <iomanip>
#include <sstream>
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoException.hh"
#include "VuoStringUtilities.hh"

extern "C" {
#include "mkdio.h"
}

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
	string delimiterStr(1, delimiter);
	return join(partialStrings, delimiterStr);
}

/**
 * Combines @a partialStrings, separated by @a delimiter, into one string.
 */
string VuoStringUtilities::join(vector<string> partialStrings, string delimiter)
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
 * Combines @a partialStrings, separated by @a delimiter, into one string.
 */
string VuoStringUtilities::join(set<string> partialStrings, string delimiter)
{
	string wholeString;
	for (set<string>::iterator i = partialStrings.begin(); i != partialStrings.end(); )
	{
		wholeString += *i;
		if (++i != partialStrings.end())
			wholeString += delimiter;
	}
	return wholeString;
}

/**
 * Returns a new string with the whitespace removed from the beginning and end.
 *
 * This function trims ASCII spaces, tabs, and linebreaks, but not other Unicode whitespace characters.
 *
 * @see VuoText_trim
 */
string VuoStringUtilities::trim(string originalString)
{
	string whitespace = " \t\v\n\r\f";

	string::size_type begin = originalString.find_first_not_of(whitespace);
	if (begin == std::string::npos)
		return "";

	string::size_type end = originalString.find_last_not_of(whitespace);

	return originalString.substr(begin, end - begin + 1);
}

/**
 * Returns a string that starts with @a parentCompositionIdentifier and ends with @a nodeIdentifier.
 * The glue between them is a non-identifier character so that the string can be unambiguously split later.
 *
 * This needs to be kept in sync with @ref VuoCompilerNode::generateSubcompositionIdentifierValue().
 */
string VuoStringUtilities::buildCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier)
{
	return parentCompositionIdentifier + "/" + nodeIdentifier;
}

/**
 * Returns a string that starts with @a nodeIdentifier and ends with @a portName.
 * The glue between them is a non-identifier character so that the string can be unambiguously split later.
 */
string VuoStringUtilities::buildPortIdentifier(const string &nodeIdentifier, const string &portName)
{
	return nodeIdentifier + ":" + portName;
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
 *  - Replaces '/'s and ':'s with '__'s
 *  - Strips out characters not in [A-Za-z0-9_]
 */
string VuoStringUtilities::transcodeToIdentifier(string str)
{
	CFMutableStringRef strCF = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(strCF, str.c_str(), kCFStringEncodingUTF8);

	CFStringNormalize(strCF, kCFStringNormalizationFormD);  // decomposes combining characters, so accents/diacritics are separated from their letters

	CFIndex strLength = CFStringGetLength(strCF);
	UniChar *strBuf = (UniChar *)malloc(strLength * sizeof(UniChar));
	if (!strBuf)
	{
		CFRelease(strCF);
		return string();
	}
	CFStringGetCharacters(strCF, CFRangeMake(0, strLength), strBuf);

	CFStringRef empty = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8);
	CFStringRef underscore = CFStringCreateWithCString(NULL, "_", kCFStringEncodingUTF8);
	CFStringRef doubleUnderscore = CFStringCreateWithCString(NULL, "__", kCFStringEncodingUTF8);
	if (!empty || !underscore || !doubleUnderscore)
	{
		CFRelease(strCF);
		if (empty)
			CFRelease(empty);
		if (underscore)
			CFRelease(underscore);
		if (doubleUnderscore)
			CFRelease(doubleUnderscore);
		return string();
	}

	for (CFIndex i = strLength-1; i >= 0; --i)
	{
		UniChar c = strBuf[i];

		CFStringRef replacement = NULL;
		if (c > 127)  // non-ASCII
			replacement = empty;
		else if (c == '.' || isspace(c))
			replacement = underscore;
		else if (c == '/' || c == ':')
			replacement = doubleUnderscore;
		else if (! isValidCharInIdentifier(c))
			replacement = empty;

		if (replacement)
			CFStringReplace(strCF, CFRangeMake(i, 1), replacement);
	}

	string ret = makeFromCFString(strCF);

	CFRelease(strCF);
	CFRelease(empty);
	CFRelease(underscore);
	CFRelease(doubleUnderscore);
	free(strBuf);

	return ret;
}

/**
 * Check whether a character is valid for an
 * identifier, i.e., matches [A-Za-z0-9_]
 */
bool VuoStringUtilities::isValidCharInIdentifier(char ch)
{
	return isalnum(ch) || ch == '_';
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
	for (string::size_type i = 0; (i = escapedString.find("  ", i)) != std::string::npos; i += 3)
		escapedString.replace(i, 2, " \\ ");
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

/**
 * Returns @a preferredIdentifier if it's available (not already in @a usedIdentifiers), otherwise
 * creates an identifier that is available by adding a suffix to @a identifierPrefix (if provided)
 * or @a preferredIdentifier. The returned identifier is added to @a usedIdentifiers.
 */
string VuoStringUtilities::formUniqueIdentifier(set<string> &takenIdentifiers,
												const string &preferredIdentifier, const string &identifierPrefix)
{
	auto isIdentifierAvailable = [&takenIdentifiers] (const string &identifier)
	{
		return takenIdentifiers.find(identifier) == takenIdentifiers.end();
	};

	string uniqueIdentifier = formUniqueIdentifier(isIdentifierAvailable, preferredIdentifier, identifierPrefix);
	takenIdentifiers.insert(uniqueIdentifier);
	return uniqueIdentifier;
}

/**
 * Returns @a preferredIdentifier if it's available (according to @a isIdentifierAvailable), otherwise
 * creates an identifier that is available by adding a suffix to @a identifierPrefix (if provided)
 * or @a preferredIdentifier.
 */
string VuoStringUtilities::formUniqueIdentifier(std::function<bool(const string &)> isIdentifierAvailable,
												const string &preferredIdentifier, const string &identifierPrefix)
{
	string unique = preferredIdentifier;
	string prefix = (! identifierPrefix.empty() ? identifierPrefix : preferredIdentifier);
	int suffix = 2;

	while (! isIdentifierAvailable(unique))
	{
		ostringstream oss;
		oss << prefix << suffix++;
		unique = oss.str();
	}

	return unique;
}

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
	mkd_cleanup(doc);

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
	free(html);
	return htmlString;
}

/**
 * Collapses a string into camel case by removing non-alphanumeric characters and adjusting capitalization.
 *
 * Also removes non-alpha leading characters.
 *
 * @param originalString The input.
 * @param forceFirstLetterToUpper If true, make the string UpperCamelCase. Otherwise, leave the first letter as-is.
 * @param forceFirstLetterToLower If true, make the string lowerCamelCase. Otherwise, leave the first letter as-is.
 * @param forceInterveningLettersToLower If true, make letters within words lowercase. Otherwise, leave them as-is.
 * @param allowSeparatorDots If true, intermediate dots (`.`) are preserved.
 *                           Leading, consecutive intermediate, and trailing dots are omitted.
 *                           Letters following dots are not forced to uppercase.
 */
string VuoStringUtilities::convertToCamelCase(const string &originalString,
											  bool forceFirstLetterToUpper, bool forceFirstLetterToLower, bool forceInterveningLettersToLower,
											  bool allowSeparatorDots)
{
	string camelCaseString;
	bool first = true;
	bool uppercaseNext = forceFirstLetterToUpper;
	bool lowercaseNext = forceFirstLetterToLower;
	bool previousWasDot = false;
	for (string::const_iterator i = originalString.begin(); i != originalString.end(); ++i)
	{
		if (first && !isalpha(*i))
			continue;
		first = false;

		bool isDot = *i == '.';
		if (allowSeparatorDots && isDot)
		{
			if (previousWasDot)
				continue;
			uppercaseNext = false;
		}
		else if (!isalnum(*i))
		{
			uppercaseNext = true;
			continue;
		}

		if (uppercaseNext)
			camelCaseString += toupper(*i);
		else if (lowercaseNext)
			camelCaseString += tolower(*i);
		else
			camelCaseString += *i;

		uppercaseNext = false;
		lowercaseNext = forceInterveningLettersToLower;
		previousWasDot = isDot;
	}

	// Trim trailing dots.
	if (allowSeparatorDots)
		while (endsWith(camelCaseString, "."))
			camelCaseString = substrBefore(camelCaseString, ".");

	return camelCaseString;
}

/**
 * Inserts spaces at CamelCase transitions within the input `camelCaseString`,
 * capitalizes the first letter of the string, and returns the result.
 *
 * Also renders standalone variable names (e.g., "x") and some common abbreviations (e.g., "rgb") in all-caps.
 */
string VuoStringUtilities::expandCamelCase(string camelCaseString)
{
	// Only apply these transformations if the whole string matches,
	// since they may appear as substrings in contexts where they shouldn't be all-caps.
	if (camelCaseString == "x")
		return "X";
	else if (camelCaseString == "y")
		return "Y";
	else if (camelCaseString == "z")
		return "Z";
	else if (camelCaseString == "w")
		return "W";
	else if (camelCaseString == "xy")
		return "XY";
	else if (camelCaseString == "osc")
		return "OSC";

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

	string allCaps[]{
		"2d",
		"3d",
		"4d",
		"Xyzw",
		"Xyz",
		"Rgbaw",
		"Rgba",
		"Rgbw",
		"Rgb",
		"Wwcw",
		"Cmy",
		"Hsl",
		"Hdmi",
		"Sdi",
		"Ntsc",
//		"Pal",  // Appears in Leap Motion "Palm Velocity".
		"Url",
		"Midi",
		"Rss",
		"Csv",
		"Tsv",
		"Ascii",
		"Json",
		"Xml",
		"Dmx",
	};
	for (auto lower : allCaps)
	{
		string upper = lower;
		std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
		VuoStringUtilities::replaceAll(out, lower, upper);
	}

	return out;
}

/**
 * Creates a string from a `CFStringRef`.
 */
string VuoStringUtilities::makeFromCFString(const void *cfs)
{
	if (!cfs)
		return "";

	CFStringRef cfString = (CFStringRef)cfs;

	// https://stackoverflow.com/questions/1609565/whats-the-cfstring-equiv-of-nsstrings-utf8string
	const char *utf8StringPtr = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8);
	if (utf8StringPtr)
		return utf8StringPtr;
	else
	{
		CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfString), kCFStringEncodingUTF8) + 1;
		char *t = (char *)calloc(1, maxBytes);
		CFStringGetCString(cfString, t, maxBytes, kCFStringEncodingUTF8);
		string s(t);
		free(t);
		return s;
	}
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

/**
 * Returns the SHA-256 hash of the string `s`, as a string of hex digits.
 *
 * @throw VuoException
 */
string VuoStringUtilities::calculateSHA256(const string &s)
{
	unsigned char hash[CC_SHA256_DIGEST_LENGTH];
	if (!CC_SHA256(s.c_str(), s.length(), hash))
		throw VuoException("Error: CC_SHA256 failed.");

	ostringstream oss;
	oss << setfill('0') << hex;
	for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i)
		oss << setw(2) << (int)hash[i];
	return oss.str();
}
