/**
 * @file
 * VuoModule interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoNodeSet;

/**
 * A modular component (i.e., node class or type) that is an add-on to the core Vuo framework.
 */
class VuoModule
{
public:
	VuoModule(string moduleKey);
	~VuoModule();

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
	void *p;
#pragma clang diagnostic pop
#if VUO_PRO
#include "../base/pro/VuoModule_Pro.hh"
#endif
};
