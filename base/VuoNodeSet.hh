/**
 * @file
 * VuoNodeSet interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONODESET_HH
#define VUONODESET_HH

#include "VuoFileUtilities.hh"

class VuoModule;

/**
 * This class represents a node set, which is a set of node classes and supporting files
 * grouped together in an archive.
 */
class VuoNodeSet
{
public:
	static VuoNodeSet * createNodeSetForModule(VuoFileUtilities::File *moduleFile);
	string getName(void);
	string getDescription(void);
	string getDescriptionForModule(VuoModule *module);
	vector<string> getNodeClassNames(void);
	vector<string> getHeaderFileNames(void);
	vector<string> getExampleCompositionFileNames(void);
	string getNodeClassContents(string nodeClassName);
	string getHeaderContents(string headerName);
	string getExampleCompositionContents(string exampleCompositionFileName);
	void extractExampleCompositionResources(string destinationDir);
	void extractDocumentationResources(string destinationDir);

private:
	VuoNodeSet(string archivePath);
	string getDescriptionFromFile(string fileName);
	void extractResourcesFromSubdirectory(string archiveSubdir, string destinationDir);

	string archivePath;  ///< The path of the archive file that contains this node set.
	map<string, string> cachedDescriptions;  ///< Maps file names in the `descriptions` folder to file contents.
};

#endif
