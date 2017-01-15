/**
 * @file
 * VuoFileUtilities interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOFILEUTILITIES_H
#define VUOFILEUTILITIES_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include "miniz.h"
#pragma clang diagnostic pop

#import <string>
#import <set>
using namespace std;

/**
 * Functions for dealing with files.
 */
class VuoFileUtilities
{
private:
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

public:
	/**
	 * A file in either a directory or an archive.
	 */
	class File
	{
	private:
		File(Archive *archive, string filePath);

		string filePath;  ///< The path of the file, relative to its directory or archive.
		string dirPath;  ///< The path of the directory, or empty if the file is in an archive.
		Archive *archive;  ///< The archive reference, or null if the file is in a directory.
		int fileDescriptor;

		friend class VuoFileUtilities;

	public:
		File(string dirPath, string filePath);
		~File(void);
		bool isInArchive(void);
		string getArchivePath(void);
		string getRelativePath(void);
		char * getContentsAsRawData(size_t &numBytes);
		string getContentsAsString(void);
		bool lockForReading(bool nonBlocking=false);
		bool lockForWriting(bool nonBlocking=false);
		void unlock(void);
	};

	static void splitPath(string path, string &dir, string &file, string &extension);
	static string makeTmpFile(string file, string extension, string directory="/tmp");
	static string makeTmpDir(string dir);
	static string getTmpDir(void);
	static void makeDir(string path);
	static string getVuoFrameworkPath(void);
	static string getUserModulesPath(void);
	static string getSystemModulesPath(void);
	static string getCachePath(void);
	static void preserveOriginalFileName(string &fileContents, string originalFileName);
	static size_t getFirstInsertionIndex(string s);
	static string readStdinToString(void);
	static string readFileToString(string path);
	static void writeRawDataToFile(const char *data, size_t numBytes, string file);
	static void writeStringToFile(string s, string file);
	static bool fileExists(string path);
	static bool dirExists(string path);
	static bool fileIsReadable(string path);
	static bool fileContainsReadableData(string path);
	static void createFile(string path);
	static void deleteFile(string path);
	static void moveFile(string fromPath, string toPath);
	static void moveFileToTrash(string filePath);
	static void copyFile(string fromPath, string toPath);
	static unsigned long getFileLastModifiedInSeconds(string path);
	static set<File *> findAllFilesInDirectory(string dirPath, set<string> archiveExtensions = set<string>(), bool shouldSearchRecursively = false);
	static set<File *> findFilesInDirectory(string dirPath, set<string> extensions, set<string> archiveExtensions = set<string>());
	static set<File *> findAllFilesInArchive(string archivePath);
	static set<File *> findFilesInArchive(string archivePath, string dirPath, set<string> extensions);
	static string getArchiveFileContentsAsString(string archivePath, string filePath);
};

#endif
