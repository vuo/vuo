/**
 * @file
 * VuoNodeSet implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoModule.hh"
#include "VuoNodeSet.hh"

/**
 * Creates a VuoNodeSet for the given node set archive file.
 */
VuoNodeSet::VuoNodeSet(string archivePath)
{
	this->archivePath = archivePath;
}

/**
 * Creates a VuoNodeSet for the archive that contains the given file.
 *
 * If the given file is not contained by an archive, returns null.
 */
VuoNodeSet * VuoNodeSet::createNodeSetForModule(VuoFileUtilities::File *moduleFile)
{
	if (moduleFile->isInArchive())
		return new VuoNodeSet(moduleFile->getArchivePath());
	else
		return NULL;
}

/**
 * Returns a name for this node set, based on the node set archive file name.
 */
string VuoNodeSet::getName(void)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(archivePath, dir, file, ext);
	return file;
}

/**
 * Returns the contents of a file in this node set's `descriptions` folder.
 *
 * The first time this function is called for a file, the file's contents are cached.
 * The cached results are returned in subsequent calls.
 */
string VuoNodeSet::getDescriptionFromFile(string fileName)
{
	string description;
	map<string, string>::iterator cachedIter = cachedDescriptions.find(fileName);
	if (cachedIter == cachedDescriptions.end())
	{
		description = VuoFileUtilities::getArchiveFileContentsAsString(archivePath, "descriptions/" + fileName);
		cachedDescriptions[fileName] = description;
	}
	else
	{
		description = cachedIter->second;
	}
	return description;
}

/**
 * Returns this node set's description.
 *
 * If this node set doesn't have a description, returns an empty string.
 */
string VuoNodeSet::getDescription(void)
{
	return getDescriptionFromFile(getName() + ".md");
}

/**
 * Returns the description for the module if it's provided in a separate file within the node set.
 *
 * Otherwise, returns an empty string.
 */
string VuoNodeSet::getDescriptionForModule(VuoModule *module)
{
	return getDescriptionFromFile(module->getModuleKey() + ".md");
}

/**
 * Returns the names of the node classes within this node set, in lexicographic order.
 */
vector<string> VuoNodeSet::getNodeClassNames(void)
{
	set<string> extensions;
	extensions.insert("vuonode");
	extensions.insert("vuonode+");
	set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInArchive(archivePath, "", extensions);

	vector<string> classNames;
	for (set<VuoFileUtilities::File *>::iterator i = files.begin(); i != files.end(); ++i)
	{
		VuoFileUtilities::File *f = *i;
		string dir, file, ext;
		VuoFileUtilities::splitPath(f->getRelativePath(), dir, file, ext);
		classNames.push_back(file);
		delete f;
	}

	std::sort(classNames.begin(), classNames.end());

	return classNames;
}

/**
 * Returns the file names of the header files within this node set.
 */
vector<string> VuoNodeSet::getHeaderFileNames(void)
{
	set<string> extensions;
	extensions.insert("h");
	set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInArchive(archivePath, "", extensions);

	vector<string> headerFileNames;
	for (set<VuoFileUtilities::File *>::iterator i = files.begin(); i != files.end(); ++i)
	{
		VuoFileUtilities::File *f = *i;
		string dir, file, ext;
		VuoFileUtilities::splitPath(f->getRelativePath(), dir, file, ext);
		headerFileNames.push_back(file + "." + ext);
		delete f;
	}

	return headerFileNames;
}

/**
 * Returns the file names of the example compositions within this node set, in lexicographic order.
 */
vector<string> VuoNodeSet::getExampleCompositionFileNames(void)
{
	set<string> extensions;
	extensions.insert("vuo");
	set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInArchive(archivePath, "examples", extensions);

	vector<string> fileNames;
	for (set<VuoFileUtilities::File *>::iterator i = files.begin(); i != files.end(); ++i)
	{
		VuoFileUtilities::File *f = *i;
		string dir, file, ext;
		VuoFileUtilities::splitPath(f->getRelativePath(), dir, file, ext);
		fileNames.push_back(file + "." + ext);
		delete f;
	}

	std::sort(fileNames.begin(), fileNames.end());

	return fileNames;
}

/**
 * Returns the contents of a node class source file within this node set.
 *
 * If the file doesn't exist, returns an empty string.
 */
string VuoNodeSet::getNodeClassContents(string nodeClassName)
{
	return VuoFileUtilities::getArchiveFileContentsAsString(archivePath, nodeClassName + ".c");
}

/**
 * Returns the contents of a port type header file within this node set.
 *
 * If the file doesn't exist, returns an empty string.
 */
string VuoNodeSet::getHeaderContents(string headerName)
{
	return VuoFileUtilities::getArchiveFileContentsAsString(archivePath, headerName);
}

/**
 * Returns the contents of an example composition within this node set.
 *
 * If the example composition doesn't exist, returns an empty string.
 */
string VuoNodeSet::getExampleCompositionContents(string exampleCompositionFileName)
{
	return VuoFileUtilities::getArchiveFileContentsAsString(archivePath, "examples/" + exampleCompositionFileName);
}

/**
 * Extracts the resources (images, etc.) for the example compositions from the node set archive.
 */
void VuoNodeSet::extractExampleCompositionResources(string destinationDir)
{
	extractResourcesFromSubdirectory("examples", destinationDir);
}

/**
 * Extracts the resources (images, etc.) for the documentation from the node set archive.
 */
void VuoNodeSet::extractDocumentationResources(string destinationDir)
{
	extractResourcesFromSubdirectory("descriptions", destinationDir);
}

/**
 * Extracts the resources (images, etc.) from the provided @c archiveSubdir of the node set archive
 * into the provided @c destinationDir.
 *
 * Helper function for VuoNodeSet::extractExampleCompositionResources(string destinationDir) and
 * VuoNodeSet::extractDocumentationResources(string destinationDir).
 */
void VuoNodeSet::extractResourcesFromSubdirectory(string archiveSubdir, string destinationDir)
{
	set<string> extensions;
	extensions.insert("png");
	extensions.insert("jpg");
	extensions.insert("mov");
	extensions.insert("mp3");
	extensions.insert("data");
	extensions.insert("3ds");
	extensions.insert("dae");
	extensions.insert("csv");
	set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInArchive(archivePath, archiveSubdir, extensions);

	for (set<VuoFileUtilities::File *>::iterator i = files.begin(); i != files.end(); ++i)
	{
		VuoFileUtilities::File *f = *i;

		string dir, file, ext;
		VuoFileUtilities::splitPath(f->getRelativePath(), dir, file, ext);
		string destinationFile = destinationDir + "/" + file + "." + ext;

		if (! VuoFileUtilities::fileExists(destinationFile))
		{
			size_t numBytes = 0;
			char *data = f->getContentsAsRawData(numBytes);
			VuoFileUtilities::writeRawDataToFile(data, numBytes, destinationFile);
		}

		delete f;
	}
}
