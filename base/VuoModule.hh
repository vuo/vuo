/**
 * @file
 * VuoModule interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMODULE_HH
#define VUOMODULE_HH

#include "VuoBase.hh"
#include "VuoModule.hh"

class VuoNodeSet;
class VuoCompilerModule;

/**
 * A modular component (i.e., node class or type) that is an add-on to the core Vuo framework.
 */
class VuoModule
{
public:
	VuoModule(string moduleKey);

	string getModuleKey(void);
	void setModuleKey(string moduleKey);

	string getDefaultTitle(void);
	string getDefaultTitleWithoutSuffix(void);
	void setDefaultTitle(string defaultTitle);

	string getDescription(void);
	void setDescription(string description);

	string getVersion(void);
	void setVersion(string version);

	vector<string> getKeywords(void);
	void setKeywords(vector<string> keywords);

	VuoNodeSet * getNodeSet(void);
	void setNodeSet(VuoNodeSet *nodeSet);

private:
	string moduleKey;
	string defaultTitle;
	string description;
	string version;
	vector<string> keywords;
	VuoNodeSet *nodeSet;

};

#endif // VUOMODULE_HH
