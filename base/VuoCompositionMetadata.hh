/**
 * @file
 * VuoCompositionMetadata interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Information about a composition to tell people where it came from, what it does, and so on.
 */
class VuoCompositionMetadata
{
public:
	VuoCompositionMetadata(void);
	VuoCompositionMetadata(const string &compositionAsString);
	string toCompositionHeader(void);

	void setName(string name);
	void setDefaultName(const string &defaultName);
	string getName(void);
	string getCustomizedName(void);
	string getDefaultName(void);

	void setAuthor(string author);
	string getAuthor(void);

	void setVersion(string version);
	string getVersion(void);

	void setCreatedInVuoVersion(string version);
	string getCreatedInVuoVersion(void);

	void setLastSavedInVuoVersion(string version);
	string getLastSavedInVuoVersion(void);

	void setDescription(string description);
	string getDescription(void);

	void setKeywords(const vector<string> &keywords);
	vector<string> getKeywords(void);

	void setCopyright(string copyright);
	string getCopyright(void);

	void setLicense(string license);
	string getLicense(void);

	void setIconURL(string url);
	string getIconURL(void);

	void setHomepageURL(string url);
	string getHomepageURL(void);

	void setDocumentationURL(string url);
	string getDocumentationURL(void);

	void setBundleIdentifier(string id);
	string getBundleIdentifier(void);

	void setFxPlugGroup(string group);
	string getFxPlugGroup(void);

	void setCategories(const vector<string> &categories);
	vector<string> getCategories(void);

private:
	string defaultName;
	string name;
	string author;
	string version;
	string createdInVuoVersion;
	string lastSavedInVuoVersion;
	string description;
	vector<string> keywords;
	string copyright;
	string license;
	string iconURL;
	string homepageURL;
	string documentationURL;
	string bundleID;
	string fxPlugGroup;
	vector<string> categories;
};
