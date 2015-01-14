/**
 * @file
 * VuoStringUtilities interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSTRINGUTILITIES_H
#define VUOSTRINGUTILITIES_H

/**
 * Functions for dealing with strings.
 */
class VuoStringUtilities
{
public:
	static bool beginsWith(string wholeString, string beginning);
	static bool endsWith(string wholeString, string ending);
	static string substrAfter(string wholeString, string beginning);
	static string replaceAll(string wholeString, char originalChar, char replacementChar);
	static string transcodeToIdentifier(string str);
	static string transcodeToGraphvizIdentifier(const string &originalString);
	static string transcodeFromGraphvizIdentifier(const string &graphvizIdentifier);

private:
	static bool isValidCharInIdentifier(char ch);
};


#endif
