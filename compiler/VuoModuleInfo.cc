/**
 * @file
 * VuoModuleInfo implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleInfo.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerEnvironment.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerModule.hh"
#include <sys/time.h>

/**
 * Constructs a VuoModuleInfo given the module/composition file's path relative to the search path.
 */
VuoModuleInfo::VuoModuleInfo(VuoCompilerEnvironment *environment, const string &searchPath, const string &relativePath,
							 bool isSourceFile, bool isSubcomposition)
{
	VuoFileUtilities::File *file = new VuoFileUtilities::File(searchPath, relativePath);
	initialize(environment, searchPath, file, isSourceFile, isSubcomposition);
}

/**
 * Constructs a VuoModuleInfo given a reference to the module/composition file.
 */
VuoModuleInfo::VuoModuleInfo(VuoCompilerEnvironment *environment, const string &searchPath, VuoFileUtilities::File *file,
							 bool isSourceFile, bool isSubcomposition)
{
	initialize(environment, searchPath, file, isSourceFile, isSubcomposition);
}

/**
 * Helper function for constructors.
 */
void VuoModuleInfo::initialize(VuoCompilerEnvironment *environment, const string &searchPath, VuoFileUtilities::File *file,
							   bool isSourceFile, bool isSubcomposition)
{
	this->environment = environment;
	this->searchPath = searchPath;
	this->file = file;
	moduleKey = VuoCompiler::getModuleKeyForPath(file->getRelativePath());
	moduleKeyMangled = VuoCompilerModule::isModuleKeyMangledInFileName(file->getRelativePath());
	sourceCodeOverridden = false;
	lastModified = VuoFileUtilities::getFileLastModifiedInSeconds(file->isInArchive() ? file->getArchivePath() : file->path());
	longestDownstreamPath = 0;
	attempted = false;
	loading = nullptr;

#if VUO_PRO
	init_Pro();
#endif

	if (isSourceFile)
	{
		loading = dispatch_group_create();
		sourceCode = file->getContentsAsString();

		if (isSubcomposition)
		{
			try
			{
				containedNodeClasses = VuoCompilerGraphvizParser::getNodeClassNamesFromCompositionFile(file->path());
			}
			catch (...)
			{
				// Issues parsing the composition will be caught later when compiling it.
			}
		}
	}
}

/**
 * Destructor.
 */
VuoModuleInfo::~VuoModuleInfo(void)
{
#if VUO_PRO
	fini_Pro();
#endif

	delete file;

	if (loading)
		dispatch_release(loading);
}

/**
 * Returns the environment that owns this module.
 */
VuoCompilerEnvironment * VuoModuleInfo::getEnvironment(void) const
{
	return environment;
}

/**
 * Returns the module search path in which the file is located.
 */
string VuoModuleInfo::getSearchPath(void) const
{
	return searchPath;
}

/**
 * Returns the module key derived from the file name or revised by setUnmangledModuleKey().
 */
string VuoModuleInfo::getModuleKey(void) const
{
	return moduleKey;
}

/**
 * Returns true if the currently stored module key is a mangled form of the actual module key.
 */
bool VuoModuleInfo::isModuleKeyMangled(void) const
{
	return moduleKeyMangled;
}

/**
 * Replaces the stored mangled module key with its unmangled form.
 */
void VuoModuleInfo::setUnmangledModuleKey(const string &moduleKey)
{
	this->moduleKey = moduleKey;
	moduleKeyMangled = false;
}

/**
 * Returns the module/source file, which may be in either a directory or an archive.
 */
VuoFileUtilities::File * VuoModuleInfo::getFile(void) const
{
	return file;
}

/**
 * Returns the module's source code (if any). This may be the file contents, or may differ if there are unsaved changes.
 */
string VuoModuleInfo::getSourceCode(void) const
{
	return sourceCode;
}

/**
 * Sets the module's source code, presumably to something other than the file contents.
 */
void VuoModuleInfo::setSourceCode(const string &sourceCode)
{
	this->sourceCode = sourceCode;
}

/**
 * Sets the module's source code back to the file contents.
 */
void VuoModuleInfo::revertSourceCode(void)
{
	sourceCode = file->getContentsAsString();
}

/**
 * Returns the value set by VuoModuleInfo::setSourceCodeOverridden(), or false by default.
 */
bool VuoModuleInfo::isSourceCodeOverridden(void) const
{
	return sourceCodeOverridden;
}

/**
 * Stores whether the source code is overridden.
 */
void VuoModuleInfo::setSourceCodeOverridden(bool overridden)
{
	sourceCodeOverridden = overridden;
}

/**
 * Returns true if this file was last modified more recently than @a other.
 */
bool VuoModuleInfo::isNewerThan(VuoModuleInfo *other) const
{
	return lastModified > other->lastModified;
}

/**
 * Returns true if this file was last modified more recently than @a ageInSeconds.
 */
bool VuoModuleInfo::isNewerThan(double ageInSeconds) const
{
	return lastModified > ageInSeconds;
}

/**
 * Returns true if this file was last modified less recently than @a ageInSeconds.
 */
bool VuoModuleInfo::isOlderThan(double ageInSeconds) const
{
	return lastModified < ageInSeconds;
}

/**
 * Sets the last-modified timestamp associated with this object, overriding (but not modifying) the file's timestamp.
 */
void VuoModuleInfo::setLastModifiedToNow(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	lastModified = t.tv_sec + t.tv_usec/1000000.0;
}

/**
 * Returns the class names of all node classes contained at the top level of this subcomposition.
 */
set<string> VuoModuleInfo::getContainedNodeClasses(void) const
{
	return containedNodeClasses;
}

/**
 * Returns the value set by VuoModuleInfo::setLongestDownstreamPath(), or 0 by default.
 */
int VuoModuleInfo::getLongestDownstreamPath(void) const
{
	return longestDownstreamPath;
}

/**
 * Stores the number of vertices in the longest path downstream of this subcomposition in the composition dependency graph.
 */
void VuoModuleInfo::setLongestDownstreamPath(int pathLength)
{
	longestDownstreamPath = pathLength;
}

/**
 * Returns the value set by VuoModuleInfo::setAttempted(), or false by default.
 */
bool VuoModuleInfo::getAttempted(void) const
{
	return attempted;
}

/**
 * Stores whether the compiler either has attempted to load this file (if it's a compiled module) or
 * has scheduled (and possibly completed) compilation of this file (if it's a source file).
 */
void VuoModuleInfo::setAttempted(bool attempted)
{
	this->attempted = attempted;
}

/**
 * Returns a dispatch group on which the compiler should schedule the compiling and loading of this source file.
 */
dispatch_group_t VuoModuleInfo::getLoadingGroup(void) const
{
	return loading;
}

/**
 * Prints debug info.
 */
void VuoModuleInfo::dump() const
{
	fprintf(stderr, "module %s:\n"
					"\tloadingGroup=%p\n"
					"\tsearchPath=%s\n"
					"\tattempted=%d\n"
					"\tenvironment=%s\n"
					"\tfile=%s%s%s\n"
					"\tsourceCodeOverridden=%d\n"
					"\tsourceCode=%s\n"
					"\tcontainedNodeClasses:\n",
			moduleKey.c_str(),
			loading,
			searchPath.c_str(),
			attempted,
			environment->getCompiledModuleCachePath().c_str(),
			file->isInArchive() ? file->getArchivePath().c_str() : "",
			file->isInArchive() ? "::" : "",
			file->isInArchive() ? file->getRelativePath().c_str() : file->path().c_str(),
			sourceCodeOverridden,
			sourceCode.c_str());
	for (auto i: containedNodeClasses)
		fprintf(stderr, "\t\t%s\n", i.c_str());
}
