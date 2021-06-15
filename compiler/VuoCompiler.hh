/**
 * @file
 * VuoCompiler interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerCompatibility.hh"
#include "VuoFileUtilities.hh"
#include "VuoFileWatcher.hh"
#include "VuoDirectedAcyclicGraph.hh"

class VuoCompilerComposition;
class VuoCompilerDelegate;
class VuoCompilerGenericType;
class VuoCompilerIssues;
class VuoCompilerModule;
class VuoCompilerNodeClass;
class VuoCompilerType;
class VuoModuleCompilationQueue;
class VuoNode;
class VuoNodeSet;
class VuoPublishedPort;
class VuoRunner;
class VuoRunningCompositionLibraries;


/**
 * This class compiles node classes, types, and library modules. It compiles and links compositions.
 *
 * Node classes, types, library modules, and compositions are all compiled to [LLVM](https://llvm.org/)
 * bitcode. LLVM bitcode is one of the file formats used by the [Clang](https://clang.llvm.org/) compiler.
 *
 * When a composition is linked, the compiled composition and all of its node classes, types, library modules,
 * and other dependencies are combined to create one of the following: an executable, a dynamic library, or
 * or separate dynamic libraries for the composition and its resources.
 *
 * All VuoCompiler instances share a basic set of node classes, types, library modules, and other dependencies.
 * Each VuoCompiler instance can add its own dependencies on top of that basic set.
 *
 * The easiest way to compile, link, and run a composition is to use the VuoCompiler factory methods
 * to create a VuoRunner that's ready to run the composition:
 *
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionString()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionString()
 *
 * When building a composition, this class optimizes the build in different ways depending on the functions,
 * and in some cases the arguments, that you use. Some functions for linking a composition accept an
 * Optimization argument. The rest default to the "faster build" optimization (Optimization_FastBuild).
 * The "faster build" optimization works by caching a library on the user's filesystem and reusing it
 * each time a composition is linked. The caching takes a while, and may delay the building of the first
 * composition. However, if you call prepareForFastBuild(), the caching can be done in advance.
 *
 * If you plan to distribute an executable or dynamic library built from a composition, use either
 * linkCompositionToCreateExecutable() or linkCompositionToCreateDynamicLibrary(), and choose the
 * "smaller binary" optimization. If building an executable, pass an rpath (such as "@loader_path/../Frameworks")
 * to enable the executable to find Vuo.framework. Since the executable or dynamic library links to
 * Vuo.framework, you need to distribute it along with the build composition.
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
		Optimization_FastBuild,  ///< Speed up the build by having the composition binary link to cache libraries on the user's system. See @ref prepareForFastBuild.
		Optimization_FastBuildExistingCache,  ///< Like Optimization_FastBuild, but skips ensuring that the cache libraries are up to date.
		Optimization_SmallBinary  ///< Don't link to any cache libraries.
	};

private:
	class Environment;

	/**
	 * Information about a module or subcomposition file that the compiler has seen, though not necessarily loaded.
	 */
	class ModuleInfo
	{
	public:
		ModuleInfo(Environment *environment, const string &searchPath, const string &relativePath, bool isSourceFile, bool isSubcomposition);
		ModuleInfo(Environment *environment, const string &searchPath, VuoFileUtilities::File *file, bool isSourceFile, bool isSubcomposition);
		~ModuleInfo(void);
		Environment * getEnvironment(void) const;
		string getSearchPath(void) const;
		string getModuleKey(void) const;
		VuoFileUtilities::File * getFile(void) const;
		string getSourceCode(void) const;
		void setSourceCode(const string &sourceCode);
		void revertSourceCode(void);
		bool isSourceCodeOverridden(void) const;
		void setSourceCodeOverridden(bool overridden);
		bool isNewerThan(ModuleInfo *other) const;
		bool isNewerThan(unsigned long seconds) const;
		bool isOlderThan(unsigned long seconds) const;
		void setLastModifiedToNow(void);
		set<string> getContainedNodeClasses(void) const;
		int getLongestDownstreamPath(void) const;
		void setLongestDownstreamPath(int pathLength);
		bool getAttempted(void) const;
		void setAttempted(bool attempted);
		dispatch_group_t getLoadingGroup(void) const;
		void dump() const;

	private:
		void initialize(Environment *environment, const string &searchPath, VuoFileUtilities::File *file, bool isSourceFile, bool isSubcomposition);

		Environment *environment;  ///< The environment that owns (or will own) this module.
		string searchPath;   ///< The module search path in which the file is located.
		string moduleKey;   ///< The module key, based on the file name.
		VuoFileUtilities::File *file;  ///< The file, which may be in a directory or an archive.
		string sourceCode;  ///< If this file is a subcomposition, its source code. May be the file contents, or may differ if there are unsaved changes.
		bool sourceCodeOverridden;  ///< If this file is a subcomposition, true if the source code has been set to something other than the file contents.
		unsigned long lastModified;   ///< The time (in seconds since a reference date) when the file was last modified, as of when this instance was constructed.
		set<string> containedNodeClasses;  ///< If this file is a subcomposition, the class names of all node classes contained at the top level of the subcomposition.
		int longestDownstreamPath;  ///< If this file is a subcomposition, the number of vertices in the longest path downstream of it in the composition dependency graph.
		bool attempted;  ///< True if this file is a compiled module and its loading has been attempted, or if this file is a source file and its compilation has been scheduled (and possibly completed).
		dispatch_group_t loading;  ///< If this file is a source file, its compiling and loading is scheduled on this dispatch group so that callers can wait on it.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
		void *p;
#pragma clang diagnostic pop
#ifdef VUO_PRO
#include "pro/VuoCompilerModuleInfo_Pro.hh"
#endif
	};

	class DependencyGraphVertex : public VuoDirectedAcyclicGraph::Vertex
	{
	public:
		static DependencyGraphVertex * vertexForDependency(const string &dependency, VuoDirectedAcyclicGraph *graph);
		string getDependency(void);
		Environment * getEnvironment(void);
		void setEnvironment(Environment *environment);
		string key(void);

	private:
		string dependency;
		Environment *environment;
		bool compatible;
	};

	/**
	 * Efficiently iterates through collections of ModuleInfo objects in Environment, without making copies of the collections.
	 */
	class ModuleInfoIterator
	{
	public:
		ModuleInfoIterator(map<string, map<string, ModuleInfo *> > *allModuleInfos, const string &overriddenCompiledModuleCachePath);
		ModuleInfoIterator(map<string, map<string, ModuleInfo *> > *allModuleInfos, const string &overriddenCompiledModuleCachePath, const set<string> &searchModuleKeys);
		ModuleInfo * next(void);

	private:
		void initialize(void);

		map<string, map<string, ModuleInfo *> > *allModuleInfos;
		set<string> searchModuleKeys;
		bool hasSearchModuleKeys;
		map<string, map<string, ModuleInfo *> >::iterator currSearchPath;
		map<string, ModuleInfo *>::iterator currModuleKey;
		bool hasCurrModuleKey;
		string overriddenCompiledModuleCachePath;
		set<string> overriddenModuleKeys;
	};

	/**
	 * A set of modules (node classes, types, libraries) and search paths at a certain level of scope.
	 *
	 * The VuoCompiler class refers to the following independent levels of scope. Listed in order from
	 * broadest to narrowest:
	 *
	 *    - Built-in — Modules officially distributed with Vuo.
	 *    - System — Modules installed in the System Modules folder.
	 *    - User — Modules installed in the User Modules folder.
	 *    - Composition-family — Modules installed in a composition's Modules folder.
	 *    - Composition — Modules required to compile the current top-level composition.
	 *
	 * At each level of scope, the VuoCompiler has two kinds of modules:
	 *
	 *    - Original modules — Modules loaded from files.
	 *    - Generated modules — Modules generated by the compiler and loaded from memory. The generated modules
	 *         at a level of scope are those required to compile compositions installed at that level of scope.
	 *
	 * Each combination of level of scope and kind of module comprises an Environment.
	 *
	 * Modules at one level of scope can refer to modules at broader levels of scope.
	 * For example, a node class at the system level can use types defined at the system level and/or
	 * the built-in level.
	 *
	 * The set of all modules and search paths available to a VuoCompiler instance consists of
	 * all of its Environments layered on top of one another (like CSS). If a module of the same key
	 * exists at multiple levels, the one at the narrowest level of scope is used.
	 */
	class Environment : public VuoFileWatcherDelegate
	{
	private:
		string target;  ///< The LLVM target triple this Environment uses.
		bool builtIn;  ///< True if this environment is for the built-in level of scope, false otherwise.
		bool generated;  ///< True if this environment is for generated modules, false if for original modules.
		set<VuoCompiler *> compilersToNotify;  ///< The compilers that this Environment notifies when it loads/unloads modules as a result of changes to the watched search paths.
		dispatch_queue_t compilersToNotifyQueue;  ///< Synchronizes access to `compilersToNotify`.
		set<VuoFileWatcher *> moduleSearchPathWatchers;  ///< Dispatch sources watching module search paths for changes.
		map<string, VuoCompilerNodeClass *> nodeClasses;  ///< Node classes loaded, plus specialized node classes generated by the compiler.
		map<string, VuoCompilerType *> types;  ///< Types loaded.
		set<VuoCompilerGenericType *> genericTypes;  ///< Generic types generated by the compiler for node classes.
		map<string, VuoNodeSet *> nodeSetForName;  ///< Node sets loaded.
		map<string, VuoCompilerModule *> libraryModules;  ///< Library modules loaded.
		map<string, dispatch_group_t> specializedModulesLoading;  ///< Module keys of possibly specialized-generic modules that are in the process of being checked if they are specialized and, if so, generated and loaded.
		map<string, map<string, ModuleInfo *> > moduleFilesAtSearchPath;  ///< The module files seen (but not necessarily loaded) from moduleSearchPaths.
		map<string, map<string, ModuleInfo *> > sourceFilesAtSearchPath;  ///< The source files seen (but not necessarily loaded) from moduleSearchPaths.
		vector<string> moduleSearchPaths;  ///< Search paths for node classes, types, and library modules.
		vector<string> headerSearchPaths;  ///< Search paths for header/include files.
		vector<string> librarySearchPaths;  ///< Search paths for libraries (other than Vuo library modules).
		vector<string> frameworkSearchPaths;  ///< Search paths for frameworks.
		vector<string> expatriateSourceFiles;  ///< Paths of source files belonging to this environment that are located elsewhere than a module search path.
		string moduleCachePath;  ///< The directory that contains the caches for this environment's scope.
		string currentModuleCacheDylib;  ///< The path of the most recent revision of this environment's module cache dylib.
		unsigned long lastModuleCacheRebuild;   ///< The time (in seconds since a reference date) when this Environment instance last scheduled its cache to be rebuilt; or, if none and this is not a built-in environment, the time when the module cache dylib was last modified; otherwise, 0.
		bool isModuleCacheableDataDirty;  ///< True if the module cache may be out-of-date. (Ignored until @ref isModuleCacheInitialized is true.)
		bool isModuleCacheInitialized;  ///< True if the module cache has been checked for the first time.
		bool isModuleCacheAvailable;  ///< True if the module cache has been checked (and rebuilt if needed) and is now up-to-date.
		set<string> moduleCacheContents;  ///< The keys/names of modules and dependencies contained in the module cache.
		set<string> moduleCacheDylibs;  ///< The paths of the dylibs that the module cache dylib links to.
		set<string> moduleCacheFrameworks;  ///< The names of the frameworks that the module cache dylib links to.
		static map<string, VuoFileUtilities::File *> moduleCacheFileForLocking;  ///< The files locked to keep different VuoCompiler processes from interfering with each other when rebuilding the module cache, indexed by path. Static so they can be reused if an Environment is destroyed and recreated, e.g. with @ref VuoCompiler::reset.
		static dispatch_queue_t moduleCacheBuildingQueue;  ///< Ensures that module caches are rebuilt one at a time in the order they were requested.
		VuoDirectedAcyclicGraph *dependencyGraph;  ///< A full dependency graph, containing all modules that have been loaded and their dependencies.
		VuoDirectedAcyclicGraph *compositionDependencyGraph;  ///< A partial dependency graph, containing all subcompositions (loaded or not) and the node classes that are their direct dependencies.
		VuoModuleCompilationQueue *moduleCompilationQueue;  ///< Ensures that successive versions of source files are compiled in the right order.

		void updateModulesAtSearchPath(const string &path);
		void updateModuleAtSearchPath(const string &moduleSearchPath, const string &moduleRelativePath);
		void updateSourceFilesAtSearchPath(const string &path);
		void startWatchingModuleSearchPath(const string &moduleSearchPath);
		VuoCompilerModule * loadModule(ModuleInfo *moduleInfo);

		friend class TestCompilingAndLinking;

	public:
		dispatch_queue_t moduleSearchPathContentsChangedQueue;  ///< Synchronizes calls to `moduleSearchPathContentsChanged()`.
		static const string pidCacheDirPrefix;  ///< The prefix of the names of pid-specific directories in the module cache.

		explicit Environment(string target, bool builtIn, bool generated);
		virtual ~Environment(void);
		string getTarget();
		void addCompilerToNotify(VuoCompiler *compiler);
		void removeCompilerToNotify(VuoCompiler *compiler);
		map<string, VuoCompilerNodeClass *> getNodeClasses(void);
		VuoCompilerNodeClass * getNodeClass(const string &moduleKey);
		VuoCompilerNodeClass * takeNodeClass(const string &moduleKey);
		map<string, VuoCompilerType *> getTypes(void);
		VuoCompilerType * getType(const string &moduleKey);
		map<string, VuoNodeSet *> getNodeSets();
		VuoCompilerModule *getLibraryModule(const string &libraryModuleName);
		map<string, VuoCompilerModule *> getLibraryModules(void);
		VuoCompilerModule * findModule(const string &moduleKey);
		VuoNodeSet * findNodeSet(const string &name);
		void addModuleSearchPath(const string &path, bool shouldWatch = true);
		vector<string> getModuleSearchPaths(void);
		void addHeaderSearchPath(const string &path);
		vector<string> getHeaderSearchPaths(void);
		void addLibrarySearchPath(const string &path);
		vector<string> getLibrarySearchPaths(void);
		void addFrameworkSearchPath(const string &path);
		void setModuleCachePath(const string &path, bool shouldAddModuleSearchPath);
		string getCompiledModuleCachePath(void);
		string getOverriddenCompiledModuleCachePath(void);
		vector<string> getFrameworkSearchPaths(void);
		VuoDirectedAcyclicGraph * getDependencyGraph(void);
		VuoDirectedAcyclicGraph * getCompositionDependencyGraph(void);
		void addExpatriateSourceFile(const string &sourcePath);
		void removeExpatriateSourceFile(const string &sourcePath);
		ModuleInfo * listModule(const string &moduleKey);
		ModuleInfoIterator listModules(const set<string> &moduleKeys);
		ModuleInfoIterator listAllModules(void);
		ModuleInfo * listSourceFile(const string &moduleKey);
		ModuleInfoIterator listSourceFiles(const set<string> &moduleKeys);
		ModuleInfoIterator listAllSourceFiles(void);
		static vector<string> getBuiltInModuleSearchPaths(void);
		static vector<string> getBuiltInHeaderSearchPaths(void);
		static vector<string> getBuiltInLibrarySearchPaths(void);
		static vector<string> getBuiltInFrameworkSearchPaths(void);
		void addSearchPathsToSharedEnvironment(void);
		void stopWatchingModuleSearchPaths(void);
		void fileChanged(const string &moduleSearchPath);
		void moduleSearchPathContentsChanged(const string &moduleSearchPath);
		void moduleFileChanged(const string &modulePath, const string &moduleSourceCode, std::function<void(void)> moduleLoadedCallback, VuoCompiler *compiler, VuoCompilerIssues *issues = nullptr);
		void notifyCompilers(const set<VuoCompilerModule *> &modulesAdded, const set<pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified, const set<VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues, bool oldModulesInvalidated = true);
		set<VuoCompilerModule *> loadCompiledModules(const set<string> &moduleKeys, const map<string, string> &sourceCodeForModule);
		set<dispatch_group_t> loadSpecializedModules(const set<string> &moduleKeys, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
		set<dispatch_group_t> compileModulesFromSourceCode(const set<string> &moduleKeys, bool shouldRecompileIfUnchanged);
		set<VuoCompilerModule *> unloadCompiledModules(const set<string> &moduleKeys);
		void deleteModulesCompiledFromSourceCode(const set<string> &moduleKeys);
		void replaceNodeClass(VuoCompilerNodeClass *newNodeClass);
		void addToDependencyGraph(VuoCompilerModule *module);
		void removeFromDependencyGraph(VuoCompilerModule *module);
		void reifyPortTypes(const map<string, VuoCompilerType *> &inheritedTypes);
		void getCacheableModulesAndDependencies(set<string> &cacheableModulesAndDependencies, set<string> &dylibsNeededToLinkToThisCache, set<string> &frameworksNeededToLinkToThisCache);
		void useModuleCache(bool shouldUseExistingCache, VuoCompiler *compiler, set<string> cacheableModulesAndDependencies, set<string> dylibsNeededToLinkToCaches, set<string> frameworksNeededToLinkToCaches, unsigned long lastPrerequisiteModuleCacheRebuild);
		static void waitForModuleCachesToBuild(void);
		bool findInModuleCache(const string &moduleOrDependency, string &cachePath);
		string getCurrentModuleCacheDylib(void);
		unsigned long getLastModuleCacheRebuild(void);
		void modulesChanged(void);

		bool isBuiltInOriginal(void);
		bool isBuiltIn(void);
		bool isGenerated(void);
		string getName();

#ifdef VUO_PRO
#include "pro/VuoCompilerEnvironment_Pro.hh"
#endif
	};

	static set<VuoCompiler *> allCompilers;  ///< All VuoCompiler instances that have been constructed and not yet destroyed.
	static dispatch_queue_t environmentQueue;  ///< Synchronizes access to the Environment data members and `allCompilers`. It's OK to call `llvmQueue` from this queue.
	static map<string, vector< vector<Environment *> > > sharedEnvironments;  ///< LLVM target triple (1st dimension), built-in, system, user scope (2nd dimension) and installed, generated (3rd dimension)
	static map<string, map<string, vector<Environment *> > > environmentsForCompositionFamily;  ///< LLVM target triple (1st dimension), path of directory containing the composition (2nd dimension), and environments for a composition and its local subcompositions (3rd dimension)
	vector< vector<Environment *> > environments;  ///< Environments available to this VuoCompiler instance, in order from broadest to narrowest.
	string lastCompositionBaseDir;  ///< The base (top-level composition's) directory for the path most recently passed to @ref VuoCompiler::setCompositionPath.
	bool lastCompositionIsSubcomposition;  ///< True if the path most recently passed to @ref VuoCompiler::setCompositionPath is for a subcomposition installed in a Modules folder.
	bool shouldLoadAllModules;  ///< If true, all available modules are loaded the first time @ref loadModulesIfNeeded is called without specifying the modules. If false, only the specified modules and their dependencies are loaded.
	bool hasLoadedAllModules;  ///< False until the first time that all available modules are loaded due to @ref shouldLoadAllModules being true.
	dispatch_queue_t modulesToLoadQueue;  ///< Synchronizes access to @ref shouldLoadAllModules and @ref hasLoadedAllModules.
	dispatch_group_t moduleSourceCompilersExist;  ///< Enables a function to wait until all scheduled asynchronous compilations of subcompositions or specialized node classes have completed.
	static dispatch_group_t moduleSourceCompilersExistGlobally;  ///< Enables @ref reset to wait until all VuoCompiler instances asynchronously compiling subcompositions or specialized node classes have been destroyed.
	dispatch_group_t moduleCacheBuilding;  ///< Enables the destructor to wait until asynchronous building of the module cache has completed.
	VuoDirectedAcyclicNetwork *dependencyGraph;  ///< A full dependency graph, containing all modules that have been loaded and their dependencies.
	VuoDirectedAcyclicNetwork *compositionDependencyGraph;  ///< A partial dependency graph, containing all subcompositions (loaded or not) and the node classes that are their direct dependencies.
	static string vuoFrameworkInProgressPath;  ///< The path to use for Vuo.framework during a call to @ref VuoCompiler::generateBuiltInModuleCaches.
	string clangPath;
	string telemetry;
	string target;
	bool isVerbose;
	bool _shouldShowSplashWindow;
	VuoCompilerDelegate *delegate;
	dispatch_queue_t delegateQueue;  ///< Synchronizes access to @ref delegate.

	void applyToInstalledEnvironments(void (^doForEnvironment)(Environment *));
	void applyToInstalledEnvironments(void (^doForEnvironment)(Environment *, bool *, string), bool *, string);
	void applyToAllEnvironments(void (^doForEnvironment)(Environment *));

	static void reset(void);
	VuoDirectedAcyclicNetwork * makeDependencyNetwork(const vector< vector<Environment *> > &environments, VuoDirectedAcyclicGraph * (^graphForEnvironment)(Environment *));
	void loadModulesIfNeeded(const set<string> &moduleKeys = set<string>());
	set<dispatch_group_t> loadModulesAndSources(const set<string> &modulesAddedKeys, const set<string> &modulesModifiedKeys, const set<string> &modulesRemovedKeys, const set<string> &sourcesAddedKeys, const set<string> &sourcesModifiedKeys, const set<string> &sourcesRemovedKeys, bool willLoadAllModules, bool shouldRecompileSourcesIfUnchanged, Environment *currentEnvironment, VuoCompilerIssues *issuesForCurrentEnvironment, std::function<void(void)> moduleLoadedCallback, const string &moduleAddedOrModifiedSourceCode);
	void findDependentModulesAndSources(map<Environment *, set<string> > &changedModules, const vector<VuoDirectedAcyclicNetwork *> &searchDependencyGraphs, VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph, map<Environment *, set<string> > &modulesDepOnChangedModules_this, map<Environment *, set<string> > &modulesDepOnChangedModules_other, map<Environment *, set<string> > &sourcesDepOnChangedModules_this, map<Environment *, set<string> > &sourcesDepOnChangedModules_other);
	void loadedModules(map<string, VuoCompilerModule *> modulesAdded, map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModified, map<string, VuoCompilerModule *> modulesRemoved, VuoCompilerIssues *issues, void *delegateData, Environment *currentEnvironment);
	void loadNodeClassGeneratedAtRuntime(VuoCompilerNodeClass *nodeClass, Environment *env);
	void reifyGenericPortTypes(VuoCompilerComposition *composition);
	void reifyGenericPortTypes(VuoNode *node);
	Module * compileCompositionToModule(VuoCompilerComposition *composition, const string &moduleKey, bool isTopLevelComposition, VuoCompilerIssues *issues);
	void linkCompositionToCreateExecutableOrDynamicLibrary(string compiledCompositionPath, string linkedCompositionPath, Optimization optimization, bool isDylib, string rPath="", bool shouldAdHocCodeSign = true);
	set<string> getDependenciesForComposition(const string &compiledCompositionPath);
	set<string> getDependenciesForComposition(const set<string> &directDependencies, bool checkCompatibility);
	void getLinkerInputs(const set<string> &dependencies, Optimization optimization, set<Module *> &modules, set<string> &libraries, set<string> &frameworks);
	void getLinkerInputs(const set<string> &dependencies, Optimization optimization, set<string> &builtInModuleAndLibraryDependencies, set<string> &userModuleAndLibraryDependencies, map<string, set<string> > &builtInCacheDependencies, map<string, set<string> > &userCacheDependencies, set<Module *> &builtInModules, set<Module *> &userModules, set<string> &builtInLibraries, set<string> &userLibraries, set<string> &externalLibraries, set<string> &externalFrameworks);
	static string getLibraryPath(const string &dependency, vector<string> librarySearchPaths);
	void useModuleCache(bool shouldUseExistingBuiltInCaches, bool shouldUseExistingOtherCaches);
	bool findInModuleCache(const string &moduleOrDependency, string &cachePath, bool &isBuiltinCache) VuoWarnUnusedResult;
	void link(string outputPath, const set<Module *> &modules, const set<string> &libraries, const set<string> &frameworks, bool isDylib, string rPath="", bool shouldAdHocCodeSign = true, VuoCompilerIssues *issues = nullptr);
	static void adHocCodeSign(string path);
	Module *readModuleFromC(string inputPath, const vector<string> &headerSearchPaths, const vector<string> &extraArgs, VuoCompilerIssues *issues);
	static Module *readModuleFromBitcode(VuoFileUtilities::File *inputFile, string arch);
	static Module *readModuleFromBitcodeData(char *inputData, size_t inputDataBytes, string arch, set<string> &availableArchs, string &error);
	static bool writeModuleToBitcode(Module *module, string outputPath);
	VuoNode * createPublishedNode(const string &nodeClassName, const vector<VuoPublishedPort *> &publishedPorts);
	static void setTargetForModule(Module *module, string target);
	static string getTargetArch(string target);
	static string getProcessTarget(void);
	VuoCompilerModule * getModule(const string &moduleKey);
	static vector<string> getCoreVuoDependencies(void);
	static string getRuntimeMainDependency(void);
	static string getRuntimeDependency(void);
	void addModuleSearchPath(string path);
	string getClangPath(void);
	void setClangPath(const string &clangPath);
	string getCompositionStubPath(void);
	static string getCachePathForComposition(const string compositionDir);

	friend class TestVuoCompiler;		///< TestVuoCompiler needs to add a search path for its own private nodes/types.
	friend class TestCompositionExecution;	///< TestCompositionExecution needs to add a search path for its own private nodes.
	friend class TestCompilerDelegate;
	friend class TestVuoCompilerModule;
	friend class TestVuoCompilerBitcodeGenerator;
	friend class TestCompilingAndLinking;  ///< TestCompilingAndLinking calls `reset()`.
	friend class TestSubcompositions;  ///< TestSubcompositions calls `reset()`.
	friend class TestModuleLoading;  ///< TestModuleLoading calls `reset()`.
	friend class TestEventDropping;  ///< TestEventDropping calls `reset()`.
	friend class TestVuoIsfModuleCompiler;  ///< TestVuoIsfModuleCompiler calls `writeModuleToBitcode()`.
	friend class TestVuoCompilerCompatibility;  ///< TestVuoCompilerCompatibility calls `getProcessTarget()`.
	friend class TestNodes;  ///< TestNodes calls `getProcessTarget()`.

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
	void compileModule(string inputPath, string outputPath);
	void compileModule(string inputPath, string outputPath, const vector<string> &includeDirs);
	void linkCompositionToCreateExecutable(string inputPath, string outputPath, Optimization optimization, string rPath="", bool shouldAdHocCodeSign = true);
	void linkCompositionToCreateDynamicLibrary(string inputPath, string outputPath, Optimization optimization, bool shouldAdHocCodeSign = true);
	void linkCompositionToCreateDynamicLibraries(string compiledCompositionPath, string linkedCompositionPath, VuoRunningCompositionLibraries *runningCompositionLibraries);
	set<string> getDirectDependenciesForComposition(VuoCompilerComposition *composition);
	set<string> getDependenciesForComposition(VuoCompilerComposition *composition);
	set<string> getDylibDependencyPathsForComposition(VuoCompilerComposition *composition);
	VuoCompilerCompatibility getCompatibilityOfDependencies(const set<string> &dependencies);
	void prepareForFastBuild(void);
	string getTarget(void);
	string getArch(void);
	static void generateBuiltInModuleCaches(string vuoFrameworkPath, string target);
	static void deleteOldModuleCaches(void);
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
	VuoCompilerModule *getLibraryModule(const string &libraryModuleName);
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
	void setTelemetry(const string &telemetry);
	void setVerbose(bool isVerbose);
	void setShouldPotentiallyShowSplashWindow(bool potentiallyShow);
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
#include "pro/VuoCompiler_Pro.hh"
#endif
};

/**
 * An abstract class to be implemented by a client that will receive notifications from the compiler.
 * Use VuoCompiler::setDelegate() to connect a VuoCompilerDelegate to a VuoCompiler.
 *
 * @version200New
 */
class VuoCompilerDelegate
{
public:
	/**
	 * Initializes private data members.
	 */
	VuoCompilerDelegate(void);

	/**
	 * This delegate function is invoked each time the compiler loads or unloads modules or encounters issues when
	 * trying to do so.
	 *
	 * This function may be called as a result of a request for modules (e.g. VuoCompiler::getNodeClass()`) or
	 * a change to the files installed in the module search paths.
	 *
	 * This function may be called multiple times in rapid succession with different modules passed as arguments,
	 * including when the compiler is loading modules for the first time. It is always called sequentially
	 * as opposed to concurrently; the next call won't happen until the current call returns.
	 *
	 * Your implementation of this function is allowed to continue using the VuoCompilerModule and VuoCompilerIssues
	 * values that were passed as arguments after the function returns (for example, if the function schedules work
	 * to be done asynchronously).
	 *
	 * For each call to this function, you must make a corresponding call to @ref loadedModulesCompleted when
	 * you've finished using the argument values. When there are multiple calls to @ref loadedModules, it's
	 * assumed that you'll finish using argument values in the same order that you received them.
	 *
	 * @param modulesAdded Modules that were loaded (e.g. for new files in a module search path).
	 * @param modulesModified Modules that were reloaded (e.g. for modified files in a module search path).
	 *     The first pair item is the old version of the module; the second is the new version.
	 * @param modulesRemoved Modules that were unloaded (e.g. for files deleted from a module search path).
	 * @param issues Errors and warnings.
	 */
	virtual void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
							   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
							   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues) = 0;

	/**
	 * Signals that this delegate has finished using the argument values from @ref loadedModules.
	 * For each call to @ref loadedModules, you should make a corresponding call to this function
	 * within your @ref loadedModules implementation or sometime after that function returns.
	 *
	 * This function is necessary for proper memory management. If you don't call it, the invalidated modules
	 * in `modulesModified` (first item in pairs) and `modulesRemoved` (all items) and the VuoCompilerIssues
	 * instance will never be destroyed.
	 */
	void loadedModulesCompleted(void);

private:
	class LoadedModulesData
	{
	public:
		LoadedModulesData(const set< pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
						  const set<VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues);
		~LoadedModulesData(void);
		void retain(void);
		void release(void);

	private:
		int referenceCount;
		dispatch_queue_t referenceCountQueue;
		set< pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModified;
		set<VuoCompilerModule *> modulesRemoved;
		VuoCompilerIssues *issues;
	};

	void enqueueData(LoadedModulesData *data);
	LoadedModulesData * dequeueData(void);

	list<LoadedModulesData *> pendingData;
	dispatch_queue_t pendingDataQueue;

	friend class VuoCompiler;
};
