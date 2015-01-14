/**
 * @file
 * VuoNodeSet interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONODESET_HH
#define VUONODESET_HH

#include "VuoFileUtilities.hh"
#include "VuoModule.hh"

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
	vector<string> getExampleCompositionFileNames(void);
	string getExampleCompositionContents(string exampleCompositionFileName);

private:
	VuoNodeSet(string archivePath);
	string getDescriptionFromFile(string fileName);

	string archivePath;  ///< The path of the archive file that contains this node set.
	map<string, string> cachedDescriptions;  ///< Maps file names in the `descriptions` folder to file contents.
};

#endif
