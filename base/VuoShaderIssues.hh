/**
 * @file
 * VuoShaderIssues interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <vector>
#include <string>

#include "VuoShaderFile.hh"

/**
 * Maintains a list of issues for each stage of a shader collection.
 */
class VuoShaderIssues
{
public:
	/**
	 * A warning or error emitted when compiling a shader.
	 */
	typedef struct
	{
		VuoShaderFile::Stage stage;
		int lineNumber;
		std::string message;
	} Issue;

	const static int NoLine;
	const static int PreambleLine;

	void addIssue(VuoShaderFile::Stage stage, int lineNumber, std::string message);

	vector<Issue> &issues();
	vector<Issue> issuesForStage(VuoShaderFile::Stage stage);
	static bool isUserEnteredLine(int lineNumber);

	void dump();

private:
	vector<Issue> _issues;
};
