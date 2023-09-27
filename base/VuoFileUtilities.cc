/**
 * @file
 * VuoFileUtilities implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <CommonCrypto/CommonDigest.h>
#include <dirent.h>
#include <spawn.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <copyfile.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mach-o/dyld.h>
#include <sys/time.h>
#include "VuoException.hh"
#include "VuoFileUtilities.hh"
#include "VuoFileUtilitiesCocoa.hh"
#include "VuoStringUtilities.hh"

bool VuoFileUtilities::dylibLoaderInitialMatchCompleted = false;
string VuoFileUtilities::vuoFrameworkPath;
string VuoFileUtilities::vuoRunnerFrameworkPath;

/**
 * Splits @c path into its directory, file name, and file extension.
 *
 * @param[in] path The path to split.
 * @param[out] dir The directory in @c path, if any. Ends with a file separator (e.g. '/') if there was one in @c path.
 * @param[out] file The file name in @c path, if any.
 * @param[out] extension The file extension in @c path, if any. Does not include the '.' character.
 *
 * @see VuoUrl_getFileParts
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
 * Transforms @a path to a standard format. The returned path is absolute if @a path is absolute,
 * relative if @a path is relative.
 */
void VuoFileUtilities::canonicalizePath(string &path)
{
	// Remove repeated file separators.
	for (int i = 0; i < path.length(); ++i)
		if (path[i] == '/')
			while (i+1 < path.length() && path[i+1] == '/')
				path.erase(i+1, 1);

	// Remove trailing file separator.
	if (path.length() > 1 && VuoStringUtilities::endsWith(path, "/"))
		path.erase(path.length()-1);
}

/**
 * Returns true if the paths refer to the same location.
 *
 * This handles different formats for the same path (by comparing the canonicalized paths).
 * It does not follow symlinks.
 */
bool VuoFileUtilities::arePathsEqual(string path1, string path2)
{
	canonicalizePath(path1);
	canonicalizePath(path2);
	return path1 == path2;
}

/**
 * Returns @a path itself if it's already an absolute path, or else an absolute path
 * consisting of @a path relative to the current working directory.
 */
string VuoFileUtilities::getAbsolutePath(const string &path)
{
	if (isAbsolutePath(path))
		return path;

	char *cwd = getcwd(NULL, 0);
	if (! cwd)
	{
		VUserLog("Couldn't get current working directory: %s", strerror(errno));
		return path;
	}

	string absolutePath = string(cwd) + "/" + path;
	free(cwd);
	return absolutePath;
}

/**
 * Returns true if @a path is an absolute path (regardless of whether the file exists),
 * false if @a path is a relative path.
 */
bool VuoFileUtilities::isAbsolutePath(const string &path)
{
	return VuoStringUtilities::beginsWith(path, "/");
}

/**
 * Creates a new temporary file with mode 0600,
 * avoiding any name conflicts with existing files.
 * Creates the file in the specified @c directory if one is provided, or in the user's temporary directory otherwise.
 *
 * Returns the path of the file.
 */
string VuoFileUtilities::makeTmpFile(string file, string extension, string directory)
{
	if (directory.empty())
		directory = getTmpDir();
	if (directory.at(directory.length()-1) != '/')
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
 * Creates a new temporary directory with mode 0700,
 * whose final path element begins with `prefix`
 * and has a unique extension (avoiding name conflicts with existing files).
 *
 * `prefix` shouldn't contain any slashes.
 *
 * Returns the path of the directory (without a trailing slash).
 */
string VuoFileUtilities::makeTmpDir(string prefix)
{
	string pathTemplate = getTmpDir() + "/" + prefix + ".XXXXXX";
	char *path = (char *)calloc(pathTemplate.length() + 1, sizeof(char));
	strncpy(path, pathTemplate.c_str(), pathTemplate.length());
	mkdtemp(path);
	string pathStr(path);
	free(path);
	return pathStr;
}

/**
 * Creates a new temporary directory, avoiding any name conflicts with existing files.
 * The temporary directory will be on the same filesystem as the specified path,
 * to facilitate using `rename()` (for example).
 * `onSameVolumeAsPath` needn't already exist.
 *
 * Returns the path of the directory (without a trailing slash).
 */
string VuoFileUtilities::makeTmpDirOnSameVolumeAsPath(string path)
{
	return VuoFileUtilitiesCocoa_makeTmpDirOnSameVolumeAsPath(path);
}

/**
 * For non-sandboxed processes,
 * returns the user's private temporary directory if avaialble
 * (e.g., `/var/folders/f3/v1j4zqhs5jz5757t0rmh7jj80000gn/T`),
 * otherwise returns the system temporary directory (`/tmp`).
 *
 * For sandboxed processes, returns the sandbox container directory
 * (e.g., `/Users/me/Library/Containers/com.apple.ScreenSaver.Engine.legacyScreenSaver/Data`).
 *
 * The returned path does not include a trailing slash.
 */
string VuoFileUtilities::getTmpDir(void)
{
	char *cwd = getcwd(NULL, 0);
	if (!cwd)
		VUserLog("Error in getcwd(): %s", strerror(errno));

	if (cwd && strstr(cwd, "/Library/Containers/"))  // We're in a sandbox.
	{
		// https://b33p.net/kosada/node/16374
		// In the macOS 10.15 screen saver sandbox, at least,
		// we're not allowed to use the system or user temporary directories for socket-files,
		// but we are allowed to use the sandbox container directory.
		string cwdS(cwd);
		free(cwd);
		return VuoStringUtilities::endsWith(cwdS, "/") ? VuoStringUtilities::substrBefore(cwdS, "/") : cwdS;
	}
	free(cwd);

	char userTempDir[PATH_MAX];
	if (confstr(_CS_DARWIN_USER_TEMP_DIR, userTempDir, PATH_MAX) > 0)
	{
		string userTempDirS(userTempDir);
		return VuoStringUtilities::endsWith(userTempDirS, "/") ? VuoStringUtilities::substrBefore(userTempDirS, "/") : userTempDirS;
	}

	return "/tmp";
}

/**
 * Creates a new directory (and parent directories if needed), if it doesn't already exist.
 *
 * @throw VuoException The directory couldn't be created.
 *
 * @version200Changed{The new directory now respects the process's umask.}
 */
void VuoFileUtilities::makeDir(string path)
{
	if (path.empty())
		throw VuoException("Couldn't create directory with empty path");

	if (dirExists(path))
		return;

	const char FILE_SEPARATOR = '/';
	size_t lastNonSeparator = path.find_last_not_of(FILE_SEPARATOR);
	if (lastNonSeparator != string::npos)
		path.resize(lastNonSeparator + 1);

	string parentDir, file, ext;
	splitPath(path, parentDir, file, ext);
	makeDir(parentDir);

	// `mkdir` ANDs the mode with the process's umask,
	// so by default the created directory's mode will end up being 0755.
	int ret = mkdir(path.c_str(), 0777);
	int mkdirErrno = errno;
	if (ret != 0 && ! dirExists(path))
		throw VuoException((string("Couldn't create directory \"" + path + "\": ") + strerror(mkdirErrno)).c_str());
}

/**
 * Helper for @ref initializeVuoFrameworkPaths.
 *
 * This needs to be kept in sync with @ref VuoFrameworkPaths_dylibLoaded().
 */
void VuoFileUtilities::dylibLoaded(const struct mach_header *mh32, intptr_t vmaddr_slide)
{
	if (dylibLoaderInitialMatchCompleted)
		return;

	const struct mach_header_64 *mh = reinterpret_cast<const mach_header_64 *>(mh32);

	// Ignore system libraries.
	if (mh->flags & MH_DYLIB_IN_CACHE)
		return;

	// Get the file path of the current dylib.
	Dl_info info{"", nullptr, "", nullptr};
	dladdr((void *)vmaddr_slide, &info);

	// Check whether it's one of the dylibs we're looking for.

	auto getMatchingPath = [info](string fragment) -> string {
		string qualifiedFragment = "/" + fragment + ".framework/Versions/";
		const char *found = strstr(info.dli_fname, qualifiedFragment.c_str());
		if (found)
		{
			char *pathC = strndup(info.dli_fname, found - info.dli_fname);
			string path = string(pathC) + "/" + fragment + ".framework";
			free(pathC);
			if (access(path.c_str(), 0) == 0)
				return path;
		}
		return string();
	};

	string possibleVuoFramework = getMatchingPath("Vuo");
	if (!possibleVuoFramework.empty())
		vuoFrameworkPath = possibleVuoFramework;

	string possibleVuoRunnerFramework = getMatchingPath("VuoRunner");
	if (!possibleVuoRunnerFramework.empty())
		vuoRunnerFrameworkPath = possibleVuoRunnerFramework;
}

/**
 * Helper for @ref getVuoFrameworkPath and @ref getVuoRunnerFrameworkPath.
 */
void VuoFileUtilities::initializeVuoFrameworkPaths(void)
{
	static once_flag once;
	call_once(once, []() {
		// Check whether Vuo.framework is in the list of loaded dynamic libraries.
		_dyld_register_func_for_add_image(&dylibLoaded);
		dylibLoaderInitialMatchCompleted = true;
		// The above function invokes the callback for each already-loaded dylib, then returns.

		if (vuoRunnerFrameworkPath.empty() && !vuoFrameworkPath.empty())
		{
			// Check for VuoRunner.framework alongside Vuo.framework.
			string pathCandidate = vuoFrameworkPath + "/../VuoRunner.framework";
			if (dirExists(pathCandidate))
				vuoRunnerFrameworkPath = pathCandidate;
		}
	});
}

/**
 * Returns the absolute path of Vuo.framework
 * (including the `Vuo.framework` directory name without a trailing slash),
 * or an empty string if Vuo.framework cannot be located.
 *
 * For a C version accessible from nodes and libraries, see @ref VuoGetFrameworkPath.
 */
string VuoFileUtilities::getVuoFrameworkPath(void)
{
	initializeVuoFrameworkPaths();
	return vuoFrameworkPath;
}

/**
 * Returns the absolute path of VuoRunner.framework
 * (including the `VuoRunner.framework` directory name without a trailing slash),
 * or an empty string if VuoRunner.framework cannot be located.
 */
string VuoFileUtilities::getVuoRunnerFrameworkPath(void)
{
	initializeVuoFrameworkPaths();
	return vuoRunnerFrameworkPath;
}

/**
 * Returns the path of the composition-local Modules folder for a composition located at
 * @a compositionPath. (The composition file need not actually exist.)
 *
 * If the composition is installed as a subcomposition (it's located in a folder called Modules),
 * returns that folder. Otherwise, returns a Modules folder located in the same folder as
 * the composition.
 */
string VuoFileUtilities::getCompositionLocalModulesPath(const string &compositionPath)
{
	string compositionDir, compositionFile, ext;
	splitPath(getAbsolutePath(compositionPath), compositionDir, compositionFile, ext);
	canonicalizePath(compositionDir);

	string parentDir, compositionDirName;
	splitPath(compositionDir, parentDir, compositionDirName, ext);
	canonicalizePath(compositionDirName);

	string compositionBaseDir = (compositionDirName == "Modules" ? parentDir : compositionDir);

	string compositionModulesDir = compositionBaseDir + "/Modules";
	canonicalizePath(compositionModulesDir);

	return compositionModulesDir;
}

/**
 * Returns the filesystem path to the user-specific Vuo Modules folder (without a trailing slash).
 *
 * The folder does not necessarily exist yet.
 *
 * If the process is running in a sandbox, the user's home folder may not be available,
 * in which case this method returns an empty string.
 */
string VuoFileUtilities::getUserModulesPath()
{
	const char *home = getenv("HOME");
	if (!home)
		return string();

	return string(home) + "/Library/Application Support/Vuo/Modules";
}

/**
 * Returns the filesystem path to the system-wide Vuo Modules folder (without a trailing slash).
 */
string VuoFileUtilities::getSystemModulesPath()
{
	return "/Library/Application Support/Vuo/Modules";
}

/**
 * Returns the filesystem path to the folder that Vuo uses to cache data (without a trailing slash).
 *
 * The folder does not necessarily exist yet.
 *
 * If the process is running in a sandbox, the user's home folder may not be available,
 * in which case this method returns an empty string.
 */
string VuoFileUtilities::getCachePath(void)
{
	const char *home = getenv("HOME");
	if (!home)
		return string();

	return string(home) + "/Library/Caches/org.vuo/" + VUO_VERSION_AND_BUILD_STRING;
}

/**
 * Returns true if @a path (a source file or compiled module) is located in a Modules folder.
 */
bool VuoFileUtilities::isInstalledAsModule(const string &path)
{
	if (path.empty())
		return false;

	string compositionLocalModulesPath = getCompositionLocalModulesPath(path);

	string dir, file, ext;
	VuoFileUtilities::splitPath(path, dir, file, ext);

	return VuoFileUtilities::arePathsEqual(compositionLocalModulesPath, dir);
}

/**
 * Saves @a originalFileName into @a fileContents so that, when @a fileContents is written to some other
 * file path, @a originalFileName will still be the name that shows up in compile errors/warnings and
 * in the @c __FILE__ macro.
 *
 * This function inserts a preprocessor directive at the beginning of @a fileContents. Any modifications
 * to @a fileContents after this function is called should keep the preprocessor directive on the first line.
 */
void VuoFileUtilities::preserveOriginalFileName(string &fileContents, string originalFileName)
{
	fileContents.insert(getFirstInsertionIndex(fileContents), "#line 1 \"" + originalFileName + "\"\n");
}

/**
 * Returns the first index at which content can be inserted into a string that was read from a file.
 * This comes after the Unicode BOM, if present.
 */
size_t VuoFileUtilities::getFirstInsertionIndex(string s)
{
	string bomUtf8 = "\xEF\xBB\xBF";
	string bomUtf16Be = "\xFE\xFF";
	string bomUtf16Le = "\xFF\xFE";
	string boms[] = { bomUtf8, bomUtf16Be, bomUtf16Le };
	for (int i = 0; i < 3; ++i)
	{
		bool isMatch = true;
		for (int j = 0; j < boms[i].length(); ++j)
			if (boms[i][j] != s[j])
				isMatch = false;

		if (isMatch)
			return boms[i].length();
	}
	return 0;
}

/**
 * Reads from standard input into a string until the end-of-file is reached.
 */
string VuoFileUtilities::readStdinToString(void)
{
	// https://stackoverflow.com/questions/201992/how-to-read-until-eof-from-cin-in-c

	string contents;
	string line;
	while (getline(cin, line))
	   contents += line;

	return contents;
}

/**
 * Reads the whole contents of the file into a string.
 *
 * @throw VuoException The file couldn't be read.
 */
string VuoFileUtilities::readFileToString(string path)
{
	// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

	ifstream in(path.c_str(), ios::in | ios::binary);
	if (! in)
		throw VuoException(string("Couldn't read file: ") + strerror(errno) + " — " + path);

	string contents;
	in.seekg(0, ios::end);
	contents.resize(in.tellg());
	in.seekg(0, ios::beg);
	in.read(&contents[0], contents.size());
	in.close();
	return contents;
}

/**
 * Writes the array of bytes to the file.
 *
 * If the file already exists, it gets overwritten.
 *
 * @throw VuoException The file couldn't be written.
 */
void VuoFileUtilities::writeRawDataToFile(const char *data, size_t numBytes, string file)
{
	FILE *f = fopen(file.c_str(), "wb");
	if (! f)
		throw VuoException(string("Couldn't open file for writing: ") + strerror(errno) + " — " + file);

	size_t numBytesWritten = fwrite(data, sizeof(char), numBytes, f);
	if (numBytesWritten != numBytes)
	{
		fclose(f);
		throw VuoException(string("Couldn't write all data: ") + strerror(errno) + " — " + file);
	}

	fclose(f);
}

/**
 * Writes the string to the file.
 *
 * If the file already exists, it gets overwritten.
 *
 * @throw VuoException The file couldn't be written.
 */
void VuoFileUtilities::writeStringToFile(string s, string file)
{
	writeRawDataToFile(s.data(), s.length(), file);
}

/**
 * Saves to file using the standard Unix procedure (write to temporary file, then rename).
 * This prevents the destination file from becoming corrupted if the write fails.
 *
 * If the file already exists, it gets overwritten.
 *
 * @throw VuoException The file couldn't be written.
 */
void VuoFileUtilities::writeStringToFileSafely(string s, string path)
{
	string dir, file, ext;
	splitPath(path, dir, file, ext);
	string tmpPath = makeTmpFile("." + file, ext, dir);

	writeStringToFile(s, tmpPath);

	moveFile(tmpPath, path);
}

/**
 * Returns true if the file exists.
 *
 * If `path` is a symlink or macOS Alias, it is dereferenced.
 */
bool VuoFileUtilities::fileExists(string path)
{
	if (VuoFileUtilitiesCocoa_isMacAlias(path))
		path = VuoFileUtilitiesCocoa_resolveMacAlias(path);

	// `access()` automatically dereferences symlinks.
	return access(path.c_str(), 0) == 0;
}

/**
 * Returns true if `path` exists and is a directory.
 *
 * If `path` is a symlink or macOS Alias, it is dereferenced.
 */
bool VuoFileUtilities::dirExists(string path)
{
	if (VuoFileUtilitiesCocoa_isMacAlias(path))
		path = VuoFileUtilitiesCocoa_resolveMacAlias(path);

	// `stat()` automatically dereferences symlinks.
	struct stat st_buf;
	int status = stat(path.c_str(), &st_buf);
	return (! status && S_ISDIR(st_buf.st_mode));
}

/**
 * Returns true if `path` exists and is a symlink.
 */
bool VuoFileUtilities::isSymlink(string path)
{
	struct stat st_buf;
	int status = lstat(path.c_str(), &st_buf);
	return (! status && S_ISLNK(st_buf.st_mode));
}

/**
 * Returns true if the file or directory is readable.
 *
 * If `path` is a symlink or macOS Alias, it is dereferenced.
 */
bool VuoFileUtilities::fileIsReadable(string path)
{
	if (VuoFileUtilitiesCocoa_isMacAlias(path))
		path = VuoFileUtilitiesCocoa_resolveMacAlias(path);

	// `access()` automatically dereferences symlinks.
	return access(path.c_str(), R_OK) == 0;
}

/**
 * Returns true if the file exists, can be opened, and has a size of more than 0 bytes.
 *
 * If `path` is a symlink or macOS Alias, it is dereferenced.
 */
bool VuoFileUtilities::fileContainsReadableData(string path)
{
	if (VuoFileUtilitiesCocoa_isMacAlias(path))
		path = VuoFileUtilitiesCocoa_resolveMacAlias(path);

	// `fopen()` automatically dereferences symlinks.
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	long pos = ftell(f);
	if (pos <= 0)
	{
		fclose(f);
		return false;
	}

	// `fopen` and `fseek` both succeed on directories, so also attempt to actually read the data.
	fseek(f, 0, SEEK_SET);
	char z;
	size_t ret = fread(&z, 1, 1, f);
	fclose(f);
	return ret > 0;
}

/**
 * Creates the file if it does not exist already; otherwise, has no effect on the file.
 *
 * @throw VuoException The user doesn't have permission to create the file.
 */
void VuoFileUtilities::createFile(string path)
{
	FILE *f = fopen(path.c_str(), "a");
	if (! f)
		throw VuoException(string("Couldn't open file for writing: ") + strerror(errno) + " — " + path);

	fclose(f);
}

/**
 * Deletes the file or directory if it exists and, if a directory, is empty; otherwise, has no effect.
 */
void VuoFileUtilities::deleteFile(string path)
{
	int ret = remove(path.c_str());
	if (ret != 0 && fileExists(path))
		VUserLog("Couldn't delete file: %s — %s", strerror(errno), path.c_str());
}

/**
 * Deletes the directory and its contents if the directory exists; otherwise, has no effect.
 */
void VuoFileUtilities::deleteDir(string path)
{
	if (! dirExists(path))
		return;

	DIR *d = opendir(path.c_str());
	if (! d)
	{
		VUserLog("Couldn't read directory: %s — %s", strerror(errno), path.c_str());
		return;
	}

	struct dirent *de;
	while( (de=readdir(d)) )
	{
		string fileName = de->d_name;
		string filePath = path + "/" + fileName;

		if (fileName == "." || fileName == "..")
			continue;

		if (dirExists(filePath))
			deleteDir(filePath);
		else
			deleteFile(filePath);
	}

	closedir(d);

	deleteFile(path);
}

/**
 * Moves the file from @a fromPath to @a toPath.
 *
 * @throw VuoException The file couldn't be moved.
 */
void VuoFileUtilities::moveFile(string fromPath, string toPath)
{
	int ret = rename(fromPath.c_str(), toPath.c_str());
	if (ret != 0)
		throw VuoException(string("Couldn't move file: ") + strerror(errno) + " — " + toPath);
}

/**
 * Moves the specified file to the user's trash folder.
 *
 * @throw VuoException The file couldn't be moved.
 */
void VuoFileUtilities::moveFileToTrash(string filePath)
{
	VuoFileUtilitiesCocoa_moveFileToTrash(filePath);
}

/**
 * Copies the file from @a fromPath to @a toPath,
 * preserving the file's mode.
 *
 * If @a preserveMetadata is true and @a toPath already exists, the file's inode and stat info is preserved,
 * thus so are any locks on the file.
 *
 * @throw VuoException The file couldn't be copied.
 *
 * @version200Changed{The copied file now preserves the original file's mode.}
 */
void VuoFileUtilities::copyFile(string fromPath, string toPath, bool preserveMetadata)
{
	int i = open(fromPath.c_str(), O_RDONLY);
	if (i == -1)
		throw VuoException(string("Couldn't open copy source: ") + strerror(errno) + " — " + fromPath);

	struct stat s;
	fstat(i, &s);
	int o = open(toPath.c_str(), O_WRONLY | O_CREAT, s.st_mode & 0777);
	if (o == -1)
	{
		close(i);
		throw VuoException(string("Couldn't open copy destination: ") + strerror(errno) + " — " + toPath);
	}

	int ret = fcopyfile(i, o, NULL, COPYFILE_DATA | (preserveMetadata ? COPYFILE_STAT : 0));
	char *e = strerror(errno);
	close(o);
	close(i);

	if (ret)
		throw VuoException(string("Couldn't copy ") + fromPath + " to " + toPath + ": " + e);

	VDebugLog("%s -> %s", fromPath.c_str(), toPath.c_str());
}

/**
 * Recursively copies the provided file or directory from `fromPath` to `toPath`.
 *
 * Preserves symlinks (rather than copying their target files).
 *
 * @throw VuoException The file couldn't be copied.
 *
 * @version200Changed{Each copied file now preserves the original file's mode.}
 */
void VuoFileUtilities::copyDirectory(string fromPath, string toPath)
{
	if (isSymlink(fromPath))
	{
		// Preserve the relative symlinks found in framework bundles, instead of flattening them.
		char linkDestination[PATH_MAX + 1];
		ssize_t len = readlink(fromPath.c_str(), linkDestination, PATH_MAX);
		linkDestination[len] = 0;  // "readlink does not append a NUL character to buf."
		if (symlink(linkDestination, toPath.c_str()) == -1)
			throw VuoException(string("Couldn't copy symlink \"") + fromPath + "\" to \"" + toPath + "\": " + strerror(errno));

		VDebugLog("%s -> %s (symlink)", fromPath.c_str(), toPath.c_str());
	}

	else if (!dirExists(fromPath))
		copyFile(fromPath, toPath);

	else
	{
		VDebugLog("%s ::", fromPath.c_str());
		auto files = findAllFilesInDirectory(fromPath);
		makeDir(toPath);
		for (auto file : files)
		{
			string sourceFile = fromPath + "/" + file->getRelativePath();
			string targetFile = toPath + "/" + file->getRelativePath();
			copyDirectory(sourceFile, targetFile);
		}
	}
}

/**
 * Returns the SHA-256 hash of the file at `path` as a string of hex digits.
 *
 * @throw VuoException
 */
string VuoFileUtilities::calculateFileSHA256(const string &path)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw VuoException("Error: Couldn't open \"" + path + "\": " + strerror(errno));
	VuoDefer(^{ close(fd); });

	struct stat stat;
	if (fstat(fd, &stat) != 0)
		throw VuoException("Error: Couldn't fstat \"" + path + "\": " + strerror(errno));

	// Instead of reading the file into a string and calling `VuoStringUtilities::calculateSHA256`,
	// use `mmap` so the OS can efficiently read parts of the file at a time (reducing memory required).
	void *data = mmap(nullptr, stat.st_size, PROT_READ, MAP_PRIVATE | MAP_NOCACHE, fd, 0);
	if (data == MAP_FAILED)
		throw VuoException("Error: Couldn't mmap \"" + path + "\": " + strerror(errno));
	VuoDefer(^{ munmap(data, stat.st_size); });

	unsigned char hash[CC_SHA256_DIGEST_LENGTH];
	if (!CC_SHA256(data, stat.st_size, hash))
		throw VuoException("Error: CC_SHA256 failed on file \"" + path + "\"");

	ostringstream oss;
	oss << setfill('0') << hex;
	for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i)
		oss << setw(2) << (int)hash[i];
	return oss.str();
}

/**
 * Returns the time that the file was last modified, in seconds from a reference date,
 * with fractional seconds up to microsecond resolution if the filesystem supports it.
 * This is useful for checking which of two files was modified more recently.
 */
double VuoFileUtilities::getFileLastModifiedInSeconds(string path)
{
	struct stat s;
	lstat(path.c_str(), &s);
	return s.st_mtimespec.tv_sec + ((s.st_mtimespec.tv_nsec/1000)/1000000.0);
}

/**
 * Returns the time since the file was last accessed or modified (whichever is more recent), in seconds,
 * with fractional seconds up to microsecond resolution if the filesystem supports it.
 */
double VuoFileUtilities::getSecondsSinceFileLastAccessed(string path)
{
	// Access times are tricky. Some Unix/Linux filesystems don't track access times at all.
	// On APFS, the exact behavior depends on whether APFS_FEATURE_STRICTATIME is set on the volume.
	// Here we check access time in case it's been updated, but use modification time as a backup.

	struct stat s;
	lstat(path.c_str(), &s);
	double lastAccessed = s.st_atimespec.tv_sec + ((s.st_atimespec.tv_nsec/1000)/1000000.0);

	double lastModified = getFileLastModifiedInSeconds(path);

	struct timeval t;
	gettimeofday(&t, NULL);

	return (t.tv_sec + t.tv_usec/1000000.0) - max(lastAccessed, lastModified);
}

/**
 * Searches a directory for files.
 *
 * If `dirPath` is a symlink or macOS Alias, it is dereferenced.
 *
 * @param dirPath The directory to search in.
 * @param archiveExtensions The file extensions for archives to search in. Any archive with one of these extensions
 *		found in the top level of the directory will be searched recursively.
 * @param shouldSearchRecursively If true, the directory will be searched searched recursively.
 *                                If false, only the top level is searched.
 * @return All files found.
 *
 * @throw VuoException The directory couldn't be read.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findAllFilesInDirectory(string dirPath, set<string> archiveExtensions,
																		bool shouldSearchRecursively)
{
	set<File *> files;

	if (VuoFileUtilitiesCocoa_isMacAlias(dirPath))
		dirPath = VuoFileUtilitiesCocoa_resolveMacAlias(dirPath);

	// `opendir()` automatically dereferences symlinks.
	DIR *d = opendir(dirPath.c_str());
	if (! d)
	{
		if (access(dirPath.c_str(), F_OK) != -1)
			throw VuoException(string("Couldn't read directory: ") + strerror(errno) + " — " + dirPath);
		return files;
	}

	struct dirent *de;
	while( (de=readdir(d)) )
	{
		string fileName = de->d_name;
		string relativeFilePath = dirPath + "/" + fileName;

		if (fileName == "." || fileName == "..")
			continue;

		bool isArchive = false;
		for (set<string>::iterator archiveExtension = archiveExtensions.begin(); archiveExtension != archiveExtensions.end(); ++archiveExtension)
			if (VuoStringUtilities::endsWith(fileName, "." + *archiveExtension))
				isArchive = true;

		if (isArchive)
		{
			set<File *> fs = findAllFilesInArchive(relativeFilePath);
			if (fs.empty())
				isArchive = false;
			else
				files.insert(fs.begin(), fs.end());
		}

		if (! isArchive)
		{
			bool shouldSearchDir = shouldSearchRecursively
								&& dirExists(relativeFilePath)
								&& !isSymlink(relativeFilePath);
			if (shouldSearchDir)
			{
				set<File *> filesInDir = findAllFilesInDirectory(relativeFilePath, archiveExtensions, true);
				for (set<File *>::iterator i = filesInDir.begin(); i != filesInDir.end(); ++i)
				{
					File *f = *i;
					f->dirPath = dirPath;
					f->filePath = fileName + "/" + f->filePath;
				}
				files.insert(filesInDir.begin(), filesInDir.end());
			}
			else
			{
				File *f = new File(dirPath, fileName);
				files.insert(f);
			}
		}
	}

	closedir(d);

	return files;
}

/**
 * Searches a directory for files with a given extension.
 *
 * @param dirPath The directory to search in. Only the top level is searched.
 * @param archiveExtensions The file extensions for archives to search in. Any archive with one of these extensions
 *		found in the top level of the directory will be searched recursively.
 * @param extensions The file extensions to search for, without the '.' character (e.g. "bc" not ".bc").
 * @return All files found.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findFilesInDirectory(string dirPath, set<string> extensions, set<string> archiveExtensions)
{
	set<File *> allFiles = findAllFilesInDirectory(dirPath, archiveExtensions);
	set<File *> matchingFiles;

	for (set<File *>::iterator i = allFiles.begin(); i != allFiles.end(); ++i)
	{
		bool endsWithExtension = false;
		for (set<string>::iterator extension = extensions.begin(); extension != extensions.end(); ++extension)
			if (VuoStringUtilities::endsWith((*i)->getRelativePath(), "." + *extension))
				endsWithExtension = true;

		if (endsWithExtension && ((*i)->isInArchive() || !VuoFileUtilities::dirExists((*i)->path())))
			matchingFiles.insert(*i);
		else
			delete *i;
	}

	return matchingFiles;
}

/**
 * Recursively searches an archive for files.
 *
 * @param archivePath The archive to search in.
 * @return All files found.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findAllFilesInArchive(string archivePath)
{
	set<File *> files;

	Archive *archive = new Archive(archivePath);
	if (! archive->zipArchive)
	{
		delete archive;
		return files;
	}

	mz_uint numFiles = mz_zip_reader_get_num_files(archive->zipArchive);
	for (mz_uint i = 0; i < numFiles; ++i)
	{
		mz_zip_archive_file_stat file_stat;
		bool success = mz_zip_reader_file_stat(archive->zipArchive, i, &file_stat);
		if (! success)
		{
			VUserLog("Error: Couldn't read file '%u' in zip archive '%s'.", i, archive->path.c_str());
			break;
		}

		File *f = new File(archive, file_stat.m_filename);
		files.insert(f);
	}

	if (archive->referenceCount == 0)
		delete archive;

	return files;
}

/**
 * Recursively searches an archive for files within a given directory and with a given extension.
 *
 * @param archivePath The archive to search in.
 * @param dirPath The directory to search in within the archive. The path should be relative to the archive's root.
 *			The path should omit the trailing file separator (e.g. "examples" not "examples/").
 * @param extensions The file extensions to search for, without the '.' character (e.g. "bc" not ".bc").
 * @return All files found.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findFilesInArchive(string archivePath, string dirPath, set<string> extensions)
{
	set<File *> allFiles = findAllFilesInArchive(archivePath);
	set<File *> matchingFiles;

	for (set<File *>::iterator i = allFiles.begin(); i != allFiles.end(); ++i)
	{
		File *f = *i;

		string dir, file, ext;
		splitPath(f->getRelativePath(), dir, file, ext);
		bool beginsWithDir = VuoStringUtilities::beginsWith(dir, dirPath);

		bool endsWithExtension = false;
		for (set<string>::iterator extension = extensions.begin(); extension != extensions.end(); ++extension)
			if (VuoStringUtilities::endsWith(f->getRelativePath(), "." + *extension))
				endsWithExtension = true;

		if (beginsWithDir && endsWithExtension)
			matchingFiles.insert(f);
		else
			delete f;
	}

	return matchingFiles;
}

/**
 * Returns the contents of a file within an archive.
 *
 * If the file doesn't exist, returns an empty string.
 *
 * @param archivePath The archive containing the file.
 * @param filePath The file. The path should be relative to the archive's root.
 * @return The file's contents.
 */
string VuoFileUtilities::getArchiveFileContentsAsString(string archivePath, string filePath)
{
	string contents;
	set<File *> archiveFiles = findAllFilesInArchive(archivePath);
	for (set<File *>::iterator i = archiveFiles.begin(); i != archiveFiles.end(); ++i)
	{
		File *file = *i;
		if (filePath == file->getRelativePath())
			contents = file->getContentsAsString();
		delete file;
	}

	return contents;
}

/**
 * Returns the available space, in bytes, on the volume containing the specified path.
 *
 * `path` should be an absolute POSIX path.  Its last few path components needn't exist.
 *
 * @throw VuoException
 */
size_t VuoFileUtilities::getAvailableSpaceOnVolumeContainingPath(string path)
{
	return VuoFileUtilitiesCocoa_getAvailableSpaceOnVolumeContainingPath(path);
}

/**
 * Ad-hoc code-signs the specified binary.
 */
void VuoFileUtilities::adHocCodeSign(string path, vector<string> environment, string entitlementsPath)
{
	double t0 = VuoLogGetTime();
	vector<string> processAndArgs{
		getVuoFrameworkPath() + "/Helpers/codesignWrapper.sh",
		"--force",
		"--sign",
		"-",  // "-" = ad-hoc
		path,
	};
	if (!entitlementsPath.empty())
	{
		processAndArgs.push_back("--entitlements");
		processAndArgs.push_back(entitlementsPath);
	}
	executeProcess(processAndArgs, environment);

	VDebugLog("\tAd-hoc code-signing took %5.2fs", VuoLogGetTime() - t0);
}

/**
 * Creates an archive with an open zip archive handle.
 *
 * If the file can't be opened as an archive, the zip archive handle is null.
 */
VuoFileUtilities::Archive::Archive(string path) :
	path(path)
{
	this->referenceCount = 0;

	// mz_zip_reader_init_file sometimes ends up with a garbage function pointer if the mz_zip_archive isn't initialized to zeroes
	this->zipArchive = (mz_zip_archive *)calloc(1, sizeof(mz_zip_archive));

	bool success = mz_zip_reader_init_file(zipArchive, path.c_str(), 0);
	if (! success)
	{
		free(zipArchive);
		zipArchive = NULL;
	}
}

/**
 * Closes the zip archive handle.
 */
VuoFileUtilities::Archive::~Archive(void)
{
	if (zipArchive)
	{
		mz_zip_reader_end(zipArchive);
		free(zipArchive);
	}
}

/**
 * Creates an unset file reference.
 *
 * (This is needed in order to use VuoFileUtilities::File in Q_DECLARE_METATYPE.)
 */
VuoFileUtilities::File::File()
{
}

/**
 * Creates a reference to a file that is in a directory.
 */
VuoFileUtilities::File::File(string dirPath, string filePath) :
	filePath(filePath),
	dirPath(dirPath)
{
	this->fileDescriptor = -1;
	this->archive = NULL;
}

/**
 * Creates a reference to a file that is in an archive.
 *
 * After this constructor is called, it's important to call the destructor.
 * The archive will be closed when all File objects that used it are destroyed.
 */
VuoFileUtilities::File::File(Archive *archive, string filePath) :
	filePath(filePath)
{
	this->fileDescriptor = -1;
	this->archive = archive;
	++archive->referenceCount;
}

/**
 * Creates a reference to a file in the same directory/archive as the current file,
 * but with a different extension.
 */
VuoFileUtilities::File VuoFileUtilities::File::fileWithDifferentExtension(string extension)
{
	string newFilePath = basename() + "." + extension;

	if (archive)
		return File(archive, newFilePath);
	else
		return File(dirPath, newFilePath);
}

/**
 * If this file is in an archive and it was the last File object using the Archive object,
 * closes the archive.
 */
VuoFileUtilities::File::~File(void)
{
	if (archive && --archive->referenceCount == 0)
		delete archive;
}

/**
 * Returns true if this file is inside an archive.
 */
bool VuoFileUtilities::File::isInArchive(void)
{
	return (archive != NULL);
}

/**
 * Returns the path of the archive that contains this file.
 *
 * If this file is not inside an archive, returns an empty string.
 */
string VuoFileUtilities::File::getArchivePath(void)
{
	return (archive ? archive->path : "");
}

/**
 * Returns the path of the file, relative to its enclosing directory or archive.
 */
string VuoFileUtilities::File::getRelativePath(void)
{
	return filePath;
}

/**
 * Returns the absolute path to the file, including the filename.
 *
 * @throw VuoException The file is in an archive.
 */
string VuoFileUtilities::File::path()
{
	if (archive)
		throw VuoException("Can't return a simple absolute path for a file in an archive.");

	return dirPath + "/" + filePath;
}

/**
 * Returns the absolute path to the folder containing the file file.
 *
 * @throw VuoException The file is in an archive.
 */
string VuoFileUtilities::File::dir()
{
	if (archive)
		throw VuoException("Can't return a simple absolute path for a file in an archive.");

	return dirPath;
}

/**
 * Returns the part of the filename before the extension.
 */
string VuoFileUtilities::File::basename()
{
	return filePath.substr(0, filePath.find_last_of('.'));
}

/**
 * Returns the extension part of the filename.
 */
string VuoFileUtilities::File::extension()
{
	return filePath.substr(filePath.find_last_of('.') + 1);
}

/**
 * Returns true if the file exists.
 */
bool VuoFileUtilities::File::exists()
{
	if (archive)
		return mz_zip_reader_locate_file(archive->zipArchive, filePath.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE) != -1;
	else
		return VuoFileUtilities::fileExists((dirPath.empty() ? "" : (dirPath + "/")) + filePath);
}

/**
 * Returns the contents of the file as an array of bytes.
 *
 * @param[out] numBytes The size of the returned array.
 * @return The array of bytes. The caller is responsible for freeing it.
 *
 * @throw VuoException The file couldn't be read.
 */
char * VuoFileUtilities::File::getContentsAsRawData(size_t &numBytes)
{
	char * buffer;
	numBytes = 0;

	if (! archive)
	{
		// http://www.cplusplus.com/reference/cstdio/fread/

		string fullPath = (dirPath.empty() ? "" : (dirPath + "/")) + filePath;

		FILE *pFile = fopen ( fullPath.c_str() , "rb" );
		if (pFile==NULL)
			throw VuoException(string("Couldn't open file: ") + strerror(errno) + " — " + fullPath);

		// obtain file size:
		fseek (pFile , 0 , SEEK_END);
		size_t lSize = ftell (pFile);
		rewind (pFile);

		// allocate memory to contain the whole file:
		buffer = (char*) malloc (sizeof(char)*lSize);

		// copy the file into the buffer:
		numBytes = fread (buffer,1,lSize,pFile);
		fclose(pFile);
		if (numBytes != lSize)
		{
			free(buffer);
			throw VuoException(string("Couldn't read file: ") + strerror(errno) + " — " + fullPath);
		}
	}
	else
	{
		buffer = (char *)mz_zip_reader_extract_file_to_heap(archive->zipArchive, filePath.c_str(), &numBytes, 0);
	}

	return buffer;
}

/**
 * Returns the contents of the file as a string.
 *
 * @throw VuoException The file couldn't be read.
 */
string VuoFileUtilities::File::getContentsAsString(void)
{
	size_t numBytes;
	char *buffer = getContentsAsRawData(numBytes);
	string s(buffer, numBytes);
	free(buffer);
	return s;
}

/**
 * Acquires a lock on the file. While this lock is held, no one else (in this or another process) can hold a lock
 * on the file from lockForWriting(). If a lock from lockFromWriting() is already held for the file, this function
 * downgrades the lock.
 *
 * @param nonBlocking True if the function should return immediately instead of waiting if the lock is not
 *		immediately available.
 * @return True if the lock was acquired.
 */
bool VuoFileUtilities::File::lockForReading(bool nonBlocking)
{
	if (fileDescriptor < 0)
		fileDescriptor = open((dirPath + "/" + filePath).c_str(), O_RDONLY);
	int ret = flock(fileDescriptor, LOCK_SH | (nonBlocking ? LOCK_NB : 0));
	return ret == 0;
}

/**
 * Acquires a lock on the file. While this lock is held, no one else (in this or another process) can hold a lock
 * on the file from lockForReading() or lockForWriting(). If a lock from lockForReading() is already held for the
 * file, this function upgrades the lock.
 *
 * @param nonBlocking True if the function should return immediately instead of waiting if the lock is not
 *		immediately available.
 * @return True if the lock was acquired.
 */
bool VuoFileUtilities::File::lockForWriting(bool nonBlocking)
{
	if (fileDescriptor < 0)
		fileDescriptor = open((dirPath + "/" + filePath).c_str(), O_RDONLY);
	int ret = flock(fileDescriptor, LOCK_EX | (nonBlocking ? LOCK_NB : 0));
	return ret == 0;
}

/**
 * Releases a lock on the file from lockForReading() or lockForWriting(). If a lock has been upgraded from reading
 * to writing, this function fully unlocks the file (rather than downgrading the lock back to reading).
 */
void VuoFileUtilities::File::unlock(void)
{
	flock(fileDescriptor, LOCK_UN);
	close(fileDescriptor);
	fileDescriptor = -1;
}

/**
 * Attempts to focus the specified `pid` (i.e., bring all its windows to the front and make one of them key).
 *
 * @threadMain
 */
void VuoFileUtilities::focusProcess(pid_t pid, bool force)
{
	VuoFileUtilitiesCocoa_focusProcess(pid, force);
}

/**
 * Launches another process and waits for it to finish.
 * If the exit status is nonzero, throws an exception containing stdout and stderr.
 *
 * The optional `environment` argument should consist of strings of the form `key=value`.
 *
 * @throw VuoException
 */
void VuoFileUtilities::executeProcess(vector<string> processAndArgs, vector<string> environment)
{
	string binaryDir, binaryFile, binaryExt;
	splitPath(processAndArgs[0], binaryDir, binaryFile, binaryExt);

	string errorPrefix = "Couldn't execute " + binaryFile + ": ";

	// Capture stdout and stderr.
	int outputPipe[2];
	int ret = pipe(outputPipe);
	if (ret)
		throw VuoException(errorPrefix + "couldn't open pipe: " + strerror(ret));
	int pipeRead  = outputPipe[0];
	int pipeWrite = outputPipe[1];
	VuoDefer(^{ close(pipeRead); });
	posix_spawn_file_actions_t fileActions;
	posix_spawn_file_actions_init(&fileActions);
	posix_spawn_file_actions_adddup2(&fileActions, pipeWrite, STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&fileActions, pipeWrite, STDERR_FILENO);
	posix_spawn_file_actions_addclose(&fileActions, pipeRead);

	// Convert args.
	// posix_spawn requires a null-terminated array, which `&processAndArgs[0]` doesn't guarantee.
	char **processAndArgsZ = (char **)malloc(sizeof(char *) * (processAndArgs.size() + 1));
	int argCount = 0;
	string commandLine;
	for (auto arg : processAndArgs)
	{
		processAndArgsZ[argCount++] = strdup(arg.c_str());
		if (argCount > 0)
			commandLine += ' ';
		commandLine += '"' + arg + '"';
	}
	processAndArgsZ[argCount] = nullptr;


	// Convert environment.
	// posix_spawn requires a null-terminated array, which `&processAndArgs[0]` doesn't guarantee.
	char **environmentZ = (char **)malloc(sizeof(char *) * (environment.size() + 1));
	int environmentVarCount = 0;
	for (auto environmentVar : environment)
	{
		environmentZ[environmentVarCount++] = strdup(environmentVar.c_str());
		commandLine = environmentVar + " " + commandLine;
	}
	environmentZ[environmentVarCount] = nullptr;


	// Execute.
	pid_t pid;
	VDebugLog("%s", commandLine.c_str());
	ret = posix_spawn(&pid, processAndArgs[0].c_str(), &fileActions, NULL, (char * const *)processAndArgsZ, (char * const *)environmentZ);
	close(pipeWrite);
	posix_spawn_file_actions_destroy(&fileActions);
	for (int i = 0; i < argCount; ++i)
		free(processAndArgsZ[i]);

	if (ret)
		throw VuoException(errorPrefix + strerror(ret));
	else
	{
		int ret;
		int status;
		while (true)
		{
			ret = waitpid(pid, &status, 0);
			if (ret == -1 && errno == EINTR)
				// This process received a signal while waiting for the other process
				// (seems to happen when running under lldb); try waiting again.
				continue;
			else if (ret == -1)
				throw VuoException(errorPrefix + "waitpid failed: " + strerror(errno));
			else if (status != 0)
			{
				string output;
				char buf[256];
				bzero(buf, 256);
				while (read(pipeRead, &buf, 255) > 0)
				{
					output += buf;
					bzero(buf, 256);
				}

				throw VuoException(binaryFile + " failed: " + output);
			}
			else
				// The other process completed without error.
				break;
		}
	}
}

/**
 * Returns true if the given extension (without leading dot) is a Vuo composition file.
 */
bool VuoFileUtilities::isCompositionExtension(string extension)
{
	return extension == "vuo";
}

/**
 * Returns C/C++/Objective-C/Objective-C++ source file extensions (without a leading dot).
 */
set<string> VuoFileUtilities::getCFamilySourceExtensions()
{
	set<string> e;
	e.insert("c");
	e.insert("cc");
	e.insert("C");
	e.insert("cpp");
	e.insert("cxx");
	e.insert("c++");
	e.insert("m");
	e.insert("mm");
	return e;
}

/**
 * Returns true if the given extension (without leading dot) is a C/C++/Objective-C/Objective-C++ source file.
 */
bool VuoFileUtilities::isCFamilySourceExtension(string extension)
{
	auto extensions = getCFamilySourceExtensions();
	return extensions.find(extension) != extensions.end();
}

/**
 * Returns true if the given extension (without leading dot) is for a C++ source file.
 */
bool VuoFileUtilities::isCPlusPlusSourceExtension(string extension)
{
	set<string> e = { "cc", "cpp", "cxx", "c++" };
	return e.find(extension) != e.end();
}

/**
 * Returns true if the given extension (without leading dot) is for an Objective-C source file.
 */
bool VuoFileUtilities::isObjectiveCSourceExtension(string extension)
{
	return extension == "m";
}

/**
 * Returns true if the given extension (without leading dot) is for an Objective-C++ source file.
 */
bool VuoFileUtilities::isObjectiveCPlusPlusSourceExtension(string extension)
{
	return extension == "mm";
}

/**
 * Returns ISF source file extensions (without a leading dot).
 */
set<string> VuoFileUtilities::getIsfSourceExtensions(void)
{
	set<string> e;
	e.insert("fs");
	e.insert("vs");
	e.insert("vuoshader");
	e.insert("vuoobjectfilter");
	return e;
}

/**
 * Returns true if the given extension (without leading dot) is an ISF source file.
 */
bool VuoFileUtilities::isIsfSourceExtension(string extension)
{
	auto extensions = getIsfSourceExtensions();
	return extensions.find(extension) != extensions.end();
}
