/**
 * @file
 * VuoClangIssues implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoClangIssues.hh"
#include "VuoStringUtilities.hh"
#include <sstream>

/**
 * Appends an issue to the list.
 */
void VuoClangIssues::addIssue(const string &location, clang::DiagnosticsEngine::Level level, const string &message)
{
	vector<string> locationParts = VuoStringUtilities::split(location, ':');
	string path;
	int line = -1;
	int column = -1;
	if (locationParts.size() == 3)
	{
		path = locationParts[0];
		line = std::stoi(locationParts[1]);
		column = std::stoi(locationParts[2]);
	}
	else
		path = location;

	Issue issue = {path, line, column, level, message};
	issues.push_back(issue);
}

/**
 * Returns a description of the issue, formatted like Clang's diagnostics so it can be parsed by an IDE.
 */
string VuoClangIssues::getDescription(const string &lineBreak)
{
	ostringstream s;
	size_t i = 0;

	for (Issue issue : issues)
	{
		if (! issue.path.empty())
		{
			s << issue.path << ":";

			if (issue.line >= 0)
			{
				s << issue.line << ":";
				if (issue.column >= 0)
					s << issue.column << ":";
			}

			s << " ";
		}

		string levelName;
		if (issue.level == clang::DiagnosticsEngine::Ignored)
			levelName = "ignored";
		else if (issue.level == clang::DiagnosticsEngine::Note)
			levelName = "note";
		else if (issue.level == clang::DiagnosticsEngine::Remark)
			levelName = "remark";
		else if (issue.level == clang::DiagnosticsEngine::Warning)
			levelName = "warning";
		else if (issue.level == clang::DiagnosticsEngine::Error)
			levelName = "error";
		else if (issue.level == clang::DiagnosticsEngine::Fatal)
			levelName = "fatal error";

		if (! levelName.empty())
			s << levelName << ": ";

		s << issue.message;

		if (++i < issues.size())
			s << lineBreak;
	}

	return s.str();
}

/**
 * Returns whether the list of issues is empty.
 */
bool VuoClangIssues::isEmpty(void)
{
	return issues.empty();
}

/**
 * Returns true if at least one issue in the list is an error.
 */
bool VuoClangIssues::hasErrors(void)
{
	auto isError = [](const Issue &issue)
	{
		return issue.level == clang::DiagnosticsEngine::Error || issue.level == clang::DiagnosticsEngine::Fatal;
	};

	return std::find_if(issues.begin(), issues.end(), isError) != issues.end();
}
