/**
 * @file
 * VuoShaderIssues implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoShaderIssues.hh"

#include "VuoStringUtilities.hh"

const int VuoShaderIssues::NoLine = INT_MAX; ///< If OpenGL didn't tell us which source line the issue applies to.
const int VuoShaderIssues::PreambleLine = 1000000000; ///< The first line number to use for auto-generated code.

#include <sstream>

/**
 * Adds an issue to the list of issues.
 */
void VuoShaderIssues::addIssue(VuoShaderFile::Stage stage, int lineNumber, std::string message)
{
	// Ignore innocuous warning.
	if (stage == VuoShaderFile::Program && message == "Output of vertex shader 'isf_FragNormCoord' not read by fragment shader")
		return;

	ostringstream oss;

	oss << message;

	if (!VuoStringUtilities::endsWith(message, ")"))
	{
		if (stage != VuoShaderFile::Program)
			oss << " (" << VuoShaderFile::stageName(stage);

		if (isUserEnteredLine(lineNumber))
			oss << " line " << lineNumber << ")";
		else if (stage != VuoShaderFile::Program)
			oss << ")";
	}

	_issues.push_back((Issue){stage, lineNumber, oss.str()});
}

/**
 * Returns the full list of issues.
 */
vector<VuoShaderIssues::Issue> &VuoShaderIssues::issues()
{
	return _issues;
}

/**
 * Returns only the issues that apply to this shader stage.
 */
vector<VuoShaderIssues::Issue> VuoShaderIssues::issuesForStage(VuoShaderFile::Stage stage)
{
	vector<VuoShaderIssues::Issue> stageIssues;
	for (vector<Issue>::iterator it = _issues.begin(); it != _issues.end(); ++it)
		if (it->stage == stage)
			stageIssues.push_back(*it);
	return stageIssues;
}

/**
 * Returns true if the line number is known
 * and is within the user-entered shader code (as opposed to the preamble).
 */
bool VuoShaderIssues::isUserEnteredLine(int lineNumber)
{
	return (lineNumber != NoLine
		 && lineNumber < PreambleLine);
}

/**
 * Logs the shader issues to the console.
 */
void VuoShaderIssues::dump()
{
	for (vector<Issue>::iterator it = _issues.begin(); it != _issues.end(); ++it)
		VUserLog("%s", it->message.c_str());
}
