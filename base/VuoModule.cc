/**
 * @file
 * VuoModule implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoModule.hh"
#include "VuoNodeSet.hh"

/**
 * Creates a module.
 *
 * @param moduleKey A unique name for this module.
 */
VuoModule::VuoModule(string moduleKey)
{
	this->moduleKey = moduleKey;
	this->nodeSet = NULL;
}

/**
 * Returns this module's unique name.
 */
string VuoModule::getModuleKey(void)
{
	return moduleKey;
}

/**
 * Sets this module's unique name.
 */
void VuoModule::setModuleKey(string moduleKey)
{
	this->moduleKey = moduleKey;
}

/**
 * Returns the default title for instances of this module, as specified by the .vuonode/.bc implementation.  Needn't be unique.
 *
 * UTF-8.
 *
 * @eg{Less Than}
 *
 * @see VuoModuleMetadata
 */
string VuoModule::getDefaultTitle(void)
{
	return defaultTitle;
}

/**
 * Returns the default title, with the optional parenthetical suffix removed.
 *
 * UTF-8.
 *
 * @par Example:
 *
 * If @ref getDefaultTitle() returns `Select Event Output (Boolean)`, getDefaultTitleWithoutSuffix() returns `Select Event Output`.
 *
 * @see VuoModuleMetadata
 */
string VuoModule::getDefaultTitleWithoutSuffix(void)
{
	string title = defaultTitle;

	if (*(title.end() - 1) == ')')
		title = title.substr(0, title.find(" ("));

	return title;
}

/**
 * Sets the default title for instances of this module.  Needn't be unique.
 *
 * UTF-8.
 *
 * @eg{Less Than}
 *
 * @see VuoModuleMetadata
 */
void VuoModule::setDefaultTitle(string defaultTitle)
{
	this->defaultTitle = defaultTitle;
}

/**
 * Returns the description given in the module's metadata, or if none is given,
 * the description found in a separate file in the module's node set.
 *
 * If this module doesn't have a description, returns an empty string.
 *
 * @see VuoModuleMetadata
 */
string VuoModule::getDescription(void)
{
	if (! description.empty())
		return description;

	if (nodeSet)
		return nodeSet->getDescriptionForModule(this);

	return "";
}

/**
 * Sets the description of this module provided as documentation.
 *
 * @see VuoModuleMetadata
 */
void VuoModule::setDescription(string description)
{
	this->description = description;
}

/**
 * Returns the module's version, in Semantic Versioning format.
 *
 * @see VuoModuleMetadata
 */
string VuoModule::getVersion(void)
{
	return version;
}

/**
 * Sets the module's version, in Semantic Versioning format.
 *
 * @see VuoModuleMetadata
 */
void VuoModule::setVersion(string version)
{
	this->version = version;
}

/**
 * Returns a list of the module's keywords.
 *
 * @see VuoModuleMetadata
 */
vector<string> VuoModule::getKeywords(void)
{
	return keywords;
}

/**
 * Sets the module's keywords.
 *
 * @see VuoModuleMetadata
 */
void VuoModule::setKeywords(vector<string> keywords)
{
	this->keywords = keywords;
}

/**
 * Returns the node set containing this module.
 */
VuoNodeSet * VuoModule::getNodeSet(void)
{
	return nodeSet;
}

/**
 * Sets the node set containing this module.
 */
void VuoModule::setNodeSet(VuoNodeSet *nodeSet)
{
	this->nodeSet = nodeSet;
}
