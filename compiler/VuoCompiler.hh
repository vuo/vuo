/**
 * @file
 * VuoCompiler interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILER_H
#define VUOCOMPILER_H

class VuoCompilerNodeClass;
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerType.hh"
#include "VuoRunner.hh"


/**
 * This class compiles node classes, types, and library modules. It compiles and links compositions.
 *
 * Node classes, types, library modules, and compositions are all compiled to [LLVM](http://llvm.org/)
 * bitcode. LLVM bitcode is one of the file formats used by the [Clang](http://clang.llvm.org/) compiler.
 *
 * When a composition is linked, the compiled composition and all of its node classes, types, library modules,
 * and other dependencies are combined to create one of the following: an executable, a dynamic library, or
 * or separate dynamic libraries for the composition and its resources.
 *
 * The easiest way to compile, link, and run a composition is to use the VuoCompiler factory methods
 * to create a VuoRunner that's ready to run the composition:
 *
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionString()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionString()
 *
 * Executables and dynamic libraries created by VuoCompiler link to Vuo.framework, so they can't be
 * distributed without it. Currently, these executables and dynamic libraries contain an absolute path
 * to Vuo.framework, so they can't easily be distributed at all; this will be improved in later versions of Vuo.
 *
 * @see DevelopingApplications
 */
class VuoCompiler
{
private:
	map<string, VuoCompilerNodeClass *> nodeClasses;
	map<string, VuoCompilerType *> types;
	map<string, VuoCompilerModule *> libraryModules;
	map<string, bool> isNodeClassAndTypeSearchPathLoaded;
	vector<string> moduleSearchPaths;  ///< Search paths for node classes, types, and library modules.
	vector<string> headerSearchPaths;  ///< Search paths for header/include files.
	vector<string> librarySearchPaths;  ///< Search paths for libraries (other than Vuo library modules).
	vector<string> preferredLibrarySearchPaths;
	vector<string> frameworkSearchPaths;
	vector<string> preferredFrameworkSearchPaths;
	llvm::sys::Path clangPath;
	string telemetry;
	string target;
	bool isVerbose;

	void loadModulesIfNeeded(void);
	void loadModules(string path);
	void reifyPortTypes(void);
	void linkCompositionToCreateExecutableOrDynamicLibrary(string compiledCompositionPath, string linkedCompositionPath, bool isDylib);
	set<string> getDependenciesForComposition(const string &compiledCompositionPath);
	void getDependenciesRecursively(const string &dependency, set<string> &dependencies);
	void getLinkerInputs(const set<string> &dependencies, set<Module *> &modules, set<string> &libraries, set<string> &frameworks);
	void link(string outputPath, const set<Module *> &modules, const set<string> &libraries, const set<string> &frameworks, bool isDylib);
	Module * readModuleFromC(string inputPath);
	static Module * readModuleFromBitcode(string inputPath);
	static bool writeModuleToBitcode(Module *module, string outputPath);
	void setTargetForModule(Module *module, string target = "");
	static vector<string> getCoreVuoDependencies(void);
	string getRuntimeMainDependency(void);
	string getRuntimeDependency(void);
	static string getModuleNameForPath(string path);
	llvm::sys::Path getClangPath(void);
	string getCompositionStubPath(void);

	friend class TestVuoCompilerBitcodeGenerator;

protected:
	void addModuleSearchPath(string path);

	friend class TestVuoCompiler;		///< TestVuoCompiler needs to add a search path for its own private nodes/types.
	friend class TestCompositionExecution;	///< TestCompositionExecution needs to add a search path for its own private nodes.

public:
	VuoCompiler(void);
	static llvm::sys::Path getVuoFrameworkPath();
	static string getUserModulesPath();
	static string getSystemModulesPath();
	void addNodeClass(VuoCompilerNodeClass *nodeClass);
	void compileComposition(VuoCompilerBitcodeGenerator *generator, string outputPath);
	void compileComposition(string inputPath, string outputPath);
	void compileCompositionString(const string &compositionString, string outputPath);
	void compileModule(string inputPath, string outputPath);
	void linkCompositionToCreateExecutable(string inputPath, string outputPath);
	void linkCompositionToCreateDynamicLibrary(string inputPath, string outputPath);
	void linkCompositionToCreateDynamicLibraries(string compiledCompositionPath, string linkedCompositionPath, string &newLinkedResourcePath, vector<string> &alreadyLinkedResourcePaths, set<string> &alreadyLinkedResources);
	VuoCompilerNodeClass * getNodeClass(const string &id);
	map<string, VuoCompilerNodeClass *> getNodeClasses(void);
	VuoCompilerType * getType(const string &id);
	void listNodeClasses(const string &format = "");
	void addHeaderSearchPath(const string &path);
	void addLibrarySearchPath(const string &path);
	void addPreferredLibrarySearchPath(const string &path);
	void clearPreferredLibrarySearchPaths();
	void addFrameworkSearchPath(const string &path);
	void addPreferredFrameworkSearchPath(const string &path);
	void setTelemetry(const string &telemetry);
	void setTarget(const string &target);
	void setVerbose(bool isVerbose);
	void setClangPath(const string &clangPath);
	string getCompositionLoaderPath(void);
	void print(void);

	static VuoRunner * newSeparateProcessRunnerFromCompositionFile(string compositionFilePath);
	static VuoRunner * newSeparateProcessRunnerFromCompositionString(string composition);
	static VuoRunner * newCurrentProcessRunnerFromCompositionFile(string compositionFilePath);
	static VuoRunner * newCurrentProcessRunnerFromCompositionString(string composition);
};


#endif
