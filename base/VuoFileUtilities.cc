/**
 * @file
 * VuoFileUtilities implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <libgen.h>
#include <dirent.h>
#include <fcntl.h>
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"


/**
 * Splits @c path into its directory, file name, and file extension.
 *
 * @param[in] path The path to split.
 * @param[out] dir The directory in @c path, if any. Ends with a file separator (e.g. '/') if there was one in @c path.
 * @param[out] file The file name in @c path, if any.
 * @param[out] extension The file extension in @c path, if any. Does not include the '.' character.
 */
void VuoFileUtilities::splitPath(string path, string &dir, string &file, string &extension)
{
	dir = "";
	file = "";
	extension = "";
	const char FILE_SEPARATOR = '/';

	size_t separatorIndex = path.rfind(FILE_SEPARATOR);
	string fileAndExtension;
	if (separatorIndex != string::npos)
	{
		dir = path.substr(0, separatorIndex + 1);
		if (separatorIndex < path.length())
			fileAndExtension = path.substr(separatorIndex + 1, path.length());
	}
	else
	{
		fileAndExtension = path;
	}

	size_t dotIndex = fileAndExtension.rfind(".");
	if (dotIndex != string::npos)
	{
		file = fileAndExtension.substr(0, dotIndex);
		extension = fileAndExtension.substr(dotIndex + 1, fileAndExtension.length());
	}
	else
	{
		file = fileAndExtension;
	}
}

/**
 * Creates a new temporary file, avoiding any name conflicts with existing files.
 * Creates the file in the specified @c directory if one is provided, or in "/tmp" otherwise.
 *
 * Returns the path of the file.
 */
string VuoFileUtilities::makeTmpFile(string file, string extension, string directory)
{
	if ((directory.length() > 0) && (directory.at(directory.length()-1) != '/'))
		directory += "/";
	string suffix = (extension.empty() ? "" : ("." + extension));
	string pathTemplate = directory + file + "-XXXXXX" + suffix;
	char *path = (char *)calloc(pathTemplate.length() + 1, sizeof(char));
	strncpy(path, pathTemplate.c_str(), pathTemplate.length());
	int fd = mkstemps(path, suffix.length());
	close(fd);  /// @todo Keep file open and return file handle. (https://b33p.net/kosada/node/4987)
	string pathStr(path);
	free(path);
	return pathStr;
}

/**
 * Creates a new temporary directory, avoiding any name conflicts with existing files.
 *
 * Returns the path of the directory.
 */
string VuoFileUtilities::makeTmpDir(string dir)
{
	string pathTemplate = "/tmp/" + dir + ".XXXXXX";
	char *path = (char *)calloc(pathTemplate.length() + 1, sizeof(char));
	strncpy(path, pathTemplate.c_str(), pathTemplate.length());
	mkdtemp(path);
	string pathStr(path);
	free(path);
	return pathStr;
}

/**
 * Writes the contents of the input @c string to a C file.
 */
FILE * VuoFileUtilities::stringToCFile(const char *string)
{
	// Write memory buffer to a pipe
	int p[2];
	pipe(p);
	write(p[1], string, strlen(string)+1);
	close(p[1]);

	// Create a C file pointer from the pipe;
	return fdopen(p[0], "r");
}

/**
 * Searches a directory for files.
 *
 * @param dirPath The directory to search in.
 * @return The paths of the found files. The paths are relative to @c dirPath.
 */
set<string> VuoFileUtilities::findFilesInDirectory(string dirPath)
{
	/// @todo Search recursively - https://b33p.net/kosada/node/2468

	set<string> filePaths;

	DIR *d = opendir(dirPath.c_str());
	if (! d)
	{
		if (access(dirPath.c_str(), F_OK) != -1)
			fprintf(stderr, "Couldn't open directory '%s' to add it to the class path\n", dirPath.c_str());
		return filePaths;
	}

	struct dirent *de;
	while( (de=readdir(d)) )
	{
		string fileName = de->d_name;
		filePaths.insert(fileName);
	}

	closedir(d);

	return filePaths;
}

/**
 * Searches a directory for files with a given extension.
 *
 * @param dirPath The directory to search in.
 * @param extension The file extension to match on, without the '.' character (e.g. "bc" not ".bc").
 * @return The paths of the found files. The paths are relative to @c dirPath.
 */
set<string> VuoFileUtilities::findFilesInDirectory(string dirPath, string extension)
{
	set<string> filePaths = findFilesInDirectory(dirPath);

	for (set<string>::iterator i = filePaths.begin(); i != filePaths.end(); )
	{
		set<string>::iterator current = i++;
		if (! VuoStringUtilities::endsWith(*current, string(".") + extension))
			filePaths.erase(current);
	}

	return filePaths;
}

/**
 * Returns true if the file exists.
 */
bool VuoFileUtilities::fileExists(string path)
{
	return access(path.c_str(), 0) == 0;
}
