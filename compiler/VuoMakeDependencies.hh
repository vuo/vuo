/**
 * @file
 * VuoMakeDependencies interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Abstraction of the "make dependencies"/"depfile" format used by CMake. Provides functions for reading and writing
 * a dependency file and manipulating its contents.
 *
 * @see https://cmake.org/cmake/help/latest/command/add_custom_command.html (under DEPFILE)
 */
class VuoMakeDependencies
{
public:
	static shared_ptr<VuoMakeDependencies> createFromFile(const string &dependencyFilePath);
	static shared_ptr<VuoMakeDependencies> createFromComponents(const string &compiledFilePath, const vector<string> &dependencyPaths);

	static string getPlaceholderCompiledFilePath(void);
	void setCompiledFilePath(const string &compiledFilePath);

	vector<string> getDependencyPaths(void);
	void setDependencyPaths(const vector<string> &dependencyPaths);

	void writeToFile(const string &dependencyFilePath);

private:
	VuoMakeDependencies(const string &compiledFilePath, const vector<string> &dependencyPaths);

	string compiledFilePath;
	vector<string> dependencyPaths;
};
