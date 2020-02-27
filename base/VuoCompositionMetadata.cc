/**
 * @file
 * VuoCompositionMetadata implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompositionMetadata.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a metadata instance with a default name and the rest of the fields empty.
 */
VuoCompositionMetadata::VuoCompositionMetadata(void)
{
	defaultName = "VuoComposition";
}

/**
 * Creates a metadata instance with fields populated from the composition's Doxygen header.
 */
VuoCompositionMetadata::VuoCompositionMetadata(const string &compositionAsString)
{
	defaultName = "VuoComposition";

	__block int charNum = 0;
	string (^getNextLine)(void) = ^{
			string line;
			while (true)
			{
				char c = compositionAsString[charNum++];
				if (c == '\n')
					break;
				if (c == 0)
					return (string)"";
				line += c;
			}
			return line;
		};

	__block bool firstLine = true;
	bool (^getNextDoxygenLine)(string &line) = ^(string &line){
			while ((line = getNextLine()) != "")
			{
				if (firstLine)
				{
					firstLine = false;
					if (line != "/**")
						return false;
					else
						continue;
				}

				if ((line == " */") || (line == "*/"))
					return false;

				if (line.substr(0, 1) == "*")
				{
					if (line.length() > 1)
						line = line.substr(2);
					else
						line = "";
					return true;
				}
				if (line.substr(0, 2) == " *")
				{
					if (line.length() > 2)
						line = line.substr(3);
					else
						line = "";
					return true;
				}
				return false;
			}
			return false;
		};

	string line;
	bool firstDescriptionLine = true;
	bool inCopyright = false;
	bool inLicense = false;
	while (getNextDoxygenLine(line))
	{
		if (line == "@file")
			continue;

		if (line.substr(0, 7) == "@brief ")
		{
			name = line.substr(7);
			continue;
		}

		if (line.substr(0, 8) == "@author ")
		{
			author = line.substr(8);
			continue;
		}

		if (line.substr(0, 9) == "@version ")
		{
			version = line.substr(9);
			continue;
		}

		if (line.substr(0, 21) == "@createdInVuoVersion ")
		{
			createdInVuoVersion = line.substr(21);
			continue;
		}

		if (line.substr(0, 23) == "@lastSavedInVuoVersion ")
		{
			lastSavedInVuoVersion = line.substr(23);
			continue;
		}

		if (line.substr(0, 10) == "@keywords ")
		{
			string keywordsAsString = line.substr(10);
			keywords = VuoStringUtilities::split(keywordsAsString, ',');
			for (string &k : keywords)
				k = VuoStringUtilities::trim(k);
			continue;
		}

		if (line.substr(0, 11) == "@copyright ")
		{
			inCopyright = true;
			copyright = line.substr(11);
			continue;
		}

		if (line.substr(0, 9) == "@license ")
		{
			inCopyright = false;
			inLicense = true;
			license = line.substr(9);
			continue;
		}

		if (line == "")
		{
			inCopyright = false;
			inLicense = false;
		}

		if (line.substr(0, 12) == "@image icon ")
		{
			inCopyright = false;
			inLicense = false;
			iconURL = line.substr(12);
			continue;
		}

		if (line.substr(0, 14) == "@url homepage ")
		{
			inCopyright = false;
			inLicense = false;
			homepageURL = line.substr(14);
			continue;
		}

		if (line.substr(0, 19) == "@url documentation ")
		{
			inCopyright = false;
			inLicense = false;
			documentationURL = line.substr(19);
			continue;
		}

		if (line.substr(0, 8) == "@bundle ")
		{
			inCopyright = false;
			inLicense = false;
			bundleID = line.substr(8);
			continue;
		}

		if (line.substr(0, 13) == "@fxpluggroup ")
		{
			inCopyright = false;
			inLicense = false;
			fxPlugGroup = line.substr(13);
			continue;
		}

		if (line.substr(0, 5) == "@see ")
		{
			inCopyright = false;
			inLicense = false;
			continue;
		}

		if (inCopyright)
		{
			copyright += '\n';
			copyright += line;
			continue;
		}

		if (inLicense)
		{
			license += '\n';
			license += line;
			continue;
		}

		if (!firstDescriptionLine)
			description += '\n';
		description += line;
		firstDescriptionLine = false;
	}

	// Trim leading and trailing newlines, if any.
	while (description.size() > 0 && description[0] == '\n')
		description.erase(description.begin());
	while (description.size() > 0 && description[description.size()-1] == '\n')
		description.erase(description.end() - 1);
}

/**
 * Returns a composition Doxygen header containing the metadata.
 */
string VuoCompositionMetadata::toCompositionHeader(void)
{
	string metadataText =
		"/**\n"
		" * @file\n";

	if (! name.empty())
	{
		metadataText += " * @brief ";
		metadataText += name;
		metadataText += "\n";
	}

	if (!description.empty())
	{
		vector<string> descriptionLines = VuoStringUtilities::split(description, '\n');
		for (const string &descriptionLine : descriptionLines)
		{
			metadataText += " *";
			metadataText += (descriptionLine.empty() ? "" : " ");
			metadataText += descriptionLine;
			metadataText += "\n";
		}
	}

	metadataText += " *\n";

	if (!author.empty())
	{
		metadataText += " * @author ";
		metadataText += author;
		metadataText += "\n";
	}

	if (!version.empty())
	{
		metadataText += " * @version ";
		metadataText += version;
		metadataText += "\n";
	}

	if (!createdInVuoVersion.empty())
	{
		metadataText += " * @createdInVuoVersion ";
		metadataText += createdInVuoVersion;
		metadataText += "\n";
	}

	if (!lastSavedInVuoVersion.empty())
	{
		metadataText += " * @lastSavedInVuoVersion ";
		metadataText += lastSavedInVuoVersion;
		metadataText += "\n";
	}

	if (!keywords.empty())
	{
		metadataText += " * @keywords ";
		metadataText += VuoStringUtilities::join(keywords, ", ");
		metadataText += "\n";
	}

	if (!VuoStringUtilities::trim(copyright).empty())
	{
		metadataText += " * @copyright ";

		vector<string> copyrightLines = VuoStringUtilities::split(copyright, '\n');
		bool firstCopyrightLine = true;
		for (const string &copyrightLine : copyrightLines)
		{
			if (!copyrightLine.empty())
			{
				if (!firstCopyrightLine)
					metadataText += " * ";
				metadataText += copyrightLine;
				metadataText += "\n";
				firstCopyrightLine = false;
			}
		}
	}

	if (!VuoStringUtilities::trim(license).empty())
	{
		metadataText += " * @license ";

		vector<string> licenseLines = VuoStringUtilities::split(license, '\n');
		bool firstLicenseLine = true;
		for (const string &licenseLine : licenseLines)
		{
			if (!licenseLine.empty())
			{
				if (!firstLicenseLine)
					metadataText += " * ";
				metadataText += licenseLine;
				metadataText += "\n";
				firstLicenseLine = false;
			}
		}
	}

	if (!iconURL.empty())
	{
		metadataText += " * @image icon ";
		metadataText += iconURL;
		metadataText += "\n";
	}

	if (!homepageURL.empty())
	{
		metadataText += " * @url homepage ";
		metadataText += homepageURL;
		metadataText += "\n";
	}

	if (!documentationURL.empty())
	{
		metadataText += " * @url documentation ";
		metadataText += documentationURL;
		metadataText += "\n";
	}

	if (!bundleID.empty())
	{
		metadataText += " * @bundle ";
		metadataText += bundleID;
		metadataText += "\n";
	}

	if (!fxPlugGroup.empty())
	{
		metadataText += " * @fxpluggroup ";
		metadataText += fxPlugGroup;
		metadataText += "\n";
	}

	metadataText +=
		" * @see This is a Vuo Composition source code file.  See https://vuo.org for further information.\n"
		" */\n\n";

	return metadataText;
}

/**
 * Sets the composition's display name specified by the user.
 */
void VuoCompositionMetadata::setName(string name)
{
	this->name = name;
}

/**
 * Sets a composition display name to fall back on if the user hasn't specified one.
 */
void VuoCompositionMetadata::setDefaultName(const string &defaultName)
{
	this->defaultName = defaultName;
}

/**
 * Returns the composition's display name specified by the user if any, otherwise the default name.
 */
string VuoCompositionMetadata::getName(void)
{
	return name.empty() ? defaultName : name;
}

/**
 * Returns the composition's display name specified by the user if any, otherwise an empty string.
 */
string VuoCompositionMetadata::getCustomizedName(void)
{
	return name;
}

/**
 * Returns the composition display name to fall back on if the user hasn't specified one.
 */
string VuoCompositionMetadata::getDefaultName(void)
{
	return defaultName;
}

/**
 * Sets the composition's author.
 */
void VuoCompositionMetadata::setAuthor(string author)
{
	this->author = author;
}

/**
 * Returns the composition's author.
 */
string VuoCompositionMetadata::getAuthor(void)
{
	return author;
}

/**
 * Sets the composition's description (documentation).
 */
void VuoCompositionMetadata::setDescription(string description)
{
	this->description = description;
}

/**
 * Returns the composition's description (documentation).
 */
string VuoCompositionMetadata::getDescription(void)
{
	return description;
}

/**
 * Sets the composition's search keywords.
 */
void VuoCompositionMetadata::setKeywords(const vector<string> &keywords)
{
	this->keywords = keywords;
}

/**
 * Returns the composition's search keywords.
 */
vector<string> VuoCompositionMetadata::getKeywords(void)
{
	return keywords;
}

/**
 * Sets the composition's copyright.
 */
void VuoCompositionMetadata::setCopyright(string copyright)
{
	this->copyright = copyright;
}

/**
 * Returns the composition's copyright.
 */
string VuoCompositionMetadata::getCopyright(void)
{
	return copyright;
}

/**
 * Sets the composition's license.
 */
void VuoCompositionMetadata::setLicense(string license)
{
	this->license = license;
}

/**
 * Returns the composition's license.
 */
string VuoCompositionMetadata::getLicense(void)
{
	return license;
}

/**
 * Sets the composition's version.
 */
void VuoCompositionMetadata::setVersion(string version)
{
	this->version = version;
}

/**
 * Returns the composition's version.
 */
string VuoCompositionMetadata::getVersion(void)
{
	return version;
}

/**
 * Sets the Vuo version in which the composition was originally created.
 */
void VuoCompositionMetadata::setCreatedInVuoVersion(string version)
{
	this->createdInVuoVersion = version;
}

/**
 * Returns the Vuo version in which the composition was originally created.
 */
string VuoCompositionMetadata::getCreatedInVuoVersion(void)
{
	return createdInVuoVersion;
}

/**
 * Sets the Vuo version in which the composition was most recently saved.
 */
void VuoCompositionMetadata::setLastSavedInVuoVersion(string version)
{
	this->lastSavedInVuoVersion = version;
}

/**
 * Returns the Vuo version in which the composition was most recently saved.
 */
string VuoCompositionMetadata::getLastSavedInVuoVersion(void)
{
	return lastSavedInVuoVersion;
}

/**
 * Sets the composition's icon URL.
 */
void VuoCompositionMetadata::setIconURL(string url)
{
	this->iconURL = url;
}

/**
 * Returns the composition's icon URL.
 */
string VuoCompositionMetadata::getIconURL(void)
{
	return iconURL;
}

/**
 * Sets the composition's homepage URL.
 */
void VuoCompositionMetadata::setHomepageURL(string url)
{
	this->homepageURL = url;
}

/**
 * Returns the composition's homepage URL.
 */
string VuoCompositionMetadata::getHomepageURL(void)
{
	return homepageURL;
}

/**
 * Sets the composition's documentation URL.
 */
void VuoCompositionMetadata::setDocumentationURL(string url)
{
	this->documentationURL = url;
}

/**
 * Returns the composition's documentation URL.
 */
string VuoCompositionMetadata::getDocumentationURL(void)
{
	return documentationURL;
}

/**
 * Sets the composition's bundle identifier.
 */
void VuoCompositionMetadata::setBundleIdentifier(string id)
{
	this->bundleID = id;
}

/**
 * Returns the composition's bundle identifier.
 */
string VuoCompositionMetadata::getBundleIdentifier(void)
{
	return bundleID;
}

/**
 * Sets the composition's FxPlug group.
 */
void VuoCompositionMetadata::setFxPlugGroup(string group)
{
	this->fxPlugGroup = group;
}

/**
 * Returns the composition's FxPlug group.
 */
string VuoCompositionMetadata::getFxPlugGroup(void)
{
	return fxPlugGroup;
}

/**
 * Sets the shader's ISF categories.
 */
void VuoCompositionMetadata::setCategories(const vector<string> &categories)
{
	this->categories = categories;
}

/**
 * Returns the shader's ISF categories.
 */
vector<string> VuoCompositionMetadata::getCategories(void)
{
	return categories;
}
