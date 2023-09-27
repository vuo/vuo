/**
 * @file
 * VuoCompilerIssue interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCable;
class VuoModule;
class VuoNode;
class VuoClangIssues;
class VuoShaderIssues;

/**
 * An error or warning that occurred while compiling a module or composition.
 *
 * A VuoCompilerIssue is reported at a level of detail aimed at composition authors.
 * It can be supplemented with a list of more technical VuoClangIssues or VuoShaderIssues
 * aimed at module developers.
 */
class VuoCompilerIssue
{
public:
	/**
	 * Severity levels of issues.
	 */
	enum IssueType
	{
		Error,  ///< Compilation fails.
		Warning  ///< Compilation can continue, though it may not have the expected result.
	};

	VuoCompilerIssue(void);
	VuoCompilerIssue(IssueType issueType, const string &action, const string &filePath, const string &summary, const string &details);
	IssueType getIssueType(void);
	string getSummary(void);
	string getDetails(bool htmlFormatted) const;
	string getShortDescription(bool htmlFormatted) const;
	string getLongDescription(bool htmlFormatted);
	void setHint(const string &hint);
	string getHint(bool htmlFormatted);
	void setHelpPath(const string &path);
	string getHelpPath(void);
	void setFilePath(const string &filePath);
	string getFilePath(void);
	void setModuleKey(const string &moduleKey);
	void setLink(const string &url, const string &text);
	void setModule(VuoModule *module);
	void setNode(VuoNode *node);
	void setNodes(const set<VuoNode *> &nodes);
	set<VuoNode *> getNodes(void);
	void setCables(const set<VuoCable *> &cables);
	set<VuoCable *> getCables(void);
	void setClangIssues(shared_ptr<VuoClangIssues> clangIssues);
	void setShaderIssues(shared_ptr<VuoShaderIssues> shaderIssues);
	shared_ptr<VuoShaderIssues> getShaderIssues(void);
	long getHash(void);

private:
	string getIssueTypeString(void);
	string fillInPlaceholders(const string &s, bool htmlFormatted) const;

	IssueType issueType;  ///< Error or warning.
	string action;  ///< The action being attempted when the issue occurred.
	string summary;  ///< Brief description of the problem.
	string details;  ///< Lengthier description of the problem.
	string hint;  ///< Suggestion to help the user fix the problem.
	string helpPath;  ///< A file in the Help Book.
	string filePath;  ///< The file being acted on when the issue occurred.
	string moduleKey;  ///< A module key to display in the details or hint.
	string linkUrl;  ///< The URL part of a link to display in the details or hint.
	string linkText;  ///< The human-readable part of a link to display in the details or hint.
	VuoModule *module;  ///< The problematic module.
	set<VuoNode *> nodes;  ///< The problematic nodes.
	set<VuoCable *> cables;  ///< The problematic cables.
	shared_ptr<VuoClangIssues> clangIssues;  ///< Issues emitted by the Clang compiler.
	shared_ptr<VuoShaderIssues> shaderIssues;  ///< Issues emitted by the ISF shader compiler.
};

/**
 * A collection of `VuoCompilerIssue`s that occurred while compiling a module or composition.
 *
 * This class provides some functions for aggregating and de-duplicating error/warning messages.
 */
class VuoCompilerIssues
{
public:
	VuoCompilerIssues(void);
	VuoCompilerIssues(const VuoCompilerIssue &issue);
	string getShortDescription(bool htmlFormatted);
	string getLongDescription(bool htmlFormatted);
	string getHint(bool htmlFormatted);
	void setFilePathIfEmpty(const string &filePath);
	void setFilePath(const string &filePath);
	void setFilePath(std::function<string(const string &)> generateFilePath);
	void append(const VuoCompilerIssue &issue);
	void append(VuoCompilerIssues *otherIssues);
	bool isEmpty(void);
	vector<VuoCompilerIssue> getList(void);
	VuoCompilerIssues * getErrors(void);
	bool hasErrors(void);

private:
	vector<VuoCompilerIssue> issues;  ///< Ordered list of issues.
};
