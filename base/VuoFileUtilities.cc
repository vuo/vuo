/**
 * @file
 * VuoFileUtilities implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
 * Returns the path of the default temporary directory.
 */
string VuoFileUtilities::getTmpDir(void)
{
	return "/tmp";
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
 * Reads from the file handle into a string.
 *
 * @param file An open file handle. It may be a regular file or a pipe.
 * @return The characters read from the file handle until the end of file was reached.
 */
string VuoFileUtilities::cFileToString(FILE *file)
{
	string fileContents;
	while (true)
	{
		char c = getc(file);
		if (c == EOF)
			break;
		fileContents += c;
	}

	return fileContents;
}

/**
 * Writes the array of bytes to the file.
 *
 * If the file already exists, it gets overwritten.
 */
void VuoFileUtilities::writeRawDataToFile(const char *data, size_t numBytes, string file)
{
	FILE *f = fopen(file.c_str(), "wb");
	fwrite(data, sizeof(char), numBytes, f);
	fclose(f);
}

/**
 * Writes the string to the file.
 *
 * If the file already exists, it gets overwritten.
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
 * Searches a directory for files.
 *
 * @param dirPath The directory to search in. Only the top level is searched.
 * @param archiveExtensions The file extensions for archives to search in. Any archive with one of these extensions
 *		found in the top level of the directory will be searched recursively.
 * @return All files found.
 */
set<VuoFileUtilities::File *> VuoFileUtilities::findAllFilesInDirectory(string dirPath, set<string> archiveExtensions)
{
	/// @todo Search recursively - https://b33p.net/kosada/node/2468

	set<File *> files;

	DIR *d = opendir(dirPath.c_str());
	if (! d)
	{
		if (access(dirPath.c_str(), F_OK) != -1)
			fprintf(stderr, "Couldn't open directory '%s' to find files in it.\n", dirPath.c_str());
		return files;
	}

	struct dirent *de;
	while( (de=readdir(d)) )
	{
		string fileName = de->d_name;

		bool isArchive = false;
		for (set<string>::iterator archiveExtension = archiveExtensions.begin(); archiveExtension != archiveExtensions.end(); ++archiveExtension)
			if (VuoStringUtilities::endsWith(fileName, "." + *archiveExtension))
				isArchive = true;

		if (isArchive)
		{
			set<File *> fs = findAllFilesInArchive(dirPath + "/" + fileName);
			if (fs.empty())
				isArchive = false;
			else
				files.insert(fs.begin(), fs.end());
		}

		if (! isArchive)
		{
			File *f = new File(dirPath, fileName);
			files.insert(f);
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
			fprintf(stderr, "Couldn't read file %u in zip archive %s\n", i, archive->path.c_str());
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
 */
char * VuoFileUtilities::File::getContentsAsRawData(size_t &numBytes)
{
	char * buffer;

	if (! archive)
	{
		// http://www.cplusplus.com/reference/cstdio/fread/

		string fullPath = (dirPath.empty() ? "" : (dirPath + "/")) + filePath;

		FILE *pFile = fopen ( fullPath.c_str() , "rb" );
		if (pFile==NULL)
		{
			fprintf(stderr, "Couldn't open file '%s' for reading\n", fullPath.c_str());
			return NULL;
		}

		// obtain file size:
		fseek (pFile , 0 , SEEK_END);
		long lSize = ftell (pFile);
		rewind (pFile);

		// allocate memory to contain the whole file:
		buffer = (char*) malloc (sizeof(char)*lSize);

		// copy the file into the buffer:
		numBytes = fread (buffer,1,lSize,pFile);
		fclose(pFile);
		if (numBytes != lSize)
		{
			fprintf(stderr, "Couldn't read file '%s'\n", fullPath.c_str());
			free(buffer);
			return NULL;
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
 */
string VuoFileUtilities::File::getContentsAsString(void)
{
	size_t numBytes;
	char *buffer = getContentsAsRawData(numBytes);
	string s(buffer, numBytes);
	free(buffer);
	return s;
}
