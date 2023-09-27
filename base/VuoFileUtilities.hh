/**
 * @file
 * VuoFileUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include "miniz.h"
#pragma clang diagnostic pop

#import <string>
#import <set>
using namespace std;

#include "VuoHeap.h"

/**
 * Functions for dealing with files.
 */
class VuoFileUtilities
{
public:
	/**
	 * An archive of files.
	 */
	class Archive
	{
	public:
		mz_zip_archive *zipArchive;  ///< An open zip archive handle, or NULL if the zip archive failed to open.
		int referenceCount;  ///< Used by callers to keep track of when the zip archive handle should be closed.
		string path;  ///< The path of the archive file.
		Archive(string path);
		~Archive(void);
	};

	/**
	 * A file in either a directory or an archive.
	 */
	class File
	{
	private:
		string filePath;  ///< The path of the file, relative to its directory or archive.
		string dirPath;  ///< The path of the directory, or empty if the file is in an archive.
		Archive *archive;  ///< The archive reference, or null if the file is in a directory.
		int fileDescriptor;

		friend class VuoFileUtilities;

	public:
		File();
		File(string dirPath, string filePath);
		File(Archive *archive, string filePath);
		~File(void);

		File fileWithDifferentExtension(string extension);

		bool isInArchive(void);
		string getArchivePath(void);
		string getRelativePath(void);
		string path();
		string dir();
		string basename();
		string extension();

		bool exists();
		char * getContentsAsRawData(size_t &numBytes);
		string getContentsAsString(void);

		bool lockForReading(bool nonBlocking=false) VuoWarnUnusedResult;
		bool lockForWriting(bool nonBlocking=false) VuoWarnUnusedResult;
		void unlock(void);
	};

	static void splitPath(string path, string &dir, string &file, string &extension);
	static void canonicalizePath(string &path);
	static bool arePathsEqual(string path1, string path2);
	static string getAbsolutePath(const string &path);
	static bool isAbsolutePath(const string &path);
	static string makeTmpFile(string file, string extension, string directory="");
	static string makeTmpDir(string prefix);
	static string makeTmpDirOnSameVolumeAsPath(string path);
	static string getTmpDir(void);
	static void makeDir(string path);
	static string getVuoFrameworkPath(void);
	static string getVuoRunnerFrameworkPath(void);
	static string getCompositionLocalModulesPath(const string &compositionPath);
	static string getUserModulesPath(void);
	static string getSystemModulesPath(void);
	static string getCachePath(void);
	static bool isInstalledAsModule(const string &path);
	static void preserveOriginalFileName(string &fileContents, string originalFileName);
	static size_t getFirstInsertionIndex(string s);
	static string readStdinToString(void);
	static string readFileToString(string path);
	static void writeRawDataToFile(const char *data, size_t numBytes, string file);
	static void writeStringToFile(string s, string file);
	static void writeStringToFileSafely(string s, string path);
	static bool fileExists(string path);
	static bool dirExists(string path);
	static bool isSymlink(string path);
	static bool fileIsReadable(string path);
	static bool fileContainsReadableData(string path);
	static void createFile(string path);
	static void deleteFile(string path);
	static void deleteDir(string path);
	static void moveFile(string fromPath, string toPath);
	static void moveFileToTrash(string filePath);
	static void copyFile(string fromPath, string toPath, bool preserveMetadata = false);
	static void copyDirectory(string fromPath, string toPath);
	static string calculateFileSHA256(const string &path);
	static double getFileLastModifiedInSeconds(string path);
	static double getSecondsSinceFileLastAccessed(string path);
	static set<File *> findAllFilesInDirectory(string dirPath, set<string> archiveExtensions = set<string>(), bool shouldSearchRecursively = false);
	static set<File *> findFilesInDirectory(string dirPath, set<string> extensions, set<string> archiveExtensions = set<string>());
	static set<File *> findAllFilesInArchive(string archivePath);
	static set<File *> findFilesInArchive(string archivePath, string dirPath, set<string> extensions);
	static string getArchiveFileContentsAsString(string archivePath, string filePath);
	static size_t getAvailableSpaceOnVolumeContainingPath(string path);
	static void adHocCodeSign(string path, vector<string> environment, string entitlementsPath="");

	static void focusProcess(pid_t pid, bool force=false);
	static void executeProcess(vector<string> processAndArgs, vector<string> environment = {});

	static bool isCompositionExtension(string extension);
	static set<string> getCFamilySourceExtensions(void);
	static bool isCFamilySourceExtension(string extension);
	static bool isCPlusPlusSourceExtension(string extension);
	static bool isObjectiveCSourceExtension(string extension);
	static bool isObjectiveCPlusPlusSourceExtension(string extension);
	static set<string> getIsfSourceExtensions(void);
	static bool isIsfSourceExtension(string extension);

private:
	static void dylibLoaded(const struct mach_header *mh32, intptr_t vmaddr_slide);
	static void initializeVuoFrameworkPaths(void);
	static bool dylibLoaderInitialMatchCompleted;
	static string vuoFrameworkPath;
	static string vuoRunnerFrameworkPath;
};
