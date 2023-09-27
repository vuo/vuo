/**
 * @file
 * VuoMakeDependencies implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMakeDependencies.hh"
#include "VuoException.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a VuoMakeDependencies object with the compiled file path and list of dependencies read from
 * the dependency file at @a dependencyFilePath.
 *
 * @throw VuoException The file couldn't be read or doesn't have the expected format.
 */
shared_ptr<VuoMakeDependencies> VuoMakeDependencies::createFromFile(const string &dependencyFilePath)
{
	string contents = VuoFileUtilities::readFileToString(dependencyFilePath);
	VuoStringUtilities::replaceAll(contents, "\\\n", "");

	string compiledFilePath;
	vector<string> dependencyPaths;
	size_t startPos = 0;
	for (size_t i = 1; i < contents.length(); ++i)
	{
		if (compiledFilePath.empty() && contents[i] == ':' && contents[i-1] != '\\')
		{
			compiledFilePath = contents.substr(startPos, i-startPos);
			startPos = i+1;
		}
		else if ((contents[i] == ' ' && contents[i-1] != '\\') || i == contents.length()-1)
		{
			string dependency = contents.substr(startPos, i-startPos);
			dependency = VuoStringUtilities::trim(dependency);
			if (! dependency.empty())
				dependencyPaths.push_back(dependency);
			startPos = i+1;
		}
	}

	if (compiledFilePath.empty())
		throw VuoException("Dependency file '%s' doesn't begin with a target path.");

	return shared_ptr<VuoMakeDependencies>(new VuoMakeDependencies(compiledFilePath, dependencyPaths));
}

/**
 * Creates a VuoMakeDependencies object containing the given compiled file path and list of dependencies.
 */
shared_ptr<VuoMakeDependencies> VuoMakeDependencies::createFromComponents(const string &compiledFilePath, const vector<string> &dependencyPaths)
{
	return shared_ptr<VuoMakeDependencies>(new VuoMakeDependencies(compiledFilePath, dependencyPaths));
}

/**
 * Constructor.
 */
VuoMakeDependencies::VuoMakeDependencies(const string &compiledFilePath, const vector<string> &dependencyPaths) :
	compiledFilePath(compiledFilePath),
	dependencyPaths(dependencyPaths)
{
}

/**
 * Returns a placeholder that the caller can pass to Clang when generating a dependency file and the
 * compiled file path is not yet known.
 */
string VuoMakeDependencies::getPlaceholderCompiledFilePath(void)
{
	return "PLACEHOLDER";
}

/**
 * Sets the compiled file path in this object (without writing anything to file).
 */
void VuoMakeDependencies::setCompiledFilePath(const string &compiledFilePath)
{
	this->compiledFilePath = compiledFilePath;
}

/**
 * Returns the list of dependencies.
 */
vector<string> VuoMakeDependencies::getDependencyPaths(void)
{
	return dependencyPaths;
}

/**
 * Sets the list of dependencies in this object (without writing anything to file).
 */
void VuoMakeDependencies::setDependencyPaths(const vector<string> &dependencyPaths)
{
	this->dependencyPaths = dependencyPaths;
}

/**
 * Writes the contents of this object to @a dependencyFilePath in the dependency file format.
 *
 * @throw VuoException The file couldn't be written, or the compiled file path is a placeholder
 *   that hasn't been replaced.
 */
void VuoMakeDependencies::writeToFile(const string &dependencyFilePath)
{
	if (compiledFilePath == getPlaceholderCompiledFilePath())
		throw VuoException("Before writing the VuoMakeDependencies to file, replace its placeholder compiled file path.");

	string contents = compiledFilePath + ": \\\n" +
					  VuoStringUtilities::join(dependencyPaths, " \\\n") +
					  "\n";
	VuoFileUtilities::writeStringToFile(contents, dependencyFilePath);
}
