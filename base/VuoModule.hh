/**
 * @file
 * VuoModule interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMODULE_HH
#define VUOMODULE_HH

#include "VuoBase.hh"
#include "VuoModule.hh"

class VuoCompilerModule;

/**
 * A modular component (i.e., node class or type) that is an add-on to the core Vuo framework.
 */
class VuoModule
{
public:
	VuoModule(string moduleKey);

	string getModuleKey(void);

	string getDefaultTitle(void);
	void setDefaultTitle(string defaultTitle);

	string getDescription(void);
	void setDescription(string description);

	string getVersion(void);
	void setVersion(string version);

	vector<string> getKeywords(void);
	void setKeywords(vector<string> keywords);

private:
	string moduleKey;
	string defaultTitle;
	string description;
	string version;
	vector<string> keywords;

};

#endif // VUOMODULE_HH
