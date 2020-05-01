/**
 * @file
 * VuoStringUtilities interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <locale>
#include <set>

/**
 * Functions for dealing with strings.
 */
class VuoStringUtilities
{
public:
	static bool beginsWith(string wholeString, string beginning);
	static bool endsWith(string wholeString, string ending);
	static string substrAfter(string wholeString, string beginning);
	static string substrBefore(string wholeString, string ending);
	static string replaceAll(string wholeString, char originalChar, char replacementChar);
	static size_t replaceAll(string &wholeString, string originalSubstring, string replacementSubstring);
	static vector<string> split(const string &wholeString, char delimiter);
	static string join(vector<string> partialStrings, char delimiter);
	static string join(vector<string> partialStrings, string delimiter);
	static string join(set<string> partialStrings, string delimiter);
	static string trim(string originalString);

	static string buildCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier);
	static string buildPortIdentifier(const string &nodeIdentifier, const string &portName);
	static string prefixSymbolName(string symbolName, string moduleKey);
	static bool isValidCharInIdentifier(char ch);
	static string transcodeToIdentifier(string str);
	static string transcodeToGraphvizIdentifier(const string &originalString);
	static string transcodeFromGraphvizIdentifier(const string &graphvizIdentifier);
	static string formUniqueIdentifier(set<string> &takenIdentifiers, const string &preferredIdentifier, const string &identifierPrefix = "");
	static string formUniqueIdentifier(std::function<bool(const string &)> isIdentifierAvailable, const string &preferredIdentifier, const string &identifierPrefix = "");

	static string generateHtmlFromMarkdown(const string &markdownString);
	static string generateHtmlFromMarkdownLine(const string &markdownString);

	static string convertToCamelCase(const string &originalString, bool capitalizeFirstLetter, bool forceFirstLetterToLower, bool forceInterveningLettersToLower,
									 bool allowSeparatorDots = false);
	static string expandCamelCase(string camelCaseString);

	static string makeFromCFString(const void *cfs);
	static string makeRandomHash(int length);

	/**
	 * Generates an integer hash value for the string.
	 */
	static inline long hash(const string &s)
	{
		return collate.hash(s.data(), s.data() + s.length());
	}

	static const std::locale locale;  ///< For hashing strings.
	static const std::collate<char> &collate;  ///< For hashing strings.

	static string calculateSHA256(const string &s);
};
