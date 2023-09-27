/**
 * @file
 * VuoModuleInfo interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoFileUtilities.hh"
class VuoCompilerEnvironment;

/**
 * Information about a module or source file that the compiler has seen, though not necessarily loaded.
 */
class VuoModuleInfo
{
public:
	VuoModuleInfo(VuoCompilerEnvironment *environment, const string &searchPath, const string &relativePath, bool isSourceFile, bool isSubcomposition);
	VuoModuleInfo(VuoCompilerEnvironment *environment, const string &searchPath, VuoFileUtilities::File *file, bool isSourceFile, bool isSubcomposition);
	~VuoModuleInfo(void);
	VuoCompilerEnvironment * getEnvironment(void) const;
	string getSearchPath(void) const;
	string getModuleKey(void) const;
	bool isModuleKeyMangled(void) const;
	void setUnmangledModuleKey(const string &moduleKey);
	VuoFileUtilities::File * getFile(void) const;
	string getSourceCode(void) const;
	void setSourceCode(const string &sourceCode);
	void revertSourceCode(void);
	bool isSourceCodeOverridden(void) const;
	void setSourceCodeOverridden(bool overridden);
	bool isNewerThan(VuoModuleInfo *other) const;
	bool isNewerThan(double seconds) const;
	bool isOlderThan(double seconds) const;
	void setLastModifiedToNow(void);
	set<string> getContainedNodeClasses(void) const;
	int getLongestDownstreamPath(void) const;
	void setLongestDownstreamPath(int pathLength);
	bool getAttempted(void) const;
	void setAttempted(bool attempted);
	dispatch_group_t getLoadingGroup(void) const;
	void dump() const;

private:
	void initialize(VuoCompilerEnvironment *environment, const string &searchPath, VuoFileUtilities::File *file, bool isSourceFile, bool isSubcomposition);

	VuoCompilerEnvironment *environment;  ///< The environment that owns (or will own) this module.
	string searchPath;   ///< The module search path in which the file is located.
	string moduleKey;   ///< The module key. Initially this is the module key extracted from the file name. If that module key is mangled, then a caller may later provide the unmangled module key.
	bool moduleKeyMangled;  ///< True if #moduleKey is a mangled form of the actual module key.
	VuoFileUtilities::File *file;  ///< The file, which may be in a directory or an archive.
	string sourceCode;  ///< If this file is a subcomposition, its source code. May be the file contents, or may differ if there are unsaved changes.
	bool sourceCodeOverridden;  ///< If this file is a subcomposition, true if the source code has been set to something other than the file contents.
	double lastModified;   ///< The time (in seconds since a reference date) when the file was last modified, as of when this instance was constructed.
	set<string> containedNodeClasses;  ///< If this file is a subcomposition, the class names of all node classes contained at the top level of the subcomposition.
	int longestDownstreamPath;  ///< If this file is a subcomposition, the number of vertices in the longest path downstream of it in the composition dependency graph.
	bool attempted;  ///< True if this file is a compiled module and its loading has been attempted, or if this file is a source file and its compilation has been scheduled (and possibly completed).
	dispatch_group_t loading;  ///< If this file is a source file, its compiling and loading is scheduled on this dispatch group so that callers can wait on it.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
	void *p;
#pragma clang diagnostic pop
#ifdef VUO_PRO
#include "../compiler/pro/VuoModuleInfo_Pro.hh"
#endif
};
