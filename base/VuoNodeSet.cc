/**
 * @file
 * VuoNodeSet implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

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
	return getDescriptionFromFile("index.md");
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
 * Returns the contents of an example composition within this node set.
 *
 * If the example composition doesn't exist, returns an empty string.
 */
string VuoNodeSet::getExampleCompositionContents(string exampleCompositionFileName)
{
	return VuoFileUtilities::getArchiveFileContentsAsString(archivePath, "examples/" + exampleCompositionFileName);
}
