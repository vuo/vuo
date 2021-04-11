/**
 * @file
 * VuoCompilerIssue implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>
#include "VuoCompilerIssue.hh"
#include "VuoModule.hh"
#include "VuoStringUtilities.hh"

/**
 * Returns either HTML-formatted or plain-text linebreaks.
 */
static string getLineBreak(bool htmlFormatted, int count = 1)
{
	string lineBreak = (htmlFormatted ? "<br>" : "\n");
	string ret;
	for (int i = 0; i < count; ++i)
		ret += lineBreak;
	return ret;
}

/**
 * Constructor. Useful for putting `VuoCompilerIssue`s in certain STL collections, not for much else.
 */
VuoCompilerIssue::VuoCompilerIssue(void)
{
	issueType = VuoCompilerIssue::Error;
	module = NULL;
}

/**
 * Constructor. The arguments will be used to build strings to display in error dialogs or error messages in the shell.
 *
 * @param issueType Error if it caused an action to fail completely, warning if the action was able to continue.
 * @param action Brief description of the action that was being attempted, e.g. "compiling module", "opening composition".
 *     Use lowercase.
 * @param filePath The file that the action was operating on, e.g. the module being compiled or the composition being opened.
 *     If none or unknown, pass an empty string.
 * @param summary Brief description of the cause of the issue, e.g. "Infinite feedback loop", "Subcomposition contains itself".
 *     Use sentence case. No period. If the issue is adequately described by the other arguments and the summary would just be
 *     redundant, pass an empty string.
 * @param details Longer description of the cause of the issue. To accommodate either HTML or plain-text formatting, it may
 *     contain placeholders that are replaced when returning the issue's details, either by themselves or within other text.
 *     Supported placeholders:
 *       - `%moduleKey` — Replaced with the formatted value of VuoCompilerIssue#moduleKey.
 *       - `%module` — Replaced with the formatted default title and module key of VuoCompilerIssue#module.
 *       - `%link` — Replaced with the formatted value of VuoCompilerIssue#linkUrl and VuoCompilerIssue#linkText.
 */
VuoCompilerIssue::VuoCompilerIssue(IssueType issueType, const string &action, const string &filePath, const string &summary, const string &details)
{
	this->issueType = issueType;
	this->action = action;
	this->filePath = filePath;
	this->summary = summary;
	this->details = details;
	module = NULL;
}

/**
 * Returns a string representation of #issueType.
 */
string VuoCompilerIssue::getIssueTypeString(void)
{
	string issueTypeStr;
	switch (issueType)
	{
		case Error:
			issueTypeStr = "Error";
			break;
		case Warning:
			issueTypeStr = "Warning";
			break;
	}

	return issueTypeStr;
}

/**
 * Returns a string in which all placeholders in @a s have been replaced with their values,
 * assuming the data members for those values have been set.
 */
string VuoCompilerIssue::fillInPlaceholders(const string &s, bool htmlFormatted) const
{
	string ret = s;

	{
		string moduleKeyFormatted = moduleKey;
		if (htmlFormatted)
			moduleKeyFormatted = "<code>" + moduleKeyFormatted + "</code>";
		VuoStringUtilities::replaceAll(ret, "%moduleKey", moduleKeyFormatted);
	}

	if (module)
	{
		string moduleTitleFormatted = module->getDefaultTitle();
		if (htmlFormatted)
			moduleTitleFormatted = "<code>" + moduleTitleFormatted + "</code>";
		string moduleKeyFormatted = module->getModuleKey();
		if (htmlFormatted)
			moduleKeyFormatted = "<code>" + moduleKeyFormatted + "</code>";
		VuoStringUtilities::replaceAll(ret, "%module", moduleTitleFormatted + " (" + moduleKeyFormatted + ")");
	}

	{
		string link = (htmlFormatted ?
						   "<a href=\"" + linkUrl + "\">" + linkText + "</a>" :
						   linkText + " (" + linkUrl + ")");
		VuoStringUtilities::replaceAll(ret, "%link", link);
	}

	return ret;
}

/**
 * Returns the type of issue (error or warning).
 */
VuoCompilerIssue::IssueType VuoCompilerIssue::getIssueType(void)
{
	return issueType;
}

/**
 * Returns a brief description of the problem.
 */
string VuoCompilerIssue::getSummary(void)
{
	return summary;
}

/**
 * Returns a lengthier description of the problem.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssue::getDetails(bool htmlFormatted) const
{
	return fillInPlaceholders(details, htmlFormatted);
}

/**
 * Returns the summary and details.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssue::getShortDescription(bool htmlFormatted) const
{
	string description;

	if (! summary.empty())
		description += summary + " — ";

	description += getDetails(htmlFormatted);

	return description;
}

/**
 * Returns a thorough description of the issue that includes all of the information provided to the constructor.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssue::getLongDescription(bool htmlFormatted)
{
	string description = getIssueTypeString() + " " + action;

	if (! filePath.empty())
	{
		string filePathFormatted = filePath;
		if (htmlFormatted)
			filePathFormatted = "<code>" + filePathFormatted + "</code>";

		description += " " + filePathFormatted;
	}

	description += ":" + getLineBreak(htmlFormatted);

	if (! summary.empty())
		description += summary + " — ";

	description += getDetails(htmlFormatted);

	return description;
}

/**
 * Sets the suggestion to help the user fix the problem.
 *
 * @see VuoCompilerIssue(IssueType, const string&, const string&, const string&, const string&) for a list of supported placeholders.
 */
void VuoCompilerIssue::setHint(const string &hint)
{
	this->hint = hint;
}

/**
 * Returns the suggestion to help the user fix the problem.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssue::getHint(bool htmlFormatted)
{
	return fillInPlaceholders(hint, htmlFormatted);
}

/**
 * Sets the path to the Help Book file explaining the problem.
 */
void VuoCompilerIssue::setHelpPath(const string &path)
{
	this->helpPath = path;
}

/**
 * Returns the path to the Help Book file explaining the problem.
 */
string VuoCompilerIssue::getHelpPath(void)
{
	return helpPath;
}

/**
 * Sets the file being processed when the issue occurred.
 */
void VuoCompilerIssue::setFilePath(const string &filePath)
{
	this->filePath = filePath;
}

/**
 * Returns the file being processed when the issue occurred.
 */
string VuoCompilerIssue::getFilePath(void)
{
	return filePath;
}

/**
 * Sets the location in source code of the problem.
 */
void VuoCompilerIssue::setLineNumber(int lineNumber)
{
	this->lineNumber = lineNumber;
}

/**
 * Returns the location in source code of the problem.
 */
int VuoCompilerIssue::getLineNumber(void)
{
	return lineNumber;
}

/**
 * Sets the replacement value for the `%moduleKey` placeholder.
 */
void VuoCompilerIssue::setModuleKey(const string &moduleKey)
{
	this->moduleKey = moduleKey;
}

/**
 * Sets the replacement values for the `%link` placeholder.
 */
void VuoCompilerIssue::setLink(const string &url, const string &text)
{
	linkUrl = url;
	linkText = text;
}

/**
 * Sets the replacement value for the `%module` placeholder.
 */
void VuoCompilerIssue::setModule(VuoModule *module)
{
	this->module = module;
}

/**
 * Sets a single node as the node involved in the issue.
 */
void VuoCompilerIssue::setNode(VuoNode *node)
{
	nodes.clear();
	nodes.insert(node);
}

/**
 * Sets the nodes involved in the issue.
 */
void VuoCompilerIssue::setNodes(const set<VuoNode *> &nodes)
{
	this->nodes = nodes;
}

/**
 * Returns the nodes involved in the issue.
 */
set<VuoNode *> VuoCompilerIssue::getNodes(void)
{
	return nodes;
}

/**
 * Sets the cables involved in the issue.
 */
void VuoCompilerIssue::setCables(const set<VuoCable *> &cables)
{
	this->cables = cables;
}

/**
 * Returns the cables involved in the issue.
 */
set<VuoCable *> VuoCompilerIssue::getCables(void)
{
	return cables;
}

/**
 * Generates a unique, consistent hash value for the error description info and the problematic nodes and cables.
 */
long VuoCompilerIssue::getHash(void)
{
	ostringstream s;

	s << getLongDescription(false);

	vector<string> nodeStrings;
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		ostringstream ss;
		ss << *i;
		nodeStrings.push_back(ss.str());
	}
	sort(nodeStrings.begin(), nodeStrings.end());
	s << "[" << VuoStringUtilities::join(nodeStrings, ',') << "]";

	vector<string> cableStrings;
	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		ostringstream ss;
		ss << *i;
		cableStrings.push_back(ss.str());
	}
	s << "[" << VuoStringUtilities::join(cableStrings, ',') << "]";

	return VuoStringUtilities::hash(s.str());
}


/**
 * Constructs an empty list of issues.
 */
VuoCompilerIssues::VuoCompilerIssues(void)
{
}

/**
 * Constructs a list of issues with one item.
 */
VuoCompilerIssues::VuoCompilerIssues(const VuoCompilerIssue &issue)
{
	issues.push_back(issue);
}

/**
 * Returns the short descriptions of the issues in the list, de-duplicated and appended.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssues::getShortDescription(bool htmlFormatted)
{
	vector<string> uniqueDescriptions;
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
	{
		string description = (*i).getShortDescription(htmlFormatted);
		if (find(uniqueDescriptions.begin(), uniqueDescriptions.end(), description) == uniqueDescriptions.end())
			uniqueDescriptions.push_back(description);
	}

	return VuoStringUtilities::join(uniqueDescriptions, getLineBreak(htmlFormatted, 2));
}

/**
 * Returns the long descriptions of the issues in the list, de-duplicated and appended.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssues::getLongDescription(bool htmlFormatted)
{
	vector<string> uniqueDescriptions;
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
	{
		string description = (*i).getLongDescription(htmlFormatted);
		if (find(uniqueDescriptions.begin(), uniqueDescriptions.end(), description) == uniqueDescriptions.end())
			uniqueDescriptions.push_back(description);
	}

	return VuoStringUtilities::join(uniqueDescriptions, getLineBreak(htmlFormatted, 2));
}

/**
 * Returns the hints of the issues in the list, de-duplicated and appended.
 *
 * @param htmlFormatted If false, the returned string is plain text. If true, the returned string may contain HTML formatting.
 */
string VuoCompilerIssues::getHint(bool htmlFormatted)
{
	vector<string> uniqueHints;
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
	{
		string hint = (*i).getHint(htmlFormatted);
		if (! hint.empty() && find(uniqueHints.begin(), uniqueHints.end(), hint) == uniqueHints.end())
			uniqueHints.push_back(hint);
	}

	return VuoStringUtilities::join(uniqueHints, getLineBreak(htmlFormatted, 2));
}

/**
 * Sets the file for each issue in the list that doesn't already have one.
 */
void VuoCompilerIssues::setFilePathIfEmpty(const string &filePath)
{
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
		if ((*i).getFilePath().empty())
			(*i).setFilePath(filePath);
}

/**
 * Sets the file for each issue in the list.
 */
void VuoCompilerIssues::setFilePath(const string &filePath)
{
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
		(*i).setFilePath(filePath);
}
/**
 * Adds the issue to the end of the list.
 */
void VuoCompilerIssues::append(const VuoCompilerIssue &issue)
{
	issues.push_back(issue);
}

/**
 * Adds the issues to the end of the list.
 */
void VuoCompilerIssues::append(VuoCompilerIssues *otherIssues)
{
	issues.insert(issues.begin(), otherIssues->issues.begin(), otherIssues->issues.end());
}

/**
 * Returns true if there are no issues in the list.
 */
bool VuoCompilerIssues::isEmpty(void)
{
	return issues.empty();
}

/**
 * Returns the `VuoCompilerIssue`s in the list.
 */
vector<VuoCompilerIssue> VuoCompilerIssues::getList(void)
{
	return issues;
}

/**
 * Returns a new `VuoCompilerIssues` containing the issues in this list that are errors.
 */
VuoCompilerIssues * VuoCompilerIssues::getErrors(void)
{
	VuoCompilerIssues *errors = new VuoCompilerIssues();
	for (vector<VuoCompilerIssue>::iterator i = issues.begin(); i != issues.end(); ++i)
		if ((*i).getIssueType() == VuoCompilerIssue::Error)
			errors->append(*i);

	return errors;
}
