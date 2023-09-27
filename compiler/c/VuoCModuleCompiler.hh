/**
 * @file
 * VuoCModuleCompiler interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerType.hh"
#include "VuoCompilerIssue.hh"
#include "VuoModuleCompiler.hh"
class VuoClangIssues;

/**
 * Responsible for compiling node classes, types, and libraries implemented in C/C++/Objective-C.
 */
class VuoCModuleCompiler : public VuoModuleCompiler
{
public:
	~VuoCModuleCompiler(void);
	VuoModuleCompilerResults compile(dispatch_queue_t llvmQueue, VuoCompilerIssues *issues);
	string generateHeader(VuoCompilerIssues *issues);
	void overrideSourceCode(const string &sourceCode, const string &sourcePath);

private:
	static void __attribute__((constructor)) init();
	static VuoModuleCompiler * newModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);
	VuoCModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);
	string getVirtualSourceDirectory(void);
	string getVirtualSourcePath(void);
	unique_ptr<clang::CompilerInstance> createCompilerInstance(const string &inputSourceCode, bool shouldInsertExternDeclarations, bool shouldEnableAllWarnings, bool shouldGenerateDependencyFile, string &dependencyFilePath, IntrusiveRefCntPtr<clang::vfs::InMemoryFileSystem> &inMemoryFileSystem);
	IntrusiveRefCntPtr<clang::vfs::InMemoryFileSystem> setUpVirtualFileSystem(clang::CompilerInstance *compilerInstance, bool shouldInsertExternDeclarations);
	void addVirtualFileToFileSystem(clang::vfs::InMemoryFileSystem *inMemoryFileSystem, const string &path, const string &contents);
	string generateVirtualFileForSpecializedHeader(clang::vfs::InMemoryFileSystem *inMemoryFileSystem, const string &path, bool shouldDefineGenericTypes);
	string generateContentsForSpecializedHeader(const string &virtualPath, const string &genericPrefix, VuoFileUtilities::File *genericHeaderFile);
	void replaceVirtualPathsInDependencyFile(shared_ptr<VuoMakeDependencies> makeDependencies, clang::vfs::InMemoryFileSystem *inMemoryFileSystem);
	string preprocess(const string &inputSourceCode);
	string replaceGenericTypes(const string &inputSourceCode);
	string abridgeUnspecializedGenericType(const string &inputSourceCode);
	Module * compileTransformedSourceCode(const string &inputSourceCode, dispatch_queue_t llvmQueue, shared_ptr<VuoMakeDependencies> &makeDependencies);
	string extractPath(const char *line, const char *prefix);
	unique_ptr<VuoFileUtilities::File> findHeaderFile(const string &path);
	unique_ptr<VuoFileUtilities::File> findGenericHeaderFile(const string &specializedModuleKey, string &genericPrefix);
	unique_ptr<VuoFileUtilities::File> findGenericSourceFile(const string &specializedPath, string &genericPrefix, unique_ptr<VuoFileUtilities::File> &genericHeaderFile);
	string findPrecompiledHeaderFile(const string &moduleSourceCode, const string &genericPrefix);
	static map<string, string> parseBackingTypesForGenericTypes(const string &moduleSourceCode);
	static string findMetadata(const string &moduleSourceCode);
	static bool isType(const string &moduleSourceCode, const string &moduleKey, size_t genericTypeCount);
	static bool isGenericType(const string &moduleSourceCode, const string &genericPrefix, size_t genericTypeCount);

	string sourceCode;
	map<string, string> typeNameReplacements;
	map<string, string> typeNameReplacementsFromIncludedHeaders;
	json_object * specializedModuleDetails;  ///< Module metadata that should be added to the metadata already in the source code.
	vector<string> headerSearchPaths;
	set<string> includedHeaders;  ///< Headers in include directives encountered while preprocessing the module's source code or added when generating the preface header.
	string precompiledHeaderPath;
	string sysroot;
	VuoCompilerIssues *compileIssues;
	shared_ptr<VuoClangIssues> clangIssues;

	friend class VuoPreprocessorCallbacks;
};
