/**
 * @file
 * VuoFileUtilities implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <libgen.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <mach-o/dyld.h>
#include "VuoFileUtilities.hh"
#include "VuoFileUtilitiesCocoa.hh"
#include "VuoStringUtilities.hh"


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
 * Returns the path of the directory (without a trailing slash).
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
 * Returns the path of the default temporary directory.
 */
string VuoFileUtilities::getTmpDir(void)
{
	return "/tmp";
}

/**
 * Creates a new directory (and parent directories if needed), if it doesn't already exist.
 */
void VuoFileUtilities::makeDir(string path)
{
	if (dirExists(path))
		return;

	const char FILE_SEPARATOR = '/';
	size_t lastNonSeparator = path.find_last_not_of(FILE_SEPARATOR);
	if (lastNonSeparator != string::npos)
		path.resize(lastNonSeparator + 1);

	string parentDir, file, ext;
	splitPath(path, parentDir, file, ext);
	makeDir(parentDir);

	int ret = mkdir(path.c_str(), 0700);
	if (ret != 0 && ! dirExists(path))
		throw std::runtime_error("Couldn't create directory \"" + path + "\"");
}

/**
 * Returns the absolute path of Vuo.framework (without a trailing slash),
 * or an empty string if Vuo.framework cannot be located.
 */
string VuoFileUtilities::getVuoFrameworkPath(void)
{
	// First check whether Vuo.framework is in the list of loaded dynamic libraries.
	const char *frameworkDylibRelativePath = "Vuo.framework/Versions/" VUO_VERSION_STRING "/Vuo";
	for(unsigned int i=0; i<_dyld_image_count(); ++i)
	{
		const char *dylibPath = _dyld_get_image_name(i);
		if (VuoStringUtilities::endsWith(dylibPath, frameworkDylibRelativePath))
		{
			string path = dylibPath;
			string dir, file, ext;
			splitPath(path, dir, file, ext);		// remove "Vuo"
			path = dir.substr(0, dir.length() - 1);	// remove "/"
			splitPath(path, dir, file, ext);		// remove VUO_VERSION_STRING
			path = dir.substr(0, dir.length() - 1);	// remove "/"
			splitPath(path, dir, file, ext);		// remove "Versions"
			path = dir.substr(0, dir.length() - 1);	// remove "/"
			if (fileExists(path))
				return path;
		}
	}

	// Failing that, check for a Vuo.framework bundled with the executable app.
	char executablePath[PATH_MAX + 1];
	uint32_t size = sizeof(executablePath);

	if (! _NSGetExecutablePath(executablePath, &size))
	{
		char cleanedExecutablePath[PATH_MAX + 1];

		realpath(executablePath, cleanedExecutablePath);  // remove extra references (e.g., "/./")
		string path = cleanedExecutablePath;
		string dir, file, ext;
		splitPath(path, dir, file, ext);		// remove executable name
		path = dir.substr(0, dir.length() - 1);	// remove "/"
		splitPath(path, dir, file, ext);		// remove "MacOS"
		path = dir.substr(0, dir.length() - 1);	// remove "/"
		path += "/Frameworks/Vuo.framework";

		if (fileExists(path))
			return path;
	}

	// Failing that, check for ~/Library/Frameworks/Vuo.framework.
	string userFrameworkPath = string(getenv("HOME")) + "/Library/Frameworks/Vuo.framework";
	if (fileExists(userFrameworkPath))
		return userFrameworkPath;

	// Failing that, check for /Library/Frameworks/Vuo.framework.
	string systemFrameworkPath = "/Library/Frameworks/Vuo.framework";
	if (fileExists(systemFrameworkPath))
		return systemFrameworkPath;

	// Give up.
	return "";
}

/**
 * Returns the filesystem path to the user-specific Vuo Modules folder (without a trailing slash).
 */
string VuoFileUtilities::getUserModulesPath()
{
	return string(getenv("HOME")) + "/Library/Application Support/Vuo/Modules";
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
 */
string VuoFileUtilities::getCachePath(void)
{
	return string(getenv("HOME")) + "/Library/Caches/org.vuo/" + VUO_VERSION_AND_BUILD_STRING;
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
	char bomUtf8[] = { 0xEF, 0xBB, 0xBF };
	char bomUtf16Be[] = { 0xFE, 0xFF };
	char bomUtf16Le[] = { 0xFF, 0xFE };
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
	// http://stackoverflow.com/questions/201992/how-to-read-until-eof-from-cin-in-c

	string contents;
	string line;
	while (getline(cin, line))
	   contents += line;

	return contents;
}

/**
 * Reads the whole contents of the file into a string.
 */
string VuoFileUtilities::readFileToString(string path)
{
	// http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

	ifstream in(path.c_str(), ios::in | ios::binary);
	if (! in)
	{
		VUserLog("Error: Couldn't read file at path '%s'.", path.c_str());
		return "";
	}

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
 * @throw std::runtime_error The file couldn't be written.
 */
void VuoFileUtilities::writeRawDataToFile(const char *data, size_t numBytes, string file)
{
	FILE *f = fopen(file.c_str(), "wb");
	if (! f)
		throw std::runtime_error("Couldn't open for writing file '" + file + "' : " + strerror(errno));

	size_t numBytesWritten = fwrite(data, sizeof(char), numBytes, f);
	if (numBytesWritten != numBytes)
		throw std::runtime_error("Couldn't write all data to file '" + file + "'");

	fclose(f);
}

/**
 * Writes the string to the file.
 *
 * If the file already exists, it gets overwritten.
 *
 * @throw std::runtime_error The file couldn't be written.
 */
void VuoFileUtilities::writeStringToFile(string s, string file)
{
	writeRawDataToFile(s.data(), s.length(), file);
}

/**
 * Returns true if the file exists.
 */
bool VuoFileUtilities::fileExists(string path)
{
	return access(path.c_str(), 0) == 0;
}

/**
 * Returns true if the file exists and is a directory.
 */
bool VuoFileUtilities::dirExists(string path)
{
	struct stat st_buf;
	int status = lstat(path.c_str(), &st_buf);  // Unlike stat, lstat doesn't follow symlinks.
	return (! status && S_ISDIR(st_buf.st_mode));
}

/**
 * Returns true if the file is readable.
 */
bool VuoFileUtilities::fileIsReadable(string path)
{
	return access(path.c_str(), R_OK) == 0;
}

/**
 * Returns true if the file exists, can be opened, and has a size of more than 0 bytes.
 */
bool VuoFileUtilities::fileContainsReadableData(string path)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	long pos = ftell(f);
	fclose(f);
	return pos > 0;
}

/**
 * Creates the file if it does not exist already; otherwise, has no effect on the file.
 */
void VuoFileUtilities::createFile(string path)
{
	FILE *f = fopen(path.c_str(), "a");
	fclose(f);
}

/**
 * Deletes the file if it exists; otherwise, has no effect.
 */
void VuoFileUtilities::deleteFile(string path)
{
	remove(path.c_str());
}

/**
 * Moves the file from @a fromPath to @a toPath.
 *
 * @throw std::runtime_error The file couldn't be moved.
 */
void VuoFileUtilities::moveFile(string fromPath, string toPath)
{
	int ret = rename(fromPath.c_str(), toPath.c_str());
	if (ret != 0)
		throw std::runtime_error("Couldn't move file '" + fromPath + "' to '" + toPath + "'");
}

/**
 * Moves the specified file to the user's trash folder.
 *
 * @throw std::runtime_error The file couldn't be moved.
 */
void VuoFileUtilities::moveFileToTrash(string filePath)
{
	VuoFileUtilitiesCocoa_moveFileToTrash(filePath);
}

/**
 * Copies the file from @a fromPath to @a toPath.
 *
 * @throw std::runtime_error The file couldn't be copied.
 */
void VuoFileUtilities::copyFile(string fromPath, string toPath)
{
	string contents = readFileToString(fromPath);
	writeStringToFile(contents, toPath);
}

/**
 * Returns the time that the file was last modified, in seconds from a reference date.
 * This is useful for checking which of two files was modified more recently.
 */
unsigned long VuoFileUtilities::getFileLastModifiedInSeconds(string path)
{
	struct stat s;
	lstat(path.c_str(), &s);
	return s.st_mtimespec.tv_sec;  // s.st_mtimespec.tv_nsec is always 0 on some OSes, hence the resolution of 1 second
}

/**
 * Searches a directory for files.
 *
 * @param dirPath The directory to search in. Only the top level is searched.
 * @param archiveExtensions The file extensions for archives to search in. Any archive with one of these extensions
 *		found in the top level of the directory will be searched recursively.
 * @param shouldSearchRecursively If true, the directory will be searched searched recursively.
 * @return All files found.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findAllFilesInDirectory(string dirPath, set<string> archiveExtensions,
																		bool shouldSearchRecursively)
{
	set<File *> files;

	DIR *d = opendir(dirPath.c_str());
	if (! d)
	{
		if (access(dirPath.c_str(), F_OK) != -1)
			VUserLog("Error: Couldn't open directory '%s' to find files in it.", dirPath.c_str());
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
			bool shouldSearchDir = shouldSearchRecursively && dirExists(relativeFilePath);
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

		if (endsWithExtension)
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
 * Creates an archive with an open zip archive handle.
 *
 * If the file can't be opened as an archive, the zip archive handle is null.
 */
VuoFileUtilities::Archive::Archive(string path)
{
	this->path = path;
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
		mz_zip_reader_end(zipArchive);
}

/**
 * Creates a reference to a file that is in a directory.
 */
VuoFileUtilities::File::File(string dirPath, string filePath)
{
	this->filePath = filePath;
	this->dirPath = dirPath;
	this->fileDescriptor = -1;
	this->archive = NULL;
}

/**
 * Creates a reference to a file that is in an archive.
 *
 * After this constructor is called, it's important to call the destructor.
 * The archive will be closed when all File objects that used it are destroyed.
 */
VuoFileUtilities::File::File(Archive *archive, string filePath)
{
	this->filePath = filePath;
	this->fileDescriptor = -1;
	this->archive = archive;
	++archive->referenceCount;
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
 * Returns the contents of the file as an array of bytes.
 *
 * @param[out] numBytes The size of the returned array.
 * @return The array of bytes. The caller is responsible for freeing it.
 *
 * @throw std::runtime_error The file couldn't be read.
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
			throw std::runtime_error("Couldn't open file '" + fullPath + "' for reading");

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
			throw std::runtime_error("Couldn't read file '" + fullPath + "'");
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
 * @throw std::runtime_error The file couldn't be read.
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
