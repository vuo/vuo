/**
 * @file
 * VuoClangIssues interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A list of problems reported by Clang, to be passed along to a module developer.
 */
class VuoClangIssues
{
public:
	/**
	 * A single issue.
	 */
	typedef struct
	{
		string path;  ///< The file in which the problem was encountered.
		int line;  ///< The line number of the problem (or -1 if none specified).
		int column;  ///< The character index of the problem (or -1 if none specified).
		clang::DiagnosticsEngine::Level level;  ///< The level of severity (error, warning, etc.).
		string message;  ///< What problem was encountered.
	} Issue;

	void addIssue(const string &location, clang::DiagnosticsEngine::Level level, const string &message);
	string getDescription(const string &lineBreak);
	bool isEmpty(void);
	bool hasErrors(void);

private:
	vector<Issue> issues;
};
