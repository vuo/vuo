/**
 * @file
 * VuoCompiler interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerCompatibility.hh"
#include "VuoFileUtilities.hh"
#include "VuoLinkerInputs.hh"
#include "VuoModuleCompilerResults.hh"

class VuoCompilerComposition;
class VuoCompilerDelegate;
class VuoCompilerEnvironment;
class VuoCompilerIssues;
class VuoCompilerModule;
class VuoCompilerNodeClass;
class VuoCompilerType;
class VuoDirectedAcyclicGraph;
class VuoDirectedAcyclicNetwork;
class VuoModuleCompiler;
class VuoNode;
class VuoNodeSet;
class VuoPublishedPort;
class VuoRunner;
class VuoRunningCompositionLibraries;

/**
 * This is the central class for compiling modules and compositions. Its main responsibilities are:
 *
 * - Compiling node classes, types, and library modules to LLVM bitcode so they can be used for
 *     compiling and linking compositions.
 * - Compiling and linking compositions to one of the following formats so they can be run.
 *    - Executable — to be executed directly or wrapped in an app bundle.
 *    - Dynamic library — to be linked into a larger project or loaded in a process.
 *    - Multiple dynamic libraries — to be used for live editing in the Vuo editor.
 *
 * A VuoCompiler instance encapsulates a set of VuoCompilerEnvironment instances that hold the node classes,
 * types, and library modules that are available to the VuoCompiler. Some VuoCompilerEnvironments are shared
 * among all VuoCompilers, while others are unique to one or a few VuoCompilers.
 *
 * @see DevelopingApplications
 */
class VuoCompiler
{
public:
	/**
	 * Options for optimizations when building a composition.
	 */
	enum Optimization
	{
		Optimization_ModuleCaches,  ///< The composition binary will link to module cache dylibs on the user's system. These dylibs are created in advance of linking the composition; see VuoCompiler::prepareModuleCaches().
		Optimization_ExistingModuleCaches,  ///< Like Optimization_ModuleCaches, but skips ensuring that the module cache dylibs are up to date.
		Optimization_NoModuleCaches  ///< The composition binary will not link to any module cache dylibs.
	};

private:
	static set<VuoCompiler *> allCompilers;  ///< All VuoCompiler instances that have been constructed and not yet destroyed.
	static dispatch_queue_t environmentQueue;  ///< Synchronizes access to the VuoCompilerEnvironment data members and `allCompilers`. It's OK to call `llvmQueue` from this queue.
	static map<string, vector< vector<VuoCompilerEnvironment *> > > sharedEnvironments;  ///< LLVM target triple (1st dimension), built-in, system, user scope (2nd dimension) and installed, generated (3rd dimension)
	static map<string, map<string, vector<VuoCompilerEnvironment *> > > environmentsForCompositionFamily;  ///< LLVM target triple (1st dimension), path of directory containing the composition (2nd dimension), and environments for a composition and its local subcompositions (3rd dimension)
	vector< vector<VuoCompilerEnvironment *> > environments;  ///< Environments available to this VuoCompiler instance, in order from broadest to narrowest.
	VuoCompilerEnvironment *generatedEnvironment;  ///< The environment in which this VuoCompiler instance will place modules that it generates.
	string lastCompositionBaseDir;  ///< The base (top-level composition's) directory for the path most recently passed to @ref VuoCompiler::setCompositionPath.
	bool shouldLoadAllModules;  ///< If true, all available modules are loaded the first time @ref loadModulesIfNeeded is called without specifying the modules. If false, only the specified modules and their dependencies are loaded.
	bool hasLoadedAllModules;  ///< False until the first time that all available modules are loaded due to @ref shouldLoadAllModules being true.
	dispatch_queue_t modulesToLoadQueue;  ///< Synchronizes access to @ref shouldLoadAllModules and @ref hasLoadedAllModules.
	static map<VuoCompilerEnvironment *, map<string, pair<VuoCompilerModule *, dispatch_group_t>>> invalidatedModulesAwaitingRecompilation;  ///< Modules that have become outdated because other modules they depend on have been modified, along with dispatch groups to wait on until they're brought up-to-date.
	static map<VuoCompilerEnvironment *, set<VuoCompilerModule *>> addedModulesAwaitingReification;  ///< New modules that have been loaded except for having their dependencies reified.
	static map<VuoCompilerEnvironment *, set<pair<VuoCompilerModule *, VuoCompilerModule *>>> modifiedModulesAwaitingReification;  ///< Modified modules that have been loaded except for having their dependencies reified.
	dispatch_group_t modulesLoading;  ///< Enables a function to wait until all calls to @ref VuoCompiler::loadModulesIfNeeded() have completed.
	dispatch_group_t moduleSourceCompilersExist;  ///< Enables a function to wait until all scheduled asynchronous compilations of subcompositions or specialized node classes have completed.
	static dispatch_group_t moduleSourceCompilersExistGlobally;  ///< Enables @ref reset to wait until all VuoCompiler instances asynchronously compiling subcompositions or specialized node classes have been destroyed.
	dispatch_group_t moduleCacheBuilding;  ///< Enables the destructor to wait until asynchronous building of the module cache has completed.
	VuoDirectedAcyclicNetwork *dependencyGraph;  ///< A full dependency graph, containing all modules that have been loaded and their dependencies.
	VuoDirectedAcyclicNetwork *compositionDependencyGraph;  ///< A partial dependency graph, containing all subcompositions (loaded or not) and the node classes that are their direct dependencies.
	static string vuoFrameworkInProgressPath;  ///< The path to use for Vuo.framework during a call to @ref VuoCompiler::generateBuiltInModuleCaches.
	string clangPath;
	string target;  ///< The LLVM target triple (architecture, vendor, OS) used when compiling a module or composition.
	string requestedTarget;  ///< The LLVM target triple that the caller specified when creating this VuoCompiler instance (or empty if none).
	bool isVerbose;
	string dependencyOutputPath;  ///< The path of the dependency (.d) file to generate when compiling a module.
	bool _shouldShowSplashWindow;
	VuoCompilerDelegate *delegate;
	dispatch_queue_t delegateQueue;  ///< Synchronizes access to @ref delegate.

	void applyToInstalledEnvironments(std::function<void(VuoCompilerEnvironment *)> doForEnvironment);
	void applyToAllEnvironments(std::function<void(VuoCompilerEnvironment *)> doForEnvironment);
	VuoCompilerEnvironment * environmentAtBroaderScope(VuoCompilerEnvironment *envA, VuoCompilerEnvironment *envB);
	VuoCompilerEnvironment * generatedEnvironmentAtSameScopeAs(VuoCompilerEnvironment *env);

	static void reset(void);
	VuoDirectedAcyclicNetwork * makeDependencyNetwork(const vector< vector<VuoCompilerEnvironment *> > &environments, VuoDirectedAcyclicGraph * (^graphForEnvironment)(VuoCompilerEnvironment *));
	VuoCompilerModule * loadModuleIfNeeded(const string &moduleKey);
	void loadModulesIfNeeded(const set<string> &moduleKeys = set<string>());
	set<dispatch_group_t> loadModulesAndSources(const set<string> &modulesAddedKeys, const set<string> &modulesModifiedKeys, const set<string> &modulesRemovedKeys, const set<string> &sourcesAddedKeys, const set<string> &sourcesModifiedKeys, const set<string> &sourcesRemovedKeys, bool willLoadAllModules, bool shouldRecompileSourcesIfUnchanged, VuoCompilerEnvironment *currentEnvironment, VuoCompilerIssues *issuesForCurrentEnvironment, std::function<void(void)> moduleLoadedCallback, const string &moduleAddedOrModifiedSourceCode);
	void findDependentModulesAndSources(map<VuoCompilerEnvironment *, set<string> > &changedModules, const vector<VuoDirectedAcyclicNetwork *> &searchDependencyGraphs, VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph, bool includeIndirectDependents, map<VuoCompilerEnvironment *, set<string> > &modulesDepOnChangedModules_this, map<VuoCompilerEnvironment *, set<string> > &modulesDepOnChangedModules_other, map<VuoCompilerEnvironment *, set<string> > &sourcesDepOnChangedModules_this, map<VuoCompilerEnvironment *, set<string> > &sourcesDepOnChangedModules_other);
	void loadedModules(map<string, VuoCompilerModule *> modulesAdded, map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModified, map<string, VuoCompilerModule *> modulesRemoved, VuoCompilerIssues *issues, void *delegateData, VuoCompilerEnvironment *currentEnvironment);
	bool reifyPortTypes(VuoCompilerNodeClass *nodeClass, std::function<VuoCompilerType *(const string &)> lookUpType);
	void reifyGenericPortTypes(VuoCompilerComposition *composition);
	void reifyGenericPortTypes(VuoNode *node);
	Module * compileCompositionToModule(VuoCompilerComposition *composition, const string &moduleKey, bool isTopLevelComposition, VuoCompilerIssues *issues);
	void linkCompositionToCreateExecutableOrDynamicLibrary(string compiledCompositionPath, string linkedCompositionPath, Optimization optimization, bool isDylib, const vector<string> &rPaths, bool shouldAdHocCodeSign = true);
	set<string> getDependenciesForComposition(const string &compiledCompositionPath);
	set<string> getDependenciesForComposition(const set<string> &directDependencies, bool checkCompatibility);
	static string getLibraryPath(const string &dependency, vector<string> librarySearchPaths);
	void makeModuleCachesAvailable(bool shouldUseExistingBuiltInCaches, bool shouldUseExistingOtherCaches, const string &target = "");
	vector<shared_ptr<VuoModuleCacheRevision>> useCurrentModuleCaches(Optimization optimization);
	void link(string outputPath, const VuoLinkerInputs &linkerInputs, bool isDylib, const vector<string> &rPaths, bool shouldAdHocCodeSign = true, VuoCompilerIssues *issues = nullptr);
	static void adHocCodeSign(string path);
	static Module *readModuleFromBitcode(VuoFileUtilities::File *inputFile, string arch);
	static Module *readModuleFromBitcodeData(char *inputData, size_t inputDataBytes, string arch, set<string> &availableArchs, string &error);
	static void verifyModule(Module *module, VuoCompilerIssues *issues);
	static void writeModuleToBitcode(Module *module, string target, string outputPath, VuoCompilerIssues *issues);
	VuoNode * createPublishedNode(const string &nodeClassName, const vector<VuoPublishedPort *> &publishedPorts);
	static void setTargetForModule(Module *module, string target);
	static string getProcessTarget(void);
	VuoCompilerModule * getModule(const string &moduleKey);
	vector<string> getRunPathSearchPaths(VuoCompilerEnvironment *narrowestScope);
	static vector<string> getCoreVuoDependencies(void);
	static string getRuntimeMainDependency(void);
	static string getRuntimeDependency(void);
	void addModuleSearchPath(string path);
	string getClangPath(void);
	void setClangPath(const string &clangPath);
	string getCompositionStubPath(void);

	friend class VuoCompilerEnvironment;
	friend class VuoLinkerInputs;
	friend class VuoModuleCache;
	friend class TestCompilerDelegate;
	friend class TestCompilingAndLinking;  ///< TestCompilingAndLinking calls `reset()`.
	friend class TestCompositionExecution;  ///< TestCompositionExecution needs to add a search path for its own private nodes.
	friend class TestEventDropping;  ///< TestEventDropping calls `reset()`.
	friend class TestModuleLoading;  ///< TestModuleLoading calls `reset()`.
	friend class TestModuleCaches;  ///< TestModuleCaches calls `getProcessTarget()`.
	friend class TestNodes;  ///< TestNodes calls `getProcessTarget()`.
	friend class TestSubcompositions;  ///< TestSubcompositions calls `reset()`.
	friend class TestVuoCompiler;  ///< TestVuoCompiler needs to add a search path for its own private nodes/types.
	friend class TestVuoCompilerBitcodeGenerator;
	friend class TestVuoCompilerCompatibility;  ///< TestVuoCompilerCompatibility calls `getProcessTarget()`.
	friend class TestVuoCompilerModule;
	friend class TestVuoIsfModuleCompiler;  ///< TestVuoIsfModuleCompiler calls `writeModuleToBitcode()`.

public:
	VuoCompiler(const string &compositionPath = "", string target = "");
	~VuoCompiler(void);
	void setDelegate(VuoCompilerDelegate *delegate);
	void setCompositionPath(const string &compositionPath);
	static Module *readModuleFromBitcode(string inputPath, string arch);
	static void destroyModule(VuoCompilerModule *module);
	static void destroyLlvmModule(Module *module);
	void compileComposition(VuoCompilerComposition *composition, string outputPath, bool isTopLevelComposition, VuoCompilerIssues *issues);
	void compileComposition(string inputPath, string outputPath, bool isTopLevelComposition, VuoCompilerIssues *issues);
	void compileCompositionString(const string &compositionString, string outputPath, bool isTopLevelComposition, VuoCompilerIssues *issues);
	VuoModuleCompiler * createModuleCompiler(const string &moduleKey, const string &inputPath, const map<string, string> &typeNameReplacements = {});
	VuoModuleCompilerResults compileModuleInMemory(const string &inputPath, const string &overriddenSourceCode = "", const map<string, string> &typeNameReplacements = {});
	void compileModule(const string &inputPath, const string &outputPath);
	void generateHeaderForModule(const string &inputPath, const string &outputPath);
	void linkCompositionToCreateExecutable(string inputPath, string outputPath, Optimization optimization, string rPath="", bool shouldAdHocCodeSign = true);
	void linkCompositionToCreateDynamicLibrary(string inputPath, string outputPath, Optimization optimization, bool shouldAdHocCodeSign = true);
	void linkCompositionToCreateDynamicLibraries(string compiledCompositionPath, string linkedCompositionPath, VuoRunningCompositionLibraries *runningCompositionLibraries);
	set<string> getDirectDependenciesForComposition(VuoCompilerComposition *composition);
	set<string> getDependenciesForComposition(VuoCompilerComposition *composition);
	set<string> getDylibDependencyPathsForComposition(VuoCompilerComposition *composition);
	VuoCompilerCompatibility getCompatibilityOfDependencies(const set<string> &dependencies);
	void prepareModuleCaches(void);
	static void generateBuiltInModuleCache(string vuoFrameworkPath, string target, bool onlyGenerateModules);
	string getTarget(void);
	string getArch(void);
	static string getTargetArch(string target);
	void setLoadAllModules(bool shouldLoadAllModules);
	VuoNode * createNode(VuoCompilerNodeClass *nodeClass, string title="", double x=0, double y=0);
	VuoNode * createNode(VuoCompilerNodeClass *nodeClass, VuoNode *nodeToCopyMetadataFrom);
	VuoNode * createPublishedInputNode(vector<VuoPublishedPort *> publishedInputPorts);
	VuoNode * createPublishedOutputNode(vector<VuoPublishedPort *> publishedOutputPorts);
	void installNodeClassAtCompositionLocalScope(const string &sourcePath);
	void uninstallNodeClassAtCompositionLocalScope(const string &sourcePath);
	void overrideInstalledNodeClass(const string &sourcePath, const string &sourceCode);
	void revertOverriddenNodeClass(const string &sourcePath);
	VuoCompilerNodeClass * getNodeClass(const string &nodeClassName);
	map<string, VuoCompilerNodeClass *> getNodeClasses(void);
	VuoCompilerType * getType(const string &typeName);
	map<string, VuoCompilerType *> getTypes(void);
	VuoCompilerModule * getLibraryModule(const string &libraryModuleName);
	map<string, VuoCompilerModule *> getLibraryModules();
	map<string, VuoNodeSet *> getNodeSets();
	VuoNodeSet * getNodeSetForName(const string &name);
	void listNodeClasses(const string &format = "");
	static string getVuoFrameworkPath(void);
	static string getModuleKeyForPath(string path);
	bool isCompositionLocalModule(string moduleKey);
	string getCompositionLocalModulesPath(void);
	string getCompositionLocalPath(void);
	void addHeaderSearchPath(const string &path);
	void addLibrarySearchPath(const string &path);
	void addFrameworkSearchPath(const string &path);
	void setVerbose(bool isVerbose);
	void setShouldPotentiallyShowSplashWindow(bool potentiallyShow);
	void setDependencyOutput(const string &path);
	bool shouldShowSplashWindow();
	string getCompositionLoaderPath(void);
	void print(void);

	static VuoRunner * newSeparateProcessRunnerFromCompositionFile(string compositionFilePath, VuoCompilerIssues *issues);
	static VuoRunner * newSeparateProcessRunnerFromCompositionString(string composition, string processName, string workingDirectory, VuoCompilerIssues *issues);
	static VuoRunner * newCurrentProcessRunnerFromCompositionFile(string compositionFilePath, VuoCompilerIssues *issues);
	static VuoRunner * newCurrentProcessRunnerFromCompositionString(string composition, string workingDirectory, VuoCompilerIssues *issues);

	static llvm::LLVMContext *globalLLVMContext;

private:
	void *p;
#ifdef VUO_PRO
#include "../compiler/pro/VuoCompiler_Pro.hh"
#endif
};
