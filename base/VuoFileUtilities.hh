/**
 * @file
 * VuoFileUtilities interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOFILEUTILITIES_H
#define VUOFILEUTILITIES_H

/**
 * Functions for dealing with files.
 */
class VuoFileUtilities
{
public:
	static void splitPath(string path, string &dir, string &file, string &extension);
	static string makeTmpFile(string file, string extension, string directory="/tmp");
	static string makeTmpDir(string dir);
	static FILE * stringToCFile(const char *string);
	static set<string> findFilesInDirectory(string dirPath);
	static set<string> findFilesInDirectory(string dirPath, string extension);
	static bool fileExists(string path);
};

#endif
