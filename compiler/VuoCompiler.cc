/**
 * @file
 * VuoCompiler implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerDiagnosticConsumer.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerGroup.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoComposition.hh"
#include "VuoException.hh"
#include "VuoGenericType.hh"
#include "VuoModuleCompiler.hh"
#include "VuoModuleCompilationQueue.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoPublishedPort.hh"
#include "VuoRunner.hh"
#include "VuoRunningCompositionLibraries.hh"
#include "VuoStringUtilities.hh"

#if VUO_PRO
#include "VuoPro.hh"
#endif

map<string, VuoFileUtilities::File *> VuoCompiler::Environment::moduleCacheFileForLocking;
dispatch_queue_t VuoCompiler::Environment::moduleCacheBuildingQueue = dispatch_queue_create("org.vuo.compiler.cache", NULL);
const string VuoCompiler::Environment::pidCacheDirPrefix = "pid-";
set<VuoCompiler *> VuoCompiler::allCompilers;
dispatch_queue_t VuoCompiler::environmentQueue = dispatch_queue_create("org.vuo.compiler.environment", NULL);
map<string, vector< vector<VuoCompiler::Environment *> > > VuoCompiler::sharedEnvironments;
map<string, map<string, vector<VuoCompiler::Environment *> > > VuoCompiler::environmentsForCompositionFamily;
dispatch_group_t VuoCompiler::moduleSourceCompilersExistGlobally = dispatch_group_create();
string VuoCompiler::vuoFrameworkInProgressPath;

dispatch_queue_t llvmQueue = NULL;  ///< Synchronizes access to LLVM's global context. Don't call environmentQueue from this queue.

llvm::LLVMContext *VuoCompiler::globalLLVMContext = nullptr;  ///< LLVM's global context.  @todo replace this with a per-VuoCompiler-instance context?

/**
 * Workaround for an LLVM bug.
 */
static void __attribute__((constructor)) VuoCompiler_init(void)
{
	// Increase the open-files limit (macOS defaults to 256).
	struct rlimit rl{OPEN_MAX, OPEN_MAX};
	getrlimit(RLIMIT_NOFILE, &rl);
	rl.rlim_cur = MIN(OPEN_MAX, rl.rlim_max);
	if (setrlimit(RLIMIT_NOFILE, &rl))
		VUserLog("Warning: Couldn't set open-files limit: %s", strerror(errno));


	llvmQueue = dispatch_queue_create("org.vuo.compiler.llvm", NULL);

	dispatch_sync(llvmQueue, ^{
					  // llvm::InitializeNativeTarget();
					  llvm::InitializeAllTargetMCs();
					  llvm::InitializeAllTargets();
					  // TargetRegistry::printRegisteredTargetsForVersion();

					  VuoCompiler::globalLLVMContext = new llvm::LLVMContext;

					  // Set up a diagnostic handler so that llvm::LLVMContext::diagnose doesn't exit
					  // when a call to llvm::Linker::linkInModule generates an error.
					  VuoCompiler::globalLLVMContext->setDiagnosticHandler([](const DiagnosticInfo &DI, void *context) {
						  string message;
						  raw_string_ostream stream(message);
						  DiagnosticPrinterRawOStream printer(stream);
						  DI.print(printer);
						  stream.flush();
						  if (! message.empty())
						  {
							  const char *severityName;
							  if (DI.getSeverity() == DS_Error)
								  severityName = "error";
							  else if (DI.getSeverity() == DS_Warning)
								  severityName = "warning";
							  else
								  severityName = "note";

							  VuoLog(VuoLog_moduleName, __FILE__, __LINE__, "llvmDiagnosticHandler", "%s: %s", severityName, message.c_str());
						  }
					  });

					  // If the Vuo compiler/linker...
					  //   1. Loads a node class that uses dispatch_object_t.
					  //   2. Generates code that uses dispatch_object_t.
					  //   3. Links the node class into a composition.
					  //   4. Generates more code that uses dispatch_object_t.
					  // ... then Step 4 ends up with the wrong llvm::Type for dispatch_object_t.
					  //
					  // A workaround is to generate some code that uses dispatch_object_t before doing Step 1.
					  //
					  // https://b33p.net/kosada/node/3845
					  // http://lists.cs.uiuc.edu/pipermail/llvmdev/2012-December/057075.html
					  Module module("", *VuoCompiler::globalLLVMContext);
					  VuoCompilerCodeGenUtilities::getDispatchObjectType(&module);

					  // Workaround for a possibly related error where the compiler ends up with the wrong
					  // llvm::Type for dispatch_semaphore_s.  https://b33p.net/kosada/node/10545
					  VuoCompilerCodeGenUtilities::getDispatchSemaphoreType(&module);

					  // Load the NodeContext and PortContext struct types preemptively to make sure that
					  // their fields get the right dispatch types. If these struct types were to be loaded
					  // first from a subcomposition module, they'd get the wrong dispatch types.
					  // https://b33p.net/kosada/node/11160
					  VuoCompilerCodeGenUtilities::getNodeContextType(&module);
					  VuoCompilerCodeGenUtilities::getPortContextType(&module);
				  });
}

/**
 * Constructs a ModuleInfo given the module/composition file's path relative to the search path.
 */
VuoCompiler::ModuleInfo::ModuleInfo(Environment *environment, const string &searchPath, const string &relativePath,
									bool isSourceFile, bool isSubcomposition)
{
	VuoFileUtilities::File *file = new VuoFileUtilities::File(searchPath, relativePath);
	initialize(environment, searchPath, file, isSourceFile, isSubcomposition);
}

/**
 * Constructs a ModuleInfo given a reference to the module/composition file.
 */
VuoCompiler::ModuleInfo::ModuleInfo(Environment *environment, const string &searchPath, VuoFileUtilities::File *file,
									bool isSourceFile, bool isSubcomposition)
{
	initialize(environment, searchPath, file, isSourceFile, isSubcomposition);
}

/**
 * Helper function for constructors.
 */
void VuoCompiler::ModuleInfo::initialize(Environment *environment, const string &searchPath, VuoFileUtilities::File *file,
										 bool isSourceFile, bool isSubcomposition)
{
	this->environment = environment;
	this->searchPath = searchPath;
	this->file = file;
	moduleKey = getModuleKeyForPath(file->getRelativePath());
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

VuoCompiler::ModuleInfo::~ModuleInfo(void)
{
#if VUO_PRO
	fini_Pro();
#endif

	delete file;

	if (loading)
		dispatch_release(loading);
}

VuoCompiler::Environment * VuoCompiler::ModuleInfo::getEnvironment(void) const
{
	return environment;
}

string VuoCompiler::ModuleInfo::getSearchPath(void) const
{
	return searchPath;
}

string VuoCompiler::ModuleInfo::getModuleKey(void) const
{
	return moduleKey;
}

VuoFileUtilities::File * VuoCompiler::ModuleInfo::getFile(void) const
{
	return file;
}

string VuoCompiler::ModuleInfo::getSourceCode(void) const
{
	return sourceCode;
}

void VuoCompiler::ModuleInfo::setSourceCode(const string &sourceCode)
{
	this->sourceCode = sourceCode;
}

void VuoCompiler::ModuleInfo::revertSourceCode(void)
{
	sourceCode = file->getContentsAsString();
}

bool VuoCompiler::ModuleInfo::isSourceCodeOverridden(void) const
{
	return sourceCodeOverridden;
}

void VuoCompiler::ModuleInfo::setSourceCodeOverridden(bool overridden)
{
	sourceCodeOverridden = overridden;
}

bool VuoCompiler::ModuleInfo::isNewerThan(ModuleInfo *other) const
{
	return lastModified > other->lastModified;
}

bool VuoCompiler::ModuleInfo::isNewerThan(unsigned long ageInSeconds) const
{
	return lastModified > ageInSeconds;
}

bool VuoCompiler::ModuleInfo::isOlderThan(unsigned long ageInSeconds) const
{
	return lastModified < ageInSeconds;
}

void VuoCompiler::ModuleInfo::setLastModifiedToNow(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	lastModified = t.tv_sec;
}

set<string> VuoCompiler::ModuleInfo::getContainedNodeClasses(void) const
{
	return containedNodeClasses;
}

int VuoCompiler::ModuleInfo::getLongestDownstreamPath(void) const
{
	return longestDownstreamPath;
}

void VuoCompiler::ModuleInfo::setLongestDownstreamPath(int pathLength)
{
	longestDownstreamPath = pathLength;
}

bool VuoCompiler::ModuleInfo::getAttempted(void) const
{
	return attempted;
}

void VuoCompiler::ModuleInfo::setAttempted(bool attempted)
{
	this->attempted = attempted;
}

dispatch_group_t VuoCompiler::ModuleInfo::getLoadingGroup(void) const
{
	return loading;
}

void VuoCompiler::ModuleInfo::dump() const
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

VuoCompiler::DependencyGraphVertex * VuoCompiler::DependencyGraphVertex::vertexForDependency(const string &dependency, VuoDirectedAcyclicGraph *graph)
{
	VuoDirectedAcyclicGraph::Vertex *v = graph->findVertex(dependency);
	if (v)
		return dynamic_cast<DependencyGraphVertex *>(v);

	DependencyGraphVertex *vv = new DependencyGraphVertex();
	vv->dependency = dependency;
	vv->environment = NULL;
	vv->compatible = true;
	return vv;
}

string VuoCompiler::DependencyGraphVertex::getDependency(void)
{
	return dependency;
}

VuoCompiler::Environment * VuoCompiler::DependencyGraphVertex::getEnvironment(void)
{
	return environment;
}

void VuoCompiler::DependencyGraphVertex::setEnvironment(Environment *environment)
{
	this->environment = environment;
}

string VuoCompiler::DependencyGraphVertex::key(void)
{
	return dependency;
}

/**
 * Constructs an iterator that will visit every item in @a allModuleInfos.
 *
 * If @a allModuleInfos contains both a compiled module and its override, the override will be returned.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator::ModuleInfoIterator(map<string, map<string, ModuleInfo *> > *allModuleInfos,
													const string &overriddenCompiledModuleCachePath)
{
	this->allModuleInfos = allModuleInfos;
	this->overriddenCompiledModuleCachePath = overriddenCompiledModuleCachePath;
	hasSearchModuleKeys = false;
	initialize();
}

/**
 * Constructs an iterator that will visit only the items in @a allModuleInfos that match one of @a searchModuleKeys.
 *
 * If @a allModuleInfos contains both a compiled module and its override, the override will be returned.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator::ModuleInfoIterator(map<string, map<string, ModuleInfo *> > *allModuleInfos,
													const string &overriddenCompiledModuleCachePath,
													const set<string> &searchModuleKeys)
{
	this->allModuleInfos = allModuleInfos;
	this->overriddenCompiledModuleCachePath = overriddenCompiledModuleCachePath;
	this->searchModuleKeys = searchModuleKeys;
	hasSearchModuleKeys = true;
	initialize();
}

/**
 * Helper for constructors.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::ModuleInfoIterator::initialize(void)
{
	currSearchPath = allModuleInfos->begin();
	hasCurrModuleKey = false;

	if (! overriddenCompiledModuleCachePath.empty())
	{
		auto i = allModuleInfos->find(overriddenCompiledModuleCachePath);
		if (i != allModuleInfos->end())
		{
			std::transform(i->second.begin(), i->second.end(),
						   std::inserter(overriddenModuleKeys, overriddenModuleKeys.begin()),
						   [](const pair<string, ModuleInfo *> &p) { return p.first; });
		}
	}
}

/**
 * Returns the next ModuleInfo to visit, or null if there are no more to visit.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfo * VuoCompiler::ModuleInfoIterator::next(void)
{
	for ( ; currSearchPath != allModuleInfos->end(); ++currSearchPath)
	{
		if (! hasCurrModuleKey)
		{
			currModuleKey = currSearchPath->second.begin();
			hasCurrModuleKey = true;
		}

		bool isOverriddenCompiledModuleCachePath = ! overriddenCompiledModuleCachePath.empty() &&
												   VuoFileUtilities::arePathsEqual(currSearchPath->first, overriddenCompiledModuleCachePath);

		for ( ; currModuleKey != currSearchPath->second.end(); ++currModuleKey)
		{
			if ((! hasSearchModuleKeys || searchModuleKeys.find(currModuleKey->first) != searchModuleKeys.end()) &&
					(isOverriddenCompiledModuleCachePath || overriddenModuleKeys.find(currModuleKey->first) == overriddenModuleKeys.end()))
			{
				ModuleInfo *moduleInfo = currModuleKey->second;
				++currModuleKey;
				return moduleInfo;
			}
		}

		hasCurrModuleKey = false;
	}

	return NULL;
}

/**
 * Creates an empty environment.
 */
VuoCompiler::Environment::Environment(string target, bool builtIn, bool generated)
	: target(target), builtIn(builtIn), generated(generated)
{
	compilersToNotifyQueue = dispatch_queue_create("org.vuo.compiler.notify", 0);
	moduleSearchPathContentsChangedQueue = dispatch_queue_create("org.vuo.compiler.watch", 0);
	lastModuleCacheRebuild = 0;
	isModuleCacheableDataDirty = false;
	isModuleCacheInitialized = false;
	isModuleCacheAvailable = false;
	dependencyGraph = new VuoDirectedAcyclicGraph();
	compositionDependencyGraph = new VuoDirectedAcyclicGraph();
	moduleCompilationQueue = new VuoModuleCompilationQueue();
}

/**
 * Destructor.
 */
VuoCompiler::Environment::~Environment(void)
{
	stopWatchingModuleSearchPaths();
	dispatch_sync(moduleSearchPathContentsChangedQueue, ^{});

	dispatch_release(moduleSearchPathContentsChangedQueue);
	dispatch_release(compilersToNotifyQueue);

	for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		destroyModule(i->second);

	for (map<string, VuoCompilerType *>::iterator i = types.begin(); i != types.end(); ++i)
		destroyModule(i->second);

	for (map<string, VuoCompilerModule *>::iterator i = libraryModules.begin(); i != libraryModules.end(); ++i)
		destroyModule(i->second);

	for (set<VuoCompilerGenericType *>::iterator i = genericTypes.begin(); i != genericTypes.end(); ++i)
	{
		VuoType *base = (*i)->getBase();
		delete *i;
		delete base;
	}

	for (map<string, VuoNodeSet *>::iterator i = nodeSetForName.begin(); i != nodeSetForName.end(); ++i)
		delete i->second;

	for (map<string, map<string, ModuleInfo *> >::iterator i = moduleFilesAtSearchPath.begin(); i != moduleFilesAtSearchPath.end(); ++i)
		for (map<string, ModuleInfo *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			delete j->second;

	for (map<string, map<string, ModuleInfo *> >::iterator i = sourceFilesAtSearchPath.begin(); i != sourceFilesAtSearchPath.end(); ++i)
		for (map<string, ModuleInfo *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			delete j->second;

	delete dependencyGraph;
	delete compositionDependencyGraph;
}

/**
 * Returns this environment's LLVM target triple.
 */
string VuoCompiler::Environment::getTarget()
{
	return target;
}

/**
 * Adds a compiler to notify when this Environment loads/unloads modules as a result of changes to the watched search paths.
 */
void VuoCompiler::Environment::addCompilerToNotify(VuoCompiler *compiler)
{
	dispatch_sync(compilersToNotifyQueue, ^{
					  compilersToNotify.insert(compiler);
				  });
}

/**
 * Removes a compiler to notify when this Environment loads/unloads modules as a result of changes to the watched search paths.
 */
void VuoCompiler::Environment::removeCompilerToNotify(VuoCompiler *compiler)
{
	dispatch_sync(compilersToNotifyQueue, ^{
					  compilersToNotify.erase(compiler);
				  });
}

/**
 * Returns the node classes loaded, plus specialized node classes generated by the compiler.
 *
 * @threadQueue{environmentQueue}
 */
map<string, VuoCompilerNodeClass *> VuoCompiler::Environment::getNodeClasses(void)
{
	return nodeClasses;
}

/**
 * Returns the node class with the given @a moduleKey, or null if no node class by that name has been loaded.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompilerNodeClass * VuoCompiler::Environment::getNodeClass(const string &moduleKey)
{
	map<string, VuoCompilerNodeClass *>::iterator nodeClassIter = nodeClasses.find(moduleKey);
	if (nodeClassIter != nodeClasses.end())
		return nodeClassIter->second;

	return NULL;
}

/**
 * Like @ref getNodeClass, but also removes the node class from this environment and transfers ownership
 * of it to the caller.
 */
VuoCompilerNodeClass * VuoCompiler::Environment::takeNodeClass(const string &moduleKey)
{
	VuoCompilerNodeClass *nodeClass = NULL;

	map<string, VuoCompilerNodeClass *>::iterator nodeClassIter = nodeClasses.find(moduleKey);
	if (nodeClassIter != nodeClasses.end())
	{
		nodeClass = nodeClassIter->second;

		nodeClasses.erase(nodeClassIter);
		removeFromDependencyGraph(nodeClass);
		modulesChanged();
	}

	return nodeClass;
}

/**
 * Returns the types loaded.
 *
 * @threadQueue{environmentQueue}
 */
map<string, VuoCompilerType *> VuoCompiler::Environment::getTypes(void)
{
	return types;
}

/**
 * Returns the type with the given @a moduleKey, or null if no type by that name has been loaded.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompilerType * VuoCompiler::Environment::getType(const string &moduleKey)
{
	map<string, VuoCompilerType *>::iterator typeIter = types.find(moduleKey);
	if (typeIter != types.end())
		return typeIter->second;

	return NULL;
}

/**
 * Returns the node sets loaded.
 *
 * @threadQueue{environmentQueue}
 */
map<string, VuoNodeSet *> VuoCompiler::Environment::getNodeSets(void)
{
	return nodeSetForName;
}

/**
 * Returns the library with the given `libraryModuleName`, or null if no library by that name has been loaded.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompilerModule *VuoCompiler::Environment::getLibraryModule(const string &libraryModuleName)
{
	map<string, VuoCompilerModule *>::iterator libraryIter = libraryModules.find(libraryModuleName);
	if (libraryIter != libraryModules.end())
		return libraryIter->second;

	return nullptr;
}

/**
 * Returns the library modules loaded.
 *
 * @threadQueue{environmentQueue}
 */
map<string, VuoCompilerModule *> VuoCompiler::Environment::getLibraryModules(void)
{
	return libraryModules;
}

/**
 * Returns the VuoCompilerNodeClass, VuoCompilerType, or VuoCompilerModule with the given name,
 * or null if no such module has been loaded.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompilerModule * VuoCompiler::Environment::findModule(const string &moduleKey)
{
	map<string, VuoCompilerNodeClass *>::iterator nodeClassIter = nodeClasses.find(moduleKey);
	if (nodeClassIter != nodeClasses.end())
		return nodeClassIter->second;

	map<string, VuoCompilerType *>::iterator typeIter = types.find(moduleKey);
	if (typeIter != types.end())
		return typeIter->second;

	map<string, VuoCompilerModule *>::iterator libraryIter = libraryModules.find(moduleKey);
	if (libraryIter != libraryModules.end())
		return libraryIter->second;

	return NULL;
}

/**
 * Returns the node set with the given name, or null if no such node set been loaded.
 *
 * @threadQueue{environmentQueue}
 */
VuoNodeSet * VuoCompiler::Environment::findNodeSet(const string &name)
{

	map<string, VuoNodeSet *>::iterator nodeSetIter = nodeSetForName.find(name);
	if (nodeSetIter != nodeSetForName.end())
		return nodeSetIter->second;

	return NULL;
}

/**
 * Adds a search path for node classes, types, and library modules,
 * and starts watching it for changes.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::addModuleSearchPath(const string &path, bool shouldWatch)
{
	moduleSearchPaths.push_back(path);

	updateModulesAtSearchPath(path);
	updateSourceFilesAtSearchPath(path);

	if (shouldWatch)
		startWatchingModuleSearchPath(path);
}

/**
 * Returns the search paths for node classes, types, and library modules.
 *
 * @threadQueue{environmentQueue}
 */
vector<string> VuoCompiler::Environment::getModuleSearchPaths(void)
{
	return moduleSearchPaths;
}

/**
 * Adds a search path for header/include files.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::addHeaderSearchPath(const string &path)
{
	headerSearchPaths.push_back(path);
}

/**
 * Returns the search paths for header/include files.
 *
 * @threadQueue{environmentQueue}
 */
vector<string> VuoCompiler::Environment::getHeaderSearchPaths(void)
{
	return headerSearchPaths;
}

/**
 * Adds a search path for libraries (other than Vuo library modules).
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::addLibrarySearchPath(const string &path)
{
	librarySearchPaths.push_back(path);
}

/**
 * Returns the search paths for libraries (other than Vuo library modules).
 *
 * @threadQueue{environmentQueue}
 */
vector<string> VuoCompiler::Environment::getLibrarySearchPaths(void)
{
	return librarySearchPaths;
}

/**
 * Adds a search path for frameworks.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::addFrameworkSearchPath(const string &path)
{
	frameworkSearchPaths.push_back(path);
}

/**
 * Returns the search paths for frameworks.
 *
 * @threadQueue{environmentQueue}
 */
vector<string> VuoCompiler::Environment::getFrameworkSearchPaths(void)
{
	return frameworkSearchPaths;
}

VuoDirectedAcyclicGraph * VuoCompiler::Environment::getDependencyGraph(void)
{
	return dependencyGraph;
}

VuoDirectedAcyclicGraph * VuoCompiler::Environment::getCompositionDependencyGraph(void)
{
	return compositionDependencyGraph;
}

/**
 * Sets the directory for compiled subcompositions and other modules compiled from source code.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::setModuleCachePath(const string &path, bool shouldAddModuleSearchPath)
{
	moduleCachePath = path;

	if (shouldAddModuleSearchPath)
	{
		addModuleSearchPath(getCompiledModuleCachePath(), false);
		addModuleSearchPath(getOverriddenCompiledModuleCachePath(), false);
	}
}

/**
 * Returns the directory for compiled subcompositions and other modules compiled from source code.
 *
 * @threadQueue{environmentQueue}
 */
string VuoCompiler::Environment::getCompiledModuleCachePath(void)
{
	return (moduleCachePath.empty() ? "" : moduleCachePath + "/Modules/" + VuoCompiler::getTargetArch(target));
}

/**
 * Returns the directory for modules compiled from unsaved subcompositions or source code.
 *
 * @threadQueue{environmentQueue}
 */
string VuoCompiler::Environment::getOverriddenCompiledModuleCachePath(void)
{
	if (moduleCachePath.empty())
		return "";

	ostringstream pid;
	pid << getpid();

	string dir, moduleCacheDirName, ext;
	VuoFileUtilities::splitPath(moduleCachePath, dir, moduleCacheDirName, ext);

	return (VuoFileUtilities::getCachePath() + "/" + pidCacheDirPrefix + pid.str() + "/" + moduleCacheDirName + "/Modules/" + VuoCompiler::getTargetArch(target));
}

/**
 * Adds a source file to this environment even though it's located outside of this environment's module search paths.
 *
 * The source file is not watched for changes.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::addExpatriateSourceFile(const string &sourcePath)
{
	expatriateSourceFiles.push_back(sourcePath);

	if (find(moduleSearchPaths.begin(), moduleSearchPaths.end(), "") == moduleSearchPaths.end())
		moduleSearchPaths.push_back("");

	auto iter = sourceFilesAtSearchPath.find("");
	if (iter != sourceFilesAtSearchPath.end())
		sourceFilesAtSearchPath.erase(iter);
}

/**
 * Removes the source file at @a sourcePath previously added by @ref addExpatriateSourceFile, if any.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::removeExpatriateSourceFile(const string &sourcePath)
{
	for (auto i = expatriateSourceFiles.begin(); i != expatriateSourceFiles.end(); ++i)
	{
		if (VuoFileUtilities::arePathsEqual(*i, sourcePath))
		{
			expatriateSourceFiles.erase(i);

			auto iter = sourceFilesAtSearchPath.find("");
			if (iter != sourceFilesAtSearchPath.end())
				sourceFilesAtSearchPath.erase(iter);

			break;
		}
	}
}

/**
 * Updates the list of all node classes, types, and library modules in the folder at @a path.
 *
 * The top level of the folder is searched for `.vuonode`, `.vuonode+`, `.bc`, and `.bc+` files.
 * A `.vuonode` file may be either a module or an archive containing modules.
 * In the latter case, the `.vuonode` archive's top level is searched.
 *
 * If @a path is the cached modules folder, any module files whose source files no longer
 * exist or whose source files are newer than the module file are deleted.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::updateModulesAtSearchPath(const string &path)
{
	if (moduleFilesAtSearchPath.find(path) != moduleFilesAtSearchPath.end())
		return;

	set<string> moduleExtensions;
	moduleExtensions.insert("vuonode");
	moduleExtensions.insert("vuonode+");
	moduleExtensions.insert("bc");
	moduleExtensions.insert("bc+");
	set<string> archiveExtensions;
	archiveExtensions.insert("vuonode");
	set<VuoFileUtilities::File *> moduleFiles = VuoFileUtilities::findFilesInDirectory(path, moduleExtensions, archiveExtensions);

	map<string, ModuleInfo *> fileForModuleKey;
	for (set<VuoFileUtilities::File *>::iterator i = moduleFiles.begin(); i != moduleFiles.end(); ++i)
	{
		VuoFileUtilities::File *moduleFile = *i;

		// Ignore macOS extended attribute storage (a.k.a. xattr, resource fork).
		if (VuoStringUtilities::beginsWith(moduleFile->basename(), "._"))
			continue;

		ModuleInfo *m = new ModuleInfo(this, path, moduleFile, false, false);
		fileForModuleKey[m->getModuleKey()] = m;
	}

	if (path == getCompiledModuleCachePath())
	{
		for (map<string, ModuleInfo *>::iterator i = fileForModuleKey.begin(); i != fileForModuleKey.end(); )
		{
			ModuleInfo *sourceInfo = listSourceFile(i->first);
			if (! sourceInfo || sourceInfo->isNewerThan(i->second))
			{
				ModuleInfo *m = i->second;
				VuoFileUtilities::deleteFile(m->getFile()->path());
				delete m;
				fileForModuleKey.erase(i++);
			}
			else
				++i;
		}
	}

	moduleFilesAtSearchPath[path] = fileForModuleKey;
}

/**
 * Adds a single module to the list of all node classes, types, and library modules in the folder at @a moduleSearchPath.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::updateModuleAtSearchPath(const string &moduleSearchPath, const string &moduleRelativePath)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(moduleRelativePath, dir, file, ext);

	set<string> moduleExtensions;
	moduleExtensions.insert(ext);
	set<string> archiveExtensions;
	archiveExtensions.insert("vuonode");

	set<VuoFileUtilities::File *> moduleFiles = VuoFileUtilities::findFilesInDirectory(moduleSearchPath, moduleExtensions, archiveExtensions);

	for (set<VuoFileUtilities::File *>::iterator i = moduleFiles.begin(); i != moduleFiles.end(); ++i)
	{
		VuoFileUtilities::File *moduleFile = *i;

		ModuleInfo *m = new ModuleInfo(this, moduleSearchPath, moduleFile, false, false);
		moduleFilesAtSearchPath[moduleSearchPath][m->getModuleKey()] = m;
	}
}

/**
 * Updates list of all compositions in the folder at @a path.
 *
 * The top level of the folder is searched for `.vuo` and `.fs` files.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::updateSourceFilesAtSearchPath(const string &path)
{
	map<string, map<string, ModuleInfo *> >::iterator sourceFilesIter = sourceFilesAtSearchPath.find(path);
	if (sourceFilesIter != sourceFilesAtSearchPath.end())
		return;

	set<VuoFileUtilities::File *> sourceFiles;
	if (! path.empty())
	{
		set<string> sourceExtensions;
		sourceExtensions.insert("vuo");
		sourceExtensions.insert("fs");
		auto cext = VuoFileUtilities::getCSourceExtensions();
		sourceExtensions.insert(cext.begin(), cext.end());
		sourceFiles = VuoFileUtilities::findFilesInDirectory(path, sourceExtensions, set<string>());
	}
	else
	{
		for (const string &sourcePath : expatriateSourceFiles)
		{
			string dir, file, ext;
			VuoFileUtilities::splitPath(sourcePath, dir, file, ext);
			VuoFileUtilities::File *sourceFile = new VuoFileUtilities::File(dir, file + "." + ext);
			sourceFiles.insert(sourceFile);
		}
	}

	map<string, ModuleInfo *> fileForModuleKey;
	for (set<VuoFileUtilities::File *>::iterator i = sourceFiles.begin(); i != sourceFiles.end(); ++i)
	{
		VuoFileUtilities::File *sourceFile = *i;

		string dir, moduleKey, ext;
		VuoFileUtilities::splitPath(sourceFile->getRelativePath(), dir, moduleKey, ext);
		bool isSubcomposition = (ext == "vuo");

		// Ignore missing expatriateSourceFiles — they might have been deleted in the meantime.
		if (path.empty() && !sourceFile->exists())
			continue;

		ModuleInfo *m = new ModuleInfo(this, path, sourceFile, true, isSubcomposition);
		if (fileForModuleKey.find(m->getModuleKey()) != fileForModuleKey.end())
			VUserLog("Warning: Conflicting source files for module %s are installed at %s", m->getModuleKey().c_str(), path.c_str());
		fileForModuleKey[m->getModuleKey()] = m;

		if (isSubcomposition)
		{
			DependencyGraphVertex *compositionVertex = DependencyGraphVertex::vertexForDependency(moduleKey, compositionDependencyGraph);
			compositionDependencyGraph->addVertex(compositionVertex);

			compositionVertex->setEnvironment(this);

			set<string> dependencies = m->getContainedNodeClasses();
			for (set<string>::iterator j = dependencies.begin(); j != dependencies.end(); ++j)
			{
				DependencyGraphVertex *dependencyVertex = DependencyGraphVertex::vertexForDependency(*j, compositionDependencyGraph);
				compositionDependencyGraph->addEdge(compositionVertex, dependencyVertex);
			}
		}
	}

	sourceFilesAtSearchPath[path] = fileForModuleKey;
}

/**
 * Returns info for the module if it's found in one of the @ref moduleSearchPaths, otherwise null.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfo * VuoCompiler::Environment::listModule(const string &moduleKey)
{
	for (const auto &moduleFiles : moduleFilesAtSearchPath)
	{
		map<string, ModuleInfo *>::const_iterator foundIter = moduleFiles.second.find(moduleKey);
		if (foundIter != moduleFiles.second.end())
			return foundIter->second;
	}

	return NULL;
}

/**
 * Returns info for each of the modules specified by @a moduleKeys that is found in one of the @ref moduleSearchPaths.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator VuoCompiler::Environment::listModules(const set<string> &moduleKeys)
{
	for (const string &path : moduleSearchPaths)
		updateModulesAtSearchPath(path);

	return ModuleInfoIterator(&moduleFilesAtSearchPath, getOverriddenCompiledModuleCachePath(), moduleKeys);
}

/**
 * Returns info for all of the modules found in the @ref moduleSearchPaths.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator VuoCompiler::Environment::listAllModules(void)
{
	for (const string &path : moduleSearchPaths)
		updateModulesAtSearchPath(path);

	return ModuleInfoIterator(&moduleFilesAtSearchPath, getOverriddenCompiledModuleCachePath());
}

/**
 * Returns info for the module source file if it's found in one of the @ref moduleSearchPaths, otherwise null.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfo * VuoCompiler::Environment::listSourceFile(const string &moduleKey)
{
	for (const auto &sourceFiles : sourceFilesAtSearchPath)
	{
		map<string, ModuleInfo *>::const_iterator foundIter = sourceFiles.second.find(moduleKey);
		if (foundIter != sourceFiles.second.end())
			return foundIter->second;
	}

	return NULL;
}

/**
 * Returns info for each of the module source files specified by @a moduleKeys that is found in one of the @ref moduleSearchPaths.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator VuoCompiler::Environment::listSourceFiles(const set<string> &moduleKeys)
{
	for (const string &path : moduleSearchPaths)
		updateSourceFilesAtSearchPath(path);

	return ModuleInfoIterator(&sourceFilesAtSearchPath, "", moduleKeys);
}

/**
 * Returns info for all of the module source files found in the @ref moduleSearchPaths.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompiler::ModuleInfoIterator VuoCompiler::Environment::listAllSourceFiles(void)
{
	for (const string &path : moduleSearchPaths)
		updateSourceFilesAtSearchPath(path);

	return ModuleInfoIterator(&sourceFilesAtSearchPath, "");
}

/**
 * Returns the search paths for built-in node classes, types, and library modules.
 */
vector<string> VuoCompiler::Environment::getBuiltInModuleSearchPaths(void)
{
	vector<string> builtInModuleSearchPaths;

	string vuoFrameworkPath = getVuoFrameworkPath();
	if (! vuoFrameworkPath.empty())
	{
		builtInModuleSearchPaths.push_back(vuoFrameworkPath + "/Modules");
	}
	else
	{
		builtInModuleSearchPaths.push_back(VUO_BUILD_DIR "/library");
		builtInModuleSearchPaths.push_back(VUO_BUILD_DIR "/node");
		builtInModuleSearchPaths.push_back(VUO_BUILD_DIR "/type");
		builtInModuleSearchPaths.push_back(VUO_BUILD_DIR "/type/list");
	}

	return builtInModuleSearchPaths;
}

/**
 * Returns the search paths for built-in header files.
 */
vector<string> VuoCompiler::Environment::getBuiltInHeaderSearchPaths(void)
{
	vector<string> builtInHeaderSearchPaths;

	string vuoFrameworkPath = getVuoFrameworkPath();
	if (! vuoFrameworkPath.empty())
	{
		builtInHeaderSearchPaths.push_back(vuoFrameworkPath + "/Frameworks/llvm.framework/Versions/A/Headers/lib/c++/v1");
		builtInHeaderSearchPaths.push_back(vuoFrameworkPath + "/Headers");
		builtInHeaderSearchPaths.push_back(vuoFrameworkPath + "/Headers/macos");          // system headers installed by Xcode Command Line Tools
		builtInHeaderSearchPaths.push_back(vuoFrameworkPath + "/Headers/macos/pthread");  //
	}
	else
	{
		builtInHeaderSearchPaths.push_back(LLVM_ROOT "/include/c++/v1");
		builtInHeaderSearchPaths.push_back(VUO_SOURCE_DIR "/library");
		builtInHeaderSearchPaths.push_back(VUO_SOURCE_DIR "/node");
		builtInHeaderSearchPaths.push_back(VUO_SOURCE_DIR "/type");
		builtInHeaderSearchPaths.push_back(VUO_SOURCE_DIR "/type/list");
		builtInHeaderSearchPaths.push_back(VUO_SOURCE_DIR "/runtime");
		builtInHeaderSearchPaths.push_back(JSONC_ROOT "/include");
	}

	return builtInHeaderSearchPaths;
}

/**
 * Returns the search paths for built-in libraries (other than library modules).
 */
vector<string> VuoCompiler::Environment::getBuiltInLibrarySearchPaths(void)
{
	vector<string> builtInLibrarySearchPaths;

	string vuoFrameworkPath = getVuoFrameworkPath();
	if (! vuoFrameworkPath.empty())
	{
		builtInLibrarySearchPaths.push_back(vuoFrameworkPath + "/Modules");

		// Ensure we (statically) link to our OpenSSL build when generating Vuo.framework's built-in module cache.
		builtInLibrarySearchPaths.push_back(OPENSSL_ROOT "/lib");
	}
	else
	{
		builtInLibrarySearchPaths.push_back(VUO_BUILD_DIR "/library");
		builtInLibrarySearchPaths.push_back(VUO_BUILD_DIR "/node");
		builtInLibrarySearchPaths.push_back(VUO_BUILD_DIR "/type");
		builtInLibrarySearchPaths.push_back(VUO_BUILD_DIR "/type/list");
		builtInLibrarySearchPaths.push_back(VUO_BUILD_DIR "/runtime");

		vector<string> conanLibDirs = VuoStringUtilities::split(CONAN_LIBRARY_PATHS, ';');
		builtInLibrarySearchPaths.insert(builtInLibrarySearchPaths.end(), conanLibDirs.begin(), conanLibDirs.end());
	}

	return builtInLibrarySearchPaths;
}

/**
 * Returns the search paths for built-in frameworks.
 */
vector<string> VuoCompiler::Environment::getBuiltInFrameworkSearchPaths(void)
{
	vector<string> builtInFrameworkSearchPaths;

	string vuoFrameworkPath = getVuoFrameworkPath();
	if (! vuoFrameworkPath.empty())
	{
		builtInFrameworkSearchPaths.push_back(vuoFrameworkPath + "/Modules/");
		builtInFrameworkSearchPaths.push_back(vuoFrameworkPath + "/Frameworks/");
	}

	return builtInFrameworkSearchPaths;
}

/**
 * Starts surveilling the given directory for changes to the files within it, and
 * when they do change, loads/unloads the modules for those files.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::startWatchingModuleSearchPath(const string &moduleSearchPath)
{
	VuoFileWatcher *watcher = new VuoFileWatcher(this, moduleSearchPath);
	moduleSearchPathWatchers.insert(watcher);
}

/**
 * Stops watching all watched directories.
 *
 * @threadQueue(environmentQueue}
 */
void VuoCompiler::Environment::stopWatchingModuleSearchPaths(void)
{
	for (set<VuoFileWatcher *>::iterator i = moduleSearchPathWatchers.begin(); i != moduleSearchPathWatchers.end(); ++i)
		delete *i;

	moduleSearchPathWatchers.clear();
}

/**
 * VuoFileWatcher delegate function, called when one or more files in @a moduleSearchPath
 * have been added, modified, or removed.
 */
void VuoCompiler::Environment::fileChanged(const string &moduleSearchPath)
{
	dispatch_sync(moduleSearchPathContentsChangedQueue, ^{
					  moduleSearchPathContentsChanged(moduleSearchPath);
				  });
}

/**
 * Callback for when files have been added, modified, or removed from one of the module search paths.
 * Updates the records of known module and composition files, unloads and loads compiled modules,
 * schedules module source files to be compiled, and notifies each of @ref compilersToNotify.
 *
 * @threadQueue{moduleSearchPathContentsChangedQueue}
 */
void VuoCompiler::Environment::moduleSearchPathContentsChanged(const string &moduleSearchPath)
{
	//VLog("                 E=%p -- %s", this, moduleSearchPath.c_str());
	VuoCompiler *compilerForLoading = new VuoCompiler(moduleSearchPath + "/unused");

	__block set<string> modulesAdded;
	__block set<string> modulesModifed;
	__block set<string> modulesRemoved;
	__block set<string> compositionsAdded;
	__block set<string> compositionsModifed;
	__block set<string> compositionsRemoved;

	dispatch_sync(environmentQueue, ^{

					  // Remove the old file records from the environment.

					  map<string, ModuleInfo *> oldModules;
					  map<string, ModuleInfo *> oldCompositions;

					  map<string, map<string, ModuleInfo *> >::iterator mf = moduleFilesAtSearchPath.find(moduleSearchPath);
					  if (mf != moduleFilesAtSearchPath.end())
					  {
						  for (map<string, ModuleInfo *>::iterator j = mf->second.begin(); j != mf->second.end(); ++j)
						  {
							  oldModules[j->first] = j->second;
						  }

						  moduleFilesAtSearchPath.erase(mf);
					  }

					  map<string, map<string, ModuleInfo *> >::iterator cf = sourceFilesAtSearchPath.find(moduleSearchPath);
					  if (cf != sourceFilesAtSearchPath.end())
					  {
						  for (map<string, ModuleInfo *>::iterator j = cf->second.begin(); j != cf->second.end(); ++j)
						  {
							  oldCompositions[j->first] = j->second;

							  VuoDirectedAcyclicGraph::Vertex *vertex = compositionDependencyGraph->findVertex(j->first);
							  compositionDependencyGraph->removeVertex(vertex);
						  }

						  sourceFilesAtSearchPath.erase(cf);
					  }

					  // Rebuild the file records based on the directory contents.

					  updateModulesAtSearchPath(moduleSearchPath);
					  updateSourceFilesAtSearchPath(moduleSearchPath);

					  // Remove any overrides for source files that have been modified or removed.

					  auto omf = moduleFilesAtSearchPath.find(getOverriddenCompiledModuleCachePath());
					  if (omf != moduleFilesAtSearchPath.end())
					  {
						  for (auto oldComposition : oldCompositions)
						  {
							  for (auto i = omf->second.begin(); i != omf->second.end(); ++i)
							  {
								  if (i->first == oldComposition.first)
								  {
									  VuoFileUtilities::deleteFile(i->second->getFile()->path());
									  omf->second.erase(i);
									  break;
								  }
							  }
						  }
					  }

					  // Compare the old and new file records to see what has changed.

					  mf = moduleFilesAtSearchPath.find(moduleSearchPath);
					  if (mf != moduleFilesAtSearchPath.end())
					  {
						  for (map<string, ModuleInfo *>::iterator n = mf->second.begin(); n != mf->second.end(); ++n)
						  {
							  string moduleKey = n->first;

							  map<string, ModuleInfo *>::iterator o = oldModules.find(moduleKey);
							  if (o != oldModules.end())
							  {
								  if (n->second->isNewerThan(o->second))
								  {
									  modulesModifed.insert(moduleKey);
								  }
								  else
								  {
									  n->second->setAttempted(o->second->getAttempted());
								  }

								  delete o->second;
								  oldModules.erase(o);
							  }
							  else
							  {
								  modulesAdded.insert(moduleKey);
							  }
						  }
					  }

					  cf = sourceFilesAtSearchPath.find(moduleSearchPath);
					  if (cf != sourceFilesAtSearchPath.end())
					  {
						  for (map<string, ModuleInfo *>::iterator n = cf->second.begin(); n != cf->second.end(); ++n)
						  {
							  string moduleKey = n->first;

							  map<string, ModuleInfo *>::iterator o = oldCompositions.find(moduleKey);
							  if (o != oldCompositions.end())
							  {
								  if (n->second->isNewerThan(o->second))
								  {
									  compositionsModifed.insert(moduleKey);
								  }
								  else
								  {
									  n->second->setAttempted(o->second->getAttempted());
									  n->second->setSourceCode(o->second->getSourceCode());
								  }

								  delete o->second;
								  oldCompositions.erase(o);
							  }
							  else
							  {
								  compositionsAdded.insert(moduleKey);
							  }
						  }
					  }

					  for (map<string, ModuleInfo *>::iterator o = oldModules.begin(); o != oldModules.end(); ++o)
					  {
						  delete o->second;
						  modulesRemoved.insert(o->first);
					  }

					  for (map<string, ModuleInfo *>::iterator o = oldCompositions.begin(); o != oldCompositions.end(); ++o)
					  {
						  delete o->second;
						  compositionsRemoved.insert(o->first);
					  }

					  compilerForLoading->loadModulesAndSources(modulesAdded, modulesModifed, modulesRemoved,
																compositionsAdded, compositionsModifed, compositionsRemoved,
																false, false, this, nullptr, nullptr, "");
				  });

	delete compilerForLoading;
}

/**
 * Callback for when a single file has been added, modified, or removed from one of the module search paths.
 *
 * Unlike @ref VuoCompiler::Environment::moduleSearchPathContentsChanged, this function only updates the
 * record for the one file. It does not check for other changed files in the directory.
 */
void VuoCompiler::Environment::moduleFileChanged(const string &modulePath, const string &moduleSourceCode,
												 std::function<void(void)> moduleLoadedCallback,
												 VuoCompiler *compiler, VuoCompilerIssues *issues)
{
	//VLog("                 E=%p -- %s", this, modulePath.c_str());
	dispatch_sync(environmentQueue, ^{

					  string moduleDir, moduleKey, ext;
					  VuoFileUtilities::splitPath(modulePath, moduleDir, moduleKey, ext);
					  VuoFileUtilities::canonicalizePath(moduleDir);

					  // Remove the old file record from the environment.

					  bool foundOldModule = false;
					  auto moduleSearchPathIter = moduleFilesAtSearchPath.find(moduleDir);
					  if (moduleSearchPathIter != moduleFilesAtSearchPath.end())
					  {
						  auto moduleIter = moduleSearchPathIter->second.find(moduleKey);
						  if (moduleIter != moduleSearchPathIter->second.end())
						  {
							  delete moduleIter->second;
							  moduleSearchPathIter->second.erase(moduleIter);
							  foundOldModule = true;
						  }
					  }

					  // Update the file record for the module by re-checking the file.

					  updateModuleAtSearchPath(moduleDir, moduleKey + "." + ext);

					  // Compare the old and new file records to see how the module has changed.

					  bool foundNewModule = false;
					  moduleSearchPathIter = moduleFilesAtSearchPath.find(moduleDir);
					  if (moduleSearchPathIter != moduleFilesAtSearchPath.end())
					  {
						  auto moduleIter = moduleSearchPathIter->second.find(moduleKey);
						  if (moduleIter != moduleSearchPathIter->second.end())
						  {
							  foundNewModule = true;
						  }
					  }

					  set<string> modulesAdded;
					  set<string> modulesModified;
					  set<string> modulesRemoved;

					  if ((foundOldModule || VuoFileUtilities::arePathsEqual(moduleDir, getOverriddenCompiledModuleCachePath())) && foundNewModule)
					  {
						  modulesModified.insert(moduleKey);
					  }
					  else if (! foundOldModule && foundNewModule)
					  {
						  modulesAdded.insert(moduleKey);
					  }
					  else if (foundOldModule && ! foundNewModule)
					  {
						  modulesRemoved.insert(moduleKey);
					  }

					  compiler->loadModulesAndSources(modulesAdded, modulesModified, modulesRemoved,
													  set<string>(), set<string>(), set<string>(),
													  false, false, this, issues, moduleLoadedCallback, moduleSourceCode);
				  });

}

/**
 * Notifies each of @ref compilersToNotify of modules loaded/unloaded and issues encountered.
 */
void VuoCompiler::Environment::notifyCompilers(const set<VuoCompilerModule *> &modulesAdded,
											   const set< pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
											   const set<VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues,
											   bool oldModulesInvalidated)
{
	if (! (modulesAdded.empty() && modulesModified.empty() && modulesRemoved.empty() && issues->isEmpty()) )
	{
		set< pair<VuoCompilerModule *, VuoCompilerModule *> > invalidatedModulesModified;
		set<VuoCompilerModule *> invalidatedModulesRemoved;
		if (oldModulesInvalidated)
		{
			invalidatedModulesModified = modulesModified;
			invalidatedModulesRemoved = modulesRemoved;
		}

		VuoCompilerDelegate::LoadedModulesData *delegateData = new VuoCompilerDelegate::LoadedModulesData(invalidatedModulesModified, invalidatedModulesRemoved, issues);

		map<string, VuoCompilerModule *> modulesAddedMap;
		map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModifiedMap;
		map<string, VuoCompilerModule *> modulesRemovedMap;
		for (VuoCompilerModule *m : modulesAdded)
			modulesAddedMap[m->getPseudoBase()->getModuleKey()] = m;
		for (pair<VuoCompilerModule *, VuoCompilerModule *> m : modulesModified)
			modulesModifiedMap[m.first->getPseudoBase()->getModuleKey()] = m;
		for (VuoCompilerModule *m : modulesRemoved)
			modulesRemovedMap[m->getPseudoBase()->getModuleKey()] = m;

		dispatch_sync(compilersToNotifyQueue, ^{
						  for (set<VuoCompiler *>::iterator i = compilersToNotify.begin(); i != compilersToNotify.end(); ++i) {
							  delegateData->retain();
						  }
						  for (set<VuoCompiler *>::iterator i = compilersToNotify.begin(); i != compilersToNotify.end(); ++i) {
							  (*i)->loadedModules(modulesAddedMap, modulesModifiedMap, modulesRemovedMap, issues, delegateData, this);
						  }
					  });
	}
	else
	{
		delete issues;
	}
}

/**
 * Loads each of @a moduleKeys that is an already-compiled module file, belongs to this environment,
 * and is not already loaded.
 *
 * Returns the modules that were actually loaded.
 *
 * Issues are logged to the console.
 *
 * @threadQueue{environmentQueue}
 */
set<VuoCompilerModule *> VuoCompiler::Environment::loadCompiledModules(const set<string> &moduleKeys, const map<string, string> &sourceCodeForModule)
{
	ModuleInfoIterator modulesToLoadIter = listModules(moduleKeys);
	set<VuoCompilerModule *> modulesLoaded;

	ModuleInfo *moduleInfo;
	while ((moduleInfo = modulesToLoadIter.next()))
	{
		string moduleKey = moduleInfo->getModuleKey();

		// Skip the module if its file is not in this environment, if the module has already been loaded,
		// or if the compiler previously tried to load the module and it failed.
		if (moduleInfo->getEnvironment() != this || findModule(moduleKey) || moduleInfo->getAttempted())
			continue;

		moduleInfo->setAttempted(true);

		// Actually load the module.
		VuoCompilerModule *module = loadModule(moduleInfo);

		if (module)
		{
			modulesLoaded.insert(module);
			addToDependencyGraph(module);
			modulesChanged();

			// For a compiled subcomposition or other source file, store its source code in the VuoCompilerNodeClass.
			string searchPath = moduleInfo->getSearchPath();
			if (VuoFileUtilities::arePathsEqual(searchPath, getCompiledModuleCachePath()) ||
					VuoFileUtilities::arePathsEqual(searchPath, getOverriddenCompiledModuleCachePath()))
			{
				VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
				if (nodeClass)
				{
					ModuleInfo *sourceInfo = listSourceFile(moduleKey);
					if (sourceInfo)
						nodeClass->setSourcePath( sourceInfo->getFile()->path() );

					auto sourceCodeIter = sourceCodeForModule.find(moduleKey);
					if (sourceCodeIter != sourceCodeForModule.end())
						nodeClass->setSourceCode( sourceCodeIter->second );
				}
			}
		}
	}

	return modulesLoaded;
}

/**
 * Starts generating and loading each of @a moduleKeys that is a specialization of a generic node class
 * and hasn't already been attempted to be loaded.
 *
 * Loading is scheduled asynchronously. The caller can wait on the returned dispatch groups,
 * and is responsible for releasing them.
 *
 * Assumes that the dependencies of each specialized node class have already been loaded.
 *
 * @threadQueue{environmentQueue}
 */
set<dispatch_group_t> VuoCompiler::Environment::loadSpecializedModules(const set<string> &moduleKeys,
																	   VuoCompiler *compiler, dispatch_queue_t llvmQueue)
{
	set<dispatch_group_t > loadingGroups;

	for (string moduleKey : moduleKeys)
	{
		// Skip if it's not a node class.

		if (moduleKey.find(".") == string::npos ||
				VuoStringUtilities::endsWith(moduleKey, ".framework") ||
				VuoStringUtilities::endsWith(moduleKey, ".dylib"))
			continue;

		// Skip the module if it's already been loaded.

		if (findModule(moduleKey))
			continue;

		// Skip the module if it's in the process of being loaded, but have the caller wait for it to finish loading.

		map<string, dispatch_group_t>::iterator iter = specializedModulesLoading.find(moduleKey);
		if (iter != specializedModulesLoading.end())
		{
			loadingGroups.insert(iter->second);
			dispatch_retain(iter->second);
			continue;
		}

		dispatch_group_t loadingGroup = dispatch_group_create();
		specializedModulesLoading[moduleKey] = loadingGroup;
		loadingGroups.insert(loadingGroup);

		// Generate the module.
		// This is done asynchronously since VuoCompilerSpecializedNodeClass::newNodeClass() needs to use environmentQueue
		// to compile the generated C code.

		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		dispatch_group_async(loadingGroup, queue, ^{
			VuoNodeClass *baseNodeClass = nullptr;
			try
			{
				baseNodeClass = VuoCompilerSpecializedNodeClass::newNodeClass(moduleKey, compiler, llvmQueue);
			}
			catch (VuoCompilerException &e)
			{
				VUserLog("Error: %s", e.what());
			}

			if (baseNodeClass)
			{
				VuoCompilerSpecializedNodeClass *specNodeClass = static_cast<VuoCompilerSpecializedNodeClass *>(baseNodeClass->getCompiler());
				compiler->loadNodeClassGeneratedAtRuntime(specNodeClass, this);

				set<string> dependencies = specNodeClass->getDependencies();
				if (!dependencies.empty())
					compiler->loadModulesIfNeeded(dependencies);
			}

			dispatch_sync(environmentQueue, ^{
				specializedModulesLoading.erase(moduleKey);
			});
		});
	}

	return loadingGroups;
}

/**
 * Starts compiling each of @a moduleKeys that is a composition or ISF file, belongs to this environment,
 * doesn't have a compiled file or has a compiled file older than the source file, and
 * hasn't already been attempted to be compiled.
 *
 * Compiling is scheduled asynchronously. The caller can wait on the returned dispatch groups.
 *
 * If a subcomposition to be compiled has other subcompositions as dependencies, the dependencies
 * are compiled first.
 *
 * @threadQueue{environmentQueue}
 */
set<dispatch_group_t> VuoCompiler::Environment::compileModulesFromSourceCode(const set<string> &moduleKeys, dispatch_group_t moduleSourceCompilersExist,
																			 bool shouldRecompileIfUnchanged)
{
	ModuleInfoIterator modulesToLoadIter = listSourceFiles(moduleKeys);

	int environmentIndex = sharedEnvironments[target].size();
	for (int i = 0; i < sharedEnvironments[target].size(); ++i)
		if (this == sharedEnvironments[target].at(i).at(0))
			environmentIndex = i;

	set<dispatch_group_t> sourcesLoading;
	int sourcesEnqueued = 0;
	ModuleInfo *sourceInfo;
	while ((sourceInfo = modulesToLoadIter.next()))
	{
		string moduleKey = sourceInfo->getModuleKey();

		dispatch_group_t loadingGroup = sourceInfo->getLoadingGroup();
		sourcesLoading.insert(loadingGroup);

		// Skip compiling if the source file has already been scheduled for compilation.
		// Either its compilation is in progress or it failed to compile.

		if (sourceInfo->getAttempted())
			continue;

		// Skip compiling if the compiled module is up-to-date.

		string sourceCode = sourceInfo->getSourceCode();

		if (! shouldRecompileIfUnchanged)
		{
			VuoCompilerNodeClass *existingNodeClass = getNodeClass(moduleKey);
			if (existingNodeClass && existingNodeClass->getSourceCode() == sourceCode)
				continue;
		}

		// Enqueue the source file to be compiled.

		sourceInfo->setAttempted(true);

		dispatch_group_enter(loadingGroup);
		dispatch_group_enter(moduleSourceCompilersExist);
		dispatch_group_enter(moduleSourceCompilersExistGlobally);

		VuoModuleCompilationQueue::Item *queueItem = new VuoModuleCompilationQueue::Item();
		queueItem->moduleKey = moduleKey;
		queueItem->sourcePath = sourceInfo->getFile()->path();
		queueItem->sourceCode = sourceCode;
		queueItem->sourceFile = sourceInfo->getFile();
		queueItem->cachedModulesPath = sourceInfo->isSourceCodeOverridden() ? getOverriddenCompiledModuleCachePath() : getCompiledModuleCachePath();
		queueItem->compiledModulePath = queueItem->cachedModulesPath + "/" + moduleKey + ".vuonode";
		queueItem->loadingGroup = loadingGroup;
		queueItem->priority = { environmentIndex, sourceInfo->getLongestDownstreamPath() };
		moduleCompilationQueue->enqueue(queueItem);
		++sourcesEnqueued;
	}

	// Compile each enqueued source file. This is done asynchronously for several reasons:
	//   - To avoid environmentQueue calling compiler code calling environmentQueue.
	//   - To compile dependencies that were enqueued later while a compilation that was enqueued earlier waits.
	//   - To be more efficient when compiling multiple source files.

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
		for (int i = 0; i < sourcesEnqueued; ++i)
		{
			__block VuoModuleCompilationQueue::Item *queueItem = moduleCompilationQueue->next();
			VUserLog("Compiling %s", queueItem->moduleKey.c_str());

			dispatch_async(queue, ^{
				try
				{
					VuoFileUtilities::makeDir(queueItem->cachedModulesPath);
				}
				catch (VuoException &e)
				{
					VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling subcomposition", "", "Couldn't create cached modules folder", e.what());
					VuoCompilerIssues *issues = new VuoCompilerIssues(issue);
					notifyCompilers(set<VuoCompilerModule *>(), set< pair<VuoCompilerModule *, VuoCompilerModule *> >(), set<VuoCompilerModule *>(), issues, false);
				}

				VuoCompiler *compiler = new VuoCompiler(queueItem->sourcePath, target);
				compiler->setLoadAllModules(false);

				VuoModuleCompilationQueue::Item *queueItemForCallback = queueItem;
				auto moduleLoadedCallback = [=](){
					dispatch_group_leave(queueItemForCallback->loadingGroup);
					moduleCompilationQueue->completed(queueItemForCallback);
				};

				string ext = queueItem->sourceFile->extension();
				if (VuoFileUtilities::isIsfSourceExtension(ext))
				{
					VuoModuleCompiler *moduleCompiler = NULL;
					try
					{
						moduleCompiler = VuoModuleCompiler::newModuleCompiler("isf", queueItem->moduleKey, queueItem->sourceFile);
					}
					catch (VuoException &e)
					{
						VuoCompilerIssues *issues = new VuoCompilerIssues(VuoCompilerIssue(VuoCompilerIssue::Error, "compiling ISF module", queueItem->sourcePath, "", e.what()));
						notifyCompilers(set<VuoCompilerModule *>(), set< pair<VuoCompilerModule *, VuoCompilerModule *> >(), set<VuoCompilerModule *>(), issues, false);
					}

					if (moduleCompiler)
					{
						moduleCompiler->overrideSourceCode(queueItem->sourceCode, queueItem->sourceFile);

						auto getType = [&compiler] (const string &moduleKey) { return compiler->getType(moduleKey); };
						VuoCompilerIssues *issues = new VuoCompilerIssues;
						Module *module = moduleCompiler->compile(getType, llvmQueue, issues);

						if (module)
						{
							dispatch_sync(llvmQueue, ^{
								setTargetForModule(module, target);
								writeModuleToBitcode(module, queueItem->compiledModulePath);
							});
						}
						else
						{
							issues->setFilePathIfEmpty(queueItem->sourcePath);
						}

						moduleFileChanged(queueItem->compiledModulePath, queueItem->sourceCode, moduleLoadedCallback, compiler, issues);
					}
					else
						moduleLoadedCallback();
				}
				else if (VuoFileUtilities::isCSourceExtension(ext) && ! queueItem->cachedModulesPath.empty())
				{
					VuoCompilerIssues *issues = new VuoCompilerIssues;

					try
					{
						compiler->compileModule(queueItem->sourcePath, queueItem->compiledModulePath, vector<string>());
					}
					catch (VuoCompilerException &e)
					{
						if (issues != e.getIssues())
							issues->append(e.getIssues());

						issues->setFilePathIfEmpty(queueItem->sourcePath);
					}

					moduleFileChanged(queueItem->compiledModulePath, queueItem->sourceCode, moduleLoadedCallback, compiler, issues);
				}
				else
				{
					VuoCompilerIssues *issues = new VuoCompilerIssues;

					try
					{
						VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(queueItem->sourceCode, compiler);

						Module *module = compiler->compileCompositionToModule(composition, queueItem->moduleKey, false, issues);
						if (! module)
							throw VuoCompilerException(issues, false);

						dispatch_sync(llvmQueue, ^{
							setTargetForModule(module, target);
							writeModuleToBitcode(module, queueItem->compiledModulePath);
						});

						VuoComposition *baseComposition = composition->getBase();
						delete composition;
						delete baseComposition;
					}
					catch (VuoCompilerException &e)
					{
						if (issues != e.getIssues())
							issues->append(e.getIssues());

						issues->setFilePathIfEmpty(queueItem->sourcePath);
					}

					moduleFileChanged(queueItem->compiledModulePath, queueItem->sourceCode, moduleLoadedCallback, compiler, issues);
				}

				delete compiler;
				dispatch_group_leave(moduleSourceCompilersExist);
				dispatch_group_leave(moduleSourceCompilersExistGlobally);
			});
		}
	});

	return sourcesLoading;
}

/**
 * Unloads each of @a moduleKeys that is a module loaded in this environment.
 *
 * Returns the modules that were actually unloaded.
 * The caller is responsible for deallocating them with @ref VuoCompiler::destroyModule.
 *
 * @threadQueue{environmentQueue}
 */
set<VuoCompilerModule *> VuoCompiler::Environment::unloadCompiledModules(const set<string> &moduleKeys)
{
	set<VuoCompilerModule *> modulesUnloaded;

	for (set<string>::const_iterator i = moduleKeys.begin(); i != moduleKeys.end(); ++i)
	{
		string moduleKey = *i;
		VuoCompilerModule *module = NULL;

		map<string, VuoCompilerNodeClass *>::iterator nodeClassIter = nodeClasses.find(moduleKey);
		if (nodeClassIter != nodeClasses.end())
		{
			module = nodeClassIter->second;
			nodeClasses.erase(nodeClassIter);
		}
		else
		{
			map<string, VuoCompilerType *>::iterator typeIter = types.find(moduleKey);
			if (typeIter != types.end())
			{
				module = typeIter->second;
				types.erase(typeIter);
			}
			else
			{
				map<string, VuoCompilerModule *>::iterator libraryModuleIter = libraryModules.find(moduleKey);
				if (libraryModuleIter != libraryModules.end())
				{
					module = libraryModuleIter->second;
					libraryModules.erase(libraryModuleIter);
				}
			}
		}

		if (module)
		{
			modulesUnloaded.insert(module);
			removeFromDependencyGraph(module);
			modulesChanged();
		}
	}

	return modulesUnloaded;
}

/**
 * Deletes the cached compiled file for each of @a moduleKeys.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::deleteModulesCompiledFromSourceCode(const set<string> &moduleKeys)
{
	for (set<string>::const_iterator i = moduleKeys.begin(); i != moduleKeys.end(); ++i)
	{
		string moduleKey = *i;

		map<string, ModuleInfo *>::iterator iter = moduleFilesAtSearchPath[getCompiledModuleCachePath()].find(moduleKey);
		if (iter != moduleFilesAtSearchPath[getCompiledModuleCachePath()].end())
			VuoFileUtilities::deleteFile(iter->second->getFile()->path());
	}
}

/**
 * Loads a node class, type, or library module from a file, and adds it to the environment.
 *
 * Assumes that the module is not already present in the environment.
 *
 * @threadQueue{environmentQueue}
 */
VuoCompilerModule * VuoCompiler::Environment::loadModule(ModuleInfo *moduleInfo)
{
	string moduleKey = moduleInfo->getModuleKey();

	// Skip certain LLVM modules that definitely aren't Vuo modules to avoid adding struct types defined in them to the LLVM
	// context, resulting in mismatched struct types in code generation (e.g. %struct.NodeContext and %struct.NodeContext.1).
	if (VuoStringUtilities::beginsWith(moduleKey, "libVuo"))
		return NULL;

	// Don't try to load single-target parts
	// (such as those found in `build/test/TestControlAndTelemetry`),
	// since they just fail and pollute the logs.
	if (VuoStringUtilities::endsWith(moduleKey, "-x86_64")
	 || VuoStringUtilities::endsWith(moduleKey, "-arm64"))
		return nullptr;

	__block size_t inputDataBytes;
	__block char *rawInputData;
	dispatch_sync(llvmQueue, ^{
		try
		{
			rawInputData = moduleInfo->getFile()->getContentsAsRawData(inputDataBytes);
		}
		catch (VuoException &e)
		{
			rawInputData = NULL;
			VUserLog("Warning: Couldn't load module '%s'. Its file may have been deleted. (%s)", moduleKey.c_str(), e.what());
		}
	});
	if (! rawInputData)
		return NULL;

	char *processedInputData;
#if VUO_PRO
	processedInputData = loadModule_Pro0(moduleInfo, moduleKey, inputDataBytes, rawInputData);
#else
	processedInputData = rawInputData;
#endif

	Module *module = NULL;
	set<string> moduleArchs;
	bool moduleParseError = !processedInputData;
	if (!moduleParseError)
	{
		string moduleReadError;
		string arch = VuoCompiler::getTargetArch(target);
		VuoLog_status("Loading module \"%s\" (%s)", moduleKey.c_str(), arch.c_str());
		module = readModuleFromBitcodeData(processedInputData, inputDataBytes, arch, moduleArchs, moduleReadError);
		VuoLog_status(NULL);
		free(processedInputData);

		if (!module)
		{
			moduleParseError = true;

			if (VuoFileUtilities::arePathsEqual(moduleInfo->getSearchPath(), getCompiledModuleCachePath()))
				VuoFileUtilities::deleteFile(moduleInfo->getFile()->path());
			else
				VUserLog("Error: Couldn't load module '%s' into %s environment: %s.", moduleKey.c_str(), arch.c_str(), moduleReadError.c_str());
		}
	}

	if (!moduleParseError)
	{
		string modulePath;
		if (! moduleInfo->getFile()->isInArchive())
			modulePath = moduleInfo->getFile()->path();

		VuoCompilerCompatibility moduleCompatibility = (VuoFileUtilities::arePathsEqual(moduleInfo->getSearchPath(), getCompiledModuleCachePath()) ?
															VuoCompilerCompatibility::compatibilityWithAnySystem() :
															VuoCompilerCompatibility::compatibilityWithArchitectures(moduleArchs));

		__block VuoCompilerModule *compilerModule;
		dispatch_sync(llvmQueue, ^{
						  compilerModule = VuoCompilerModule::newModule(moduleKey, module, modulePath, moduleCompatibility);
					  });

		if (compilerModule)
		{
			if (dynamic_cast<VuoCompilerNodeClass *>(compilerModule))
				nodeClasses[moduleKey] = static_cast<VuoCompilerNodeClass *>(compilerModule);
			else if (dynamic_cast<VuoCompilerType *>(compilerModule))
				types[moduleKey] = static_cast<VuoCompilerType *>(compilerModule);
			else
				libraryModules[moduleKey] = compilerModule;

			VuoNodeSet *nodeSet = VuoNodeSet::createNodeSetForModule(moduleInfo->getFile());
			if (nodeSet)
			{
				map<string, VuoNodeSet *>::iterator nodeSetIter = nodeSetForName.find(nodeSet->getName());
				if (nodeSetIter == nodeSetForName.end())
				{
					nodeSetForName[nodeSet->getName()] = nodeSet;
				}
				else
				{
					delete nodeSet;
					nodeSet = nodeSetIter->second;
				}
				compilerModule->getPseudoBase()->setNodeSet(nodeSet);
			}

#if VUO_PRO
			loadModule_Pro1(rawInputData, processedInputData, compilerModule);
#endif

			compilerModule->setBuiltIn( isBuiltInOriginal() );

			return compilerModule;
		}
		else
		{
			destroyLlvmModule(module);
		}
	}

	return NULL;
}

/**
 * Unloads the old version of this node class (if any) and adds the new version to this environment.
 *
 * @threadQueue{environmentQueue};
 */
void VuoCompiler::Environment::replaceNodeClass(VuoCompilerNodeClass *newNodeClass)
{
	string moduleKey = newNodeClass->getBase()->getModuleKey();

	VuoCompilerNodeClass *oldNodeClass = nodeClasses[moduleKey];
	if (oldNodeClass)
	{
		removeFromDependencyGraph(oldNodeClass);
		destroyModule(oldNodeClass);
	}

	nodeClasses[moduleKey] = newNodeClass;
	addToDependencyGraph(newNodeClass);
	modulesChanged();
}

void VuoCompiler::Environment::addToDependencyGraph(VuoCompilerModule *module)
{
	DependencyGraphVertex *vertex = DependencyGraphVertex::vertexForDependency(module->getPseudoBase()->getModuleKey(), dependencyGraph);
	dependencyGraph->addVertex(vertex);

	vertex->setEnvironment(this);

	set<string> dependencies = module->getDependencies();
	for (set<string>::iterator i = dependencies.begin(); i != dependencies.end(); ++i)
	{
		DependencyGraphVertex *depVertex = DependencyGraphVertex::vertexForDependency(*i, dependencyGraph);
		dependencyGraph->addEdge(vertex, depVertex);
	}
}

void VuoCompiler::Environment::removeFromDependencyGraph(VuoCompilerModule *module)
{
	DependencyGraphVertex *vertex = DependencyGraphVertex::vertexForDependency(module->getPseudoBase()->getModuleKey(), dependencyGraph);
	dependencyGraph->removeVertex(vertex);
}

/**
 * Updates the data-and-event ports of each known node class to match them up with known types.
 * This method needs to be called between when the last node class or type is loaded and
 * when a composition is compiled. It can be called multiple times.
 *
 * @param inheritedTypes Types in environments at broader levels of scope and thus available to
 *    this environment.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::reifyPortTypes(const map<string, VuoCompilerType *> &inheritedTypes)
{
	map<string, VuoCompilerType *> availableTypes = inheritedTypes;
	availableTypes.insert(types.begin(), types.end());

	for (map<string, VuoCompilerNodeClass *>::const_iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
	{
		VuoNodeClass *nodeClass = i->second->getBase();

		vector<VuoPortClass *> inputPortClasses = nodeClass->getInputPortClasses();
		vector<VuoPortClass *> outputPortClasses = nodeClass->getOutputPortClasses();
		vector<VuoPortClass *> portClasses;
		portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
		portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());

		for (vector<VuoPortClass *>::iterator j = portClasses.begin(); j != portClasses.end(); ++j)
		{
			VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>((*j)->getCompiler());
			VuoType *baseType = portClass->getDataVuoType();

			if (baseType && ! baseType->hasCompiler())
			{
				string typeName = baseType->getModuleKey();
				VuoCompilerType *reifiedType = NULL;

				VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(baseType);
				if (genericType)
				{
					reifiedType = VuoCompilerGenericType::newGenericType(genericType, availableTypes);
					if (reifiedType) {
						genericTypes.insert( static_cast<VuoCompilerGenericType *>(reifiedType) );
					}
				}
				else
				{
					map<string, VuoCompilerType *>::iterator reifiedTypeIter = availableTypes.find(typeName);
					if (reifiedTypeIter != availableTypes.end())
					{
						delete baseType;
						reifiedType = reifiedTypeIter->second;
					}
				}

				if (reifiedType) {
					portClass->setDataVuoType(reifiedType->getBase());
				}
			}
		}

		vector<VuoCompilerTriggerDescription *> triggers = nodeClass->getCompiler()->getTriggerDescriptions();
		for (vector<VuoCompilerTriggerDescription *>::iterator j = triggers.begin(); j != triggers.end(); ++j)
		{
			VuoCompilerTriggerDescription *trigger = *j;
			VuoType *baseType = trigger->getDataType();

			if (baseType && ! baseType->hasCompiler())
			{
				string typeName = baseType->getModuleKey();
				map<string, VuoCompilerType *>::iterator reifiedTypeIter = availableTypes.find(typeName);
				if (reifiedTypeIter != availableTypes.end())
				{
					delete baseType;
					VuoCompilerType *reifiedType = reifiedTypeIter->second;
					trigger->setDataType(reifiedType->getBase());
				}
			}
		}
	}
}

/**
 * Inventories the items that should be included in this environment's cache.
 *
 * @param[out] cacheableModulesAndDependencies The keys/names of all modules and static libraries that should be contained
 *     in this environment's cache.
 * @param[out] dylibsNeededToLinkToThisCache The paths of all dylibs that the module cache dylib links to.
 * @param[out] frameworksNeededToLinkToThisCache The names of all frameworks that the cache dylib links to.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::getCacheableModulesAndDependencies(set<string> &cacheableModulesAndDependencies,
																  set<string> &dylibsNeededToLinkToThisCache,
																  set<string> &frameworksNeededToLinkToThisCache)
{
	if (isModuleCacheInitialized && ! isModuleCacheableDataDirty)
	{
		cacheableModulesAndDependencies = moduleCacheContents;
		dylibsNeededToLinkToThisCache = moduleCacheDylibs;
		frameworksNeededToLinkToThisCache = moduleCacheFrameworks;
		return;
	}

	VuoCompilerCompatibility compositionTargets = VuoCompilerCompatibility::compatibilityWithTargetTriple(target);

	// Include all modules…
	map<string, VuoCompilerModule *> allModules;
	allModules.insert(nodeClasses.begin(), nodeClasses.end());
	allModules.insert(types.begin(), types.end());
	allModules.insert(libraryModules.begin(), libraryModules.end());

	set<string> dependencies;
	for (map<string, VuoCompilerModule *>::iterator i = allModules.begin(); i != allModules.end(); ++i)
	{
		string moduleKey = i->first;
		VuoCompilerModule *module = i->second;

#if VUO_PRO
		// … except Pro modules and modules that depend on them…
		if (module->requiresPro())
			continue;
#endif

		// … and incompatible modules.
		if (! module->getCompatibleTargets().isCompatibleWith(compositionTargets))
			continue;

		cacheableModulesAndDependencies.insert(moduleKey);

		set<string> moduleDependencies = module->getDependencies();
		dependencies.insert(moduleDependencies.begin(), moduleDependencies.end());
	}

	// For the built-in environment, include Vuo's core dependencies.
	if (builtIn && ! generated)
	{
		vector<string> coreDependencies = getCoreVuoDependencies();
		dependencies.insert(coreDependencies.begin(), coreDependencies.end());
	}

	// Include all dependencies of the included module that are located in this environment.
	// (All modules are already included, so we only need to handle non-module dependencies.)
	for (set<string>::iterator i = dependencies.begin(); i != dependencies.end(); ++i)
	{
		string dependency = *i;
		if (allModules.find(dependency) == allModules.end())
		{
			if (VuoStringUtilities::endsWith(dependency, ".framework"))
				frameworksNeededToLinkToThisCache.insert(dependency);
			else
			{
				string dependencyPath = VuoCompiler::getLibraryPath(dependency, librarySearchPaths);
				if (! dependencyPath.empty())
				{
					if (VuoStringUtilities::endsWith(dependencyPath, ".dylib"))
						dylibsNeededToLinkToThisCache.insert(dependencyPath);
					else
						cacheableModulesAndDependencies.insert(dependency);
				}
			}
		}
	}

	moduleCacheDylibs = dylibsNeededToLinkToThisCache;
	moduleCacheFrameworks = frameworksNeededToLinkToThisCache;

	if (builtIn)
	{
		currentModuleCacheDylib = VuoFileUtilities::buildModuleCacheDylibPath(moduleCachePath, builtIn, generated);

		// Give each built-in cache arch a unique name, so they can be built in parallel.
		if (!vuoFrameworkInProgressPath.empty())
			currentModuleCacheDylib += "-" + VuoCompiler::getTargetArch(target);
	}
}

/**
 * Attempts to make the module cache available — either making it available initially or making it available
 * again after a change to this environment's modules has made it out-of-date.
 *
 * If the cache is out-of-date (and @a shouldUseExistingCache is false), it is scheduled to be rebuilt
 * asynchronously.
 *
 * If this environment's modules haven't changed since the last time this function was called, this call
 * just returns without checking or rebuilding the cache.
 *
 * @param shouldUseExistingCache If true, the existing cache (if any) is used, without checking if it's
 *     consistent with this environment.
 * @param compiler The compiler to use for linking the cache (if it needs to be rebuilt).
 * @param cacheableModulesAndDependencies The items that the cache should contain, from @ref getCacheableModulesAndDependencies.
 * @param dylibsNeededToLinkToCaches Dynamic libraries that the cache should link to, from @ref getCacheableModulesAndDependencies.
 * @param frameworksNeededToLinkToCaches Frameworks that the cache should link to, from @ref getCacheableModulesAndDependencies.
 * @param lastPrerequisiteModuleCacheRebuild The most recent time that any of the caches that this cache depends on were rebuilt.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::useModuleCache(bool shouldUseExistingCache, VuoCompiler *compiler,
											  set<string> cacheableModulesAndDependencies,
											  set<string> dylibsNeededToLinkToCaches, set<string> frameworksNeededToLinkToCaches,
											  unsigned long lastPrerequisiteModuleCacheRebuild)
{
	// Ignore the cache if the `prelinkCache` preference is false.

	static dispatch_once_t checked = 0;
	static bool prelinkCache = true;
	dispatch_once(&checked, ^{
		Boolean valid;
		bool result = CFPreferencesGetAppBooleanValue(CFSTR("prelinkCache"), CFSTR("org.vuo.Editor"), &valid);
		if (valid)
			prelinkCache = result;
	});
	if (! prelinkCache)
	{
		VDebugLog("Ignoring the module cache since the 'prelinkCache' preference is false.");
		return;
	}

	// Don't do anything if this environment doesn't have a cache configured.

	if (moduleCachePath.empty())
		return;

	// Don't bother rechecking the cache if neither the modules in this environment nor the caches that it depends on have changed.

	string cacheDescription = VuoFileUtilities::buildModuleCacheDescription(moduleCachePath, generated);
	if (isModuleCacheInitialized && ! isModuleCacheableDataDirty &&
			(builtIn || lastModuleCacheRebuild >= lastPrerequisiteModuleCacheRebuild))
	{
		VDebugLog("No need to recheck %s.", cacheDescription.c_str());
		return;
	}

	try
	{
		VDebugLog("Checking if %s is up-to-date…", cacheDescription.c_str());

		bool isCacheUpToDate = true;

		if (isModuleCacheInitialized && isModuleCacheableDataDirty)
			isCacheUpToDate = false;

		isModuleCacheInitialized = true;
		isModuleCacheableDataDirty = false;

		string indexPath = VuoFileUtilities::buildModuleCacheIndexPath(moduleCachePath, builtIn, generated);
		if (!vuoFrameworkInProgressPath.empty())
			indexPath += "-" + VuoCompiler::getTargetArch(target);
		string dir, file, ext;
		VuoFileUtilities::splitPath(indexPath, dir, file, ext);
		string indexFileName = file + "." + ext;

		// Create the cache directory and index if they don't already exist. (If they do exist, don't affect the last-modified times.)

		bool dirExists = VuoFileUtilities::fileExists(moduleCachePath);
		if (! dirExists)
		{
			if (shouldUseExistingCache)
				throw VuoException("Trying to use the existing cache, but the cache directory doesn't exist.", false);

			VuoFileUtilities::makeDir(moduleCachePath);
			isCacheUpToDate = false;
		}

		bool indexFileExists = VuoFileUtilities::fileExists(indexPath);
		if (! indexFileExists)
		{
			if (shouldUseExistingCache)
				throw VuoException("Trying to use the existing cache, but the cache index doesn't exist.", false);

			VuoFileUtilities::createFile(indexPath);
			isCacheUpToDate = false;
		}

		// Lock the cache for reading.

		VuoFileUtilities::File *fileForLocking;
		{
			fileForLocking = moduleCacheFileForLocking[indexPath];
			if (! fileForLocking)
			{
				fileForLocking = new VuoFileUtilities::File(moduleCachePath, indexFileName);
				moduleCacheFileForLocking[indexPath] = fileForLocking;
			}

			if (!fileForLocking->lockForReading())
				VDebugLog("\tWarning: Couldn't lock for reading.");
		}

		// If this is the first time this Environment is using its cache, see if there's a dylib on disk.

		if (currentModuleCacheDylib.empty())
			currentModuleCacheDylib = VuoFileUtilities::findLatestRevisionOfModuleCacheDylib(moduleCachePath, builtIn, generated, lastModuleCacheRebuild);

		if (shouldUseExistingCache && currentModuleCacheDylib.empty())
			throw VuoException("Trying to use the existing cache, but the cache dylib doesn't exist.", false);

		// Check if the dylib is newer than the other caches that it depends on.

		if (isCacheUpToDate)
			isCacheUpToDate = lastModuleCacheRebuild >= lastPrerequisiteModuleCacheRebuild;

		// Check if the dylib looks remotely valid.

		if (isCacheUpToDate)
		{
			bool dylibHasData = VuoFileUtilities::fileContainsReadableData(currentModuleCacheDylib);
			if (! dylibHasData)
			{
				if (shouldUseExistingCache)
					throw VuoException("Trying to use the existing cache, but the cache doesn't contain readable data.", false);
				else
					isCacheUpToDate = false;
			}
		}

		// List the items actually in the cache, according to its index.

		const char separator = '\n';
		if (isCacheUpToDate || shouldUseExistingCache)
		{
			VuoFileUtilities::File indexFile(moduleCachePath, indexFileName);
			string index = indexFile.getContentsAsString();
			vector<string> actualIndex = VuoStringUtilities::split(index, separator);

			moduleCacheContents.clear();
			moduleCacheContents.insert(actualIndex.begin(), actualIndex.end());

			if (shouldUseExistingCache)
			{
				isModuleCacheAvailable = true;
				return;
			}
		}

		// Check if the list of actual items matches the list of expected items.

		if (isCacheUpToDate)
		{
			if (moduleCacheContents.size() != cacheableModulesAndDependencies.size())
				isCacheUpToDate = false;
			else
			{
				set<string> contentsInBoth;
				std::set_intersection(moduleCacheContents.begin(), moduleCacheContents.end(),
									  cacheableModulesAndDependencies.begin(), cacheableModulesAndDependencies.end(),
									  std::inserter(contentsInBoth, contentsInBoth.begin()));

				if (contentsInBoth.size() != cacheableModulesAndDependencies.size())
					isCacheUpToDate = false;
			}
		}

		// Check if the cache is newer than all of the modules in it.

		if (isCacheUpToDate)
		{
			unsigned long cacheLastModified = VuoFileUtilities::getFileLastModifiedInSeconds(currentModuleCacheDylib);

			for (map<string, map<string, ModuleInfo *> >::iterator i = moduleFilesAtSearchPath.begin(); i != moduleFilesAtSearchPath.end(); ++i)
			{
				for (map<string, ModuleInfo *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					if (! j->second->isOlderThan(cacheLastModified))
					{
						isCacheUpToDate = false;
						break;
					}
				}
			}
		}

		// If the cache is consistent with this environment, we're done.

		if (isCacheUpToDate)
		{
			VDebugLog("Up-to-date.");

			isModuleCacheAvailable = true;
			return;
		}

		// Otherwise, (re)build the cache.

		if (! builtIn)
		{
			currentModuleCacheDylib = VuoFileUtilities::buildModuleCacheDylibPath(moduleCachePath, builtIn, generated);

			struct timeval t;
			gettimeofday(&t, NULL);
			lastModuleCacheRebuild = t.tv_sec;
		}

		dispatch_async(moduleCacheBuildingQueue, ^{
			VUserLog("Rebuilding %s…", cacheDescription.c_str());

			set<Module *> modulesToLink;
			set<string> librariesToLink;
			set<string> frameworksToLink;
			{
				compiler->getLinkerInputs(cacheableModulesAndDependencies, Optimization_SmallBinary, modulesToLink, librariesToLink, frameworksToLink);

				librariesToLink.insert(dylibsNeededToLinkToCaches.begin(), dylibsNeededToLinkToCaches.end());
				frameworksToLink.insert(frameworksNeededToLinkToCaches.begin(), frameworksNeededToLinkToCaches.end());
			}

			bool gotLockForWriting = false;
			try
			{
				// Try to upgrade the file lock for writing.
				gotLockForWriting = fileForLocking->lockForWriting(true);
				if (! gotLockForWriting)
					throw VuoException("The cache file is being used by another process. "
									   "If any composition windows are open from previous Vuo sessions, quit them. "
									   "If any processes whose names start with \"VuoComposition\" or one of your composition file names appear in Activity Monitor, force-quit them.",
									   false);

				// Link the dependencies to create the cached resources dylib in a temporary file.
				string dir, file, ext;
				VuoFileUtilities::splitPath(currentModuleCacheDylib, dir, file, ext);
				string tmpPath = VuoFileUtilities::makeTmpFile(file, "dylib");
				vector<string> rPaths = compiler->getRunPathSearchPaths(this);
				VuoCompilerIssues *issues = new VuoCompilerIssues;
				compiler->link(tmpPath, modulesToLink, librariesToLink, frameworksToLink, true, rPaths, false, issues);

				if (! issues->isEmpty())
					VUserLog("Warning: May not have fully rebuilt %s for the \"faster build\" optimization:\n%s", cacheDescription.c_str(), issues->getLongDescription(false).c_str());

				// Move the temporary file into the cache.
				VuoFileUtilities::moveFile(tmpPath, currentModuleCacheDylib);

				// Change the dylib's ID from the temporary path to the path within the cache.
				VuoFileUtilities::executeProcess({
					getVuoFrameworkPath() + "/Helpers/install_name_tool",
					"-id",
					currentModuleCacheDylib,
					currentModuleCacheDylib,
				});

				// Ad-hoc code-sign the runtime-generated System and User caches,
				// but don't ad-hoc code-sign the buildtime-generated Builtin module cache
				// since `framework/CMakeLists.txt` later changes its ID/rpath/loads.
				if (vuoFrameworkInProgressPath.empty())
					adHocCodeSign(currentModuleCacheDylib);

				// Write the list of dependencies to the index file.
				vector<string> expectedContents(cacheableModulesAndDependencies.begin(), cacheableModulesAndDependencies.end());
				string index = VuoStringUtilities::join(expectedContents, separator);
				VuoFileUtilities::writeStringToFile(index, indexPath);

				// Delete any older revisions of the dylib.
				VuoFileUtilities::deleteOtherRevisionsOfModuleCacheDylib(currentModuleCacheDylib);

				// Downgrade the file lock back to reading.
				if (!fileForLocking->lockForReading())
					VDebugLog("\tWarning: Couldn't downgrade the lock back to reading.");

				dispatch_sync(environmentQueue, ^{
					moduleCacheContents = cacheableModulesAndDependencies;
					isModuleCacheAvailable = true;
				});
			}
			catch (VuoException &e)
			{
				// Downgrade the file lock back to reading.
				if (gotLockForWriting)
					if (!fileForLocking->lockForReading())
						VDebugLog("\tWarning: Couldn't downgrade the lock back to reading.");

				VUserLog("Warning: Couldn't rebuild %s for the \"faster build\" optimization: %s", cacheDescription.c_str(), e.what());
			}

			VUserLog("Done.");
		});
	}
	catch (VuoException &e)
	{
		if (vuoFrameworkInProgressPath.empty())
			VUserLog("Warning: Couldn't use %s for the \"faster build\" optimization: %s", cacheDescription.c_str(), e.what());
	}
}

/**
 * Waits for cache (re)builds scheduled by @ref useModuleCache to complete.
 */
void VuoCompiler::Environment::waitForModuleCachesToBuild(void)
{
	dispatch_sync(moduleCacheBuildingQueue, ^{});
}

/**
 * If the cache is available and @a moduleOrDependency is in it, returns true and outputs information about the cache.
 * Otherwise, returns false.
 *
 * @param moduleOrDependency The key/name of the module or dependency to search for.
 * @param cachePath The path of the cache dylib.
 *
 * @threadQueue{environmentQueue}
 */
bool VuoCompiler::Environment::findInModuleCache(const string &moduleOrDependency, string &cachePath)
{
	if (isModuleCacheAvailable && moduleCacheContents.find(moduleOrDependency) != moduleCacheContents.end())
	{
		cachePath = currentModuleCacheDylib;
		return true;
	}

	return false;
}

/**
 * Returns the path of the most recent revision of this environment's module cache dylib.
 */
string VuoCompiler::Environment::getCurrentModuleCacheDylib(void)
{
	return currentModuleCacheDylib;
}

/**
 * Returns the time (in seconds since a reference date) when this Environment instance last scheduled its cache to be rebuilt;
 * or, if none and this is not a built-in environment, the time when the module cache dylib was last modified; otherwise, 0.
 */
unsigned long VuoCompiler::Environment::getLastModuleCacheRebuild(void)
{
	return lastModuleCacheRebuild;
}

/**
 * Call this function when modules have been added to, modified in, or removed from this environment.
 * It flags the cache as needing rebuilt.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::Environment::modulesChanged(void)
{
	isModuleCacheableDataDirty = true;
	isModuleCacheAvailable = false;
}

/**
 * Returns true if this is the environment for built-in original modules.
 */
bool VuoCompiler::Environment::isBuiltInOriginal()
{
	return builtIn && ! generated;
}

/**
 * Returns true if this environment is for built-in modules.
 */
bool VuoCompiler::Environment::isBuiltIn()
{
	return builtIn;
}

/**
 * Returns true if this environment is for generated modules, false if for original modules.
 */
bool VuoCompiler::Environment::isGenerated()
{
	return generated;
}

/**
 * Returns an identifier for this environment, suitable for log messages.
 */
string VuoCompiler::Environment::getName()
{
	if (isBuiltInOriginal())
		return "builtin";
	else if (this == sharedEnvironments[target][0][1])
		return "builtin (generated)";
	else if (this == sharedEnvironments[target][1][0])
		return "system";
	else if (this == sharedEnvironments[target][1][1])
		return "system (generated)";
	else if (this == sharedEnvironments[target][2][0])
		return "user";
	else if (this == sharedEnvironments[target][2][1])
		return "user (generated)";
	return "composition-local";
}

/**
 * Calls @a doForEnvironment for each of the installed environments, in order from broadest to narrowest scope.
 */
void VuoCompiler::applyToInstalledEnvironments(void (^doForEnvironment)(Environment *))
{
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  doForEnvironment((*i)[0]);
					  }
				  });
}

/**
 * Calls @a doForEnvironment for each of the installed environments, in order from broadest to narrowest scope.
 */
void VuoCompiler::applyToInstalledEnvironments(void (^doForEnvironment)(Environment *, bool *, string), bool *result, string arg)
{
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  doForEnvironment((*i)[0], result, arg);
					  }
				  });
}

/**
 * Calls @a doForEnvironment for each of the installed and generated environments, in order from broadest to narrowest scope.
 */
void VuoCompiler::applyToAllEnvironments(void (^doForEnvironment)(Environment *environment))
{
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  for (vector<Environment *>::iterator j = i->begin(); j != i->end(); ++j) {
							  doForEnvironment(*j);
						  }
					  }
				  });
}

/**
 * Creates a compiler instance that can be used for compiling and linking.
 *
 * @param compositionPath If this compiler will be compiling a composition and its path is already known,
 *     pass the path so the compiler can locate composition-local modules. If the path is not yet known,
 *     it can be set later with @ref VuoCompiler::setCompositionPath or @ref VuoCompiler::compileComposition. If not compiling a composition,
 *     pass an empty string.
 * @param target The LLVM Target Triple to use for this compiler instance.
 *               Affects which slice of multi-archtecture binaries is loaded,
 *               and affects the bitcode that's generated.
 *               If no target is specified, the current process's architecture is used.
 * @version200Changed{Added `compositionPath` argument.}
 */
VuoCompiler::VuoCompiler(const string &compositionPath, string target)
{
	if (target.empty())
	{
		this->target = target = getProcessTarget();
		VDebugLog("%p  target=%s (from current process)", this, this->target.c_str());
	}
	else
	{
		this->target = target;
		VDebugLog("%p  target=%s", this, this->target.c_str());
	}

#if VUO_PRO
	init_Pro();
#endif

	shouldLoadAllModules = true;
	hasLoadedAllModules = false;
	modulesToLoadQueue = dispatch_queue_create("org.vuo.compiler.modules", NULL);
	moduleSourceCompilersExist = dispatch_group_create();
	moduleCacheBuilding = dispatch_group_create();
	dependencyGraph = NULL;
	compositionDependencyGraph = NULL;
	isVerbose = false;
	_shouldShowSplashWindow = false;
	delegate = NULL;
	delegateQueue = dispatch_queue_create("org.vuo.compiler.delegate", NULL);

	string vuoFrameworkPath = getVuoFrameworkPath();
	if (! vuoFrameworkPath.empty())
		clangPath = vuoFrameworkPath + "/Helpers/clang";
	else
		clangPath = LLVM_ROOT "/bin/clang";

	dispatch_sync(environmentQueue, ^{
					  allCompilers.insert(this);

					  if (sharedEnvironments[target].empty())
					  {
						  sharedEnvironments[target] = vector< vector<Environment *> >(3, vector<Environment *>(2, NULL));
						  for (vector< vector<Environment *> >::iterator i = sharedEnvironments[target].begin(); i != sharedEnvironments[target].end(); ++i)
							  for (vector<Environment *>::iterator j = i->begin(); j != i->end(); ++j)
								  *j = new Environment(this->target, i == sharedEnvironments[target].begin(), j != i->begin());

						  vector<string> builtInModuleSearchPaths = Environment::getBuiltInModuleSearchPaths();
						  for (vector<string>::iterator i = builtInModuleSearchPaths.begin(); i != builtInModuleSearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addModuleSearchPath(*i, false);

						  vector<string> builtInHeaderSearchPaths = Environment::getBuiltInHeaderSearchPaths();
						  for (vector<string>::iterator i = builtInHeaderSearchPaths.begin(); i != builtInHeaderSearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addHeaderSearchPath(*i);

						  vector<string> builtInLibrarySearchPaths = Environment::getBuiltInLibrarySearchPaths();
						  for (vector<string>::iterator i = builtInLibrarySearchPaths.begin(); i != builtInLibrarySearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addLibrarySearchPath(*i);

						  vector<string> builtInFrameworkSearchPaths = Environment::getBuiltInFrameworkSearchPaths();
						  for (vector<string>::iterator i = builtInFrameworkSearchPaths.begin(); i != builtInFrameworkSearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addFrameworkSearchPath(*i);

						  // Allow system administrator to override Vuo.framework modules
						  sharedEnvironments[target][1][0]->addModuleSearchPath(VuoFileUtilities::getSystemModulesPath());
						  sharedEnvironments[target][1][0]->addLibrarySearchPath(VuoFileUtilities::getSystemModulesPath());

						  // Allow user to override Vuo.framework and system-wide modules
						  sharedEnvironments[target][2][0]->addModuleSearchPath(VuoFileUtilities::getUserModulesPath());
						  sharedEnvironments[target][2][0]->addLibrarySearchPath(VuoFileUtilities::getUserModulesPath());

						  // Set up module cache paths.
						  // Since the built-in module caches are part of Vuo.framework (put there by `generateBuiltInModuleCaches`),
						  // only attempt to use module caches if Vuo.framework exists.
						  string vuoFrameworkPath = getVuoFrameworkPath();
						  if (! vuoFrameworkPath.empty())
						  {
							  vector<string> moduleCachePaths(3);
							  moduleCachePaths[0] = vuoFrameworkPath + "/Modules/Builtin";
							  moduleCachePaths[1] = VuoFileUtilities::getCachePath() + "/System";
							  moduleCachePaths[2] = VuoFileUtilities::getCachePath() + "/User";

							  for (size_t i = 0; i < moduleCachePaths.size(); ++i)
							  {
								  string moduleCachePath = moduleCachePaths[i];

								  sharedEnvironments[target][i][0]->setModuleCachePath(moduleCachePath, true);
								  sharedEnvironments[target][i][1]->setModuleCachePath(moduleCachePath, false);
							  }
						  }
					  }
				  });

	setCompositionPath(compositionPath);
}

/**
 * Destructor.
 */
VuoCompiler::~VuoCompiler(void)
{
#if VUO_PRO
	fini_Pro();
#endif

	dispatch_sync(environmentQueue, ^{
		allCompilers.erase(this);
	});

	dispatch_group_wait(moduleCacheBuilding, DISPATCH_TIME_FOREVER);
	dispatch_release(moduleCacheBuilding);

	dispatch_group_wait(moduleSourceCompilersExist, DISPATCH_TIME_FOREVER);
	dispatch_release(moduleSourceCompilersExist);

	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
		(*i)[0]->removeCompilerToNotify(this);

	dispatch_sync(delegateQueue, ^{});

	dispatch_release(modulesToLoadQueue);
	dispatch_release(delegateQueue);

	delete dependencyGraph;
	delete compositionDependencyGraph;
}

/**
 * Resets the static environments and caches. Not thread-safe.
 */
void VuoCompiler::reset(void)
{
	dispatch_group_wait(moduleSourceCompilersExistGlobally, DISPATCH_TIME_FOREVER);

	dispatch_sync(environmentQueue, ^{

	for (auto e : sharedEnvironments)
		for (vector< vector<Environment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
		{
			(*i)[0]->stopWatchingModuleSearchPaths();
			dispatch_sync((*i)[0]->moduleSearchPathContentsChangedQueue, ^{});
		}

	for (auto e : environmentsForCompositionFamily)
		for (map< string, vector<Environment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
		{
			(i->second)[0]->stopWatchingModuleSearchPaths();
			dispatch_sync((i->second)[0]->moduleSearchPathContentsChangedQueue, ^{});
		}

	for (auto e : sharedEnvironments)
		for (vector< vector<Environment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
			for (vector<Environment *>::iterator j = i->begin(); j != i->end(); ++j)
				delete *j;

	for (auto e : environmentsForCompositionFamily)
		for (map< string, vector<Environment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
			for (vector<Environment *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				delete *j;

	allCompilers.clear();
	sharedEnvironments.clear();
	environmentsForCompositionFamily.clear();

	});
}

/**
 * Sets the delegate that receives messages from the compiler when modules are loaded. May be null.
 *
 * @version200New
 */
void VuoCompiler::setDelegate(VuoCompilerDelegate *delegate)
{
	dispatch_async(delegateQueue, ^{
					   this->delegate = delegate;
				   });
}

/**
 * If this compiler will be compiling a composition, pass its path here (if not already passed to
 * @ref VuoCompiler::VuoCompiler) so the compiler can locate composition-local modules.
 *
 * If you compile a composition and haven't told the compiler its path, the composition will be able
 * to use modules at the shared scopes (built-in, system, user) but not at the composition-local scope.
 *
 * @version200New
 */
void VuoCompiler::setCompositionPath(const string &compositionPath)
{
	string compositionModulesDir;
	string compositionBaseDir;
	lastCompositionIsSubcomposition = false;
	if (! compositionPath.empty())
	{
		compositionModulesDir = VuoFileUtilities::getCompositionLocalModulesPath(compositionPath);

		string file, ext;
		VuoFileUtilities::splitPath(compositionModulesDir, compositionBaseDir, file, ext);
		VuoFileUtilities::canonicalizePath(compositionBaseDir);

		string compositionDir;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, ext);
		VuoFileUtilities::canonicalizePath(compositionDir);
		lastCompositionIsSubcomposition = (compositionDir == compositionModulesDir);
	}

	// Set up `environments` to contain all environments available to this compiler, in order from broadest to narrowest.

	dispatch_sync(environmentQueue, ^{
					  if (! environments.empty() && compositionBaseDir == lastCompositionBaseDir) {
						  return;
					  }
					  lastCompositionBaseDir = compositionBaseDir;

					  // Clear out the existing environments for this compiler.

					  Environment *oldCompositionFamilyInstalledEnvironment = nullptr;
					  vector<Environment *> compositionEnvironments;
					  if (! environments.empty())
					  {
						  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
							  (*i)[0]->removeCompilerToNotify(this);
						  }

						  if (environments.size() >= 5) {
							  oldCompositionFamilyInstalledEnvironment = environments[3][0];
						  }

						  compositionEnvironments = environments.back();

						  environments.clear();
					  }

					  // If the composition is located in one of the shared environments (built-in, system, user),
					  // add that shared environment and any shared environments at broader scope to the compiler.
					  // If the composition is not in a shared environment, add all of the shared environments to the compiler.

					  bool isCompositionInSharedEnvironment = false;
					  for (vector< vector<Environment *> >::iterator i = sharedEnvironments[target].begin(); i != sharedEnvironments[target].end(); ++i)
					  {
						  environments.push_back(*i);

						  vector<string> moduleSearchPaths = (*i)[0]->getModuleSearchPaths();
						  for (vector<string>::iterator j = moduleSearchPaths.begin(); j != moduleSearchPaths.end(); ++j)
						  {
							  string moduleSearchPath = *j;
							  VuoFileUtilities::canonicalizePath(moduleSearchPath);
							  if (moduleSearchPath == compositionModulesDir)
							  {
								  isCompositionInSharedEnvironment = true;
								  break;
							  }
						  }

						  if (isCompositionInSharedEnvironment) {
							  break;
						  }
					  }

					  // If the composition is not in a shared environment, add the composition-family environment to the compiler.

					  Environment *newCompositionFamilyInstalledEnvironment = nullptr;
					  if (! isCompositionInSharedEnvironment && ! compositionPath.empty())
					  {
						  vector<Environment *> compositionFamilyEnvironments = environmentsForCompositionFamily[target][compositionBaseDir];
						  if (compositionFamilyEnvironments.empty())
						  {
							  compositionFamilyEnvironments = vector<Environment *>(2, NULL);
							  compositionFamilyEnvironments[0] = new Environment(this->target, false, false);
							  compositionFamilyEnvironments[1] = new Environment(this->target, false, true);
							  environmentsForCompositionFamily[target][compositionBaseDir] = compositionFamilyEnvironments;

							  // Allow the user to place modules/subcompositions in a Modules folder inside the composition folder.

							  compositionFamilyEnvironments[0]->addModuleSearchPath(compositionModulesDir);

							  // Locate this environment's cache in a folder whose name is the composition folder path with
							  // slashes replaced with Unicode Modifier Letter Colon.
							  string moduleCachePath = getCachePathForComposition(compositionBaseDir);

							  compositionFamilyEnvironments[0]->setModuleCachePath(moduleCachePath, true);
							  compositionFamilyEnvironments[1]->setModuleCachePath(moduleCachePath, false);
						  }
						  environments.push_back(compositionFamilyEnvironments);

						  newCompositionFamilyInstalledEnvironment = compositionFamilyEnvironments[0];
					  }

					  // Add the composition environment to the compiler (or add it back in if it already existed).

					  if (compositionEnvironments.empty())
					  {
						  compositionEnvironments = vector<Environment *>(2, NULL);
						  compositionEnvironments[0] = new Environment(this->target, false, false);
						  compositionEnvironments[1] = new Environment(this->target, false, true);
					  }
					  environments.push_back(compositionEnvironments);

					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  (*i)[0]->addCompilerToNotify(this);
					  }

					  delete dependencyGraph;
					  delete compositionDependencyGraph;
					  dependencyGraph = makeDependencyNetwork(environments, ^VuoDirectedAcyclicGraph * (Environment *env) { return env->getDependencyGraph(); });
					  compositionDependencyGraph = makeDependencyNetwork(environments, ^VuoDirectedAcyclicGraph * (Environment *env) { return env->getCompositionDependencyGraph(); });

					  // If the compiler has a different local Modules directory than before, notify the compiler's delegate
					  // of composition-family modules that are newly available/unavailable.

					  if (oldCompositionFamilyInstalledEnvironment != newCompositionFamilyInstalledEnvironment)
					  {
						  auto getModules = [] (Environment *env)
						  {
							  map<string, VuoCompilerModule *> modules;
							  if (env)
							  {
								  for (auto i : env->getNodeClasses()) {
									  modules.insert(i);
								  }
								  for (auto i : env->getTypes()) {
									  modules.insert(i);
								  }
								  for (auto i : env->getLibraryModules()) {
									  modules.insert(i);
								  }
							  }
							  return modules;
						  };

						  map<string, VuoCompilerModule *> modulesAdded = getModules(newCompositionFamilyInstalledEnvironment);
						  map<string, VuoCompilerModule *> modulesRemoved = getModules(oldCompositionFamilyInstalledEnvironment);

						  map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModified;
						  for (map<string, VuoCompilerModule *>::iterator add = modulesAdded.begin(); add != modulesAdded.end(); )
						  {
							  map<string, VuoCompilerModule *>::iterator rem = modulesRemoved.find(add->first);
							  if (rem != modulesRemoved.end())
							  {
								  modulesModified[add->first] = make_pair(rem->second, add->second);
								  modulesAdded.erase(add++);
								  modulesRemoved.erase(rem);
							  }
							  else
							  {
								  ++add;
							  }
						  }

						  if (! (modulesAdded.empty() && modulesModified.empty() && modulesRemoved.empty()) )
						  {
							  VuoCompilerIssues *issues = new VuoCompilerIssues();
							  VuoCompilerDelegate::LoadedModulesData *delegateData = new VuoCompilerDelegate::LoadedModulesData(set< pair<VuoCompilerModule *, VuoCompilerModule *> >(), set<VuoCompilerModule *>(), issues);
							  delegateData->retain();

							  Environment *scopeEnvironment = newCompositionFamilyInstalledEnvironment;
							  if (! scopeEnvironment) {
								  scopeEnvironment = compositionEnvironments.at(0);
							  }

							  loadedModules(modulesAdded, modulesModified, modulesRemoved, issues, delegateData, scopeEnvironment);
						  }
					  }
				  });
}

/**
 * Ties together the dependency graphs of @a environments into one giant dependency structure.
 *
 * @threadQueue{environmentQueue}
 */
VuoDirectedAcyclicNetwork * VuoCompiler::makeDependencyNetwork(const vector< vector<Environment *> > &environments,
															   VuoDirectedAcyclicGraph * (^graphForEnvironment)(Environment *))
{
	if (!graphForEnvironment)
		return NULL;

	VuoDirectedAcyclicNetwork *network = new VuoDirectedAcyclicNetwork();

	for (vector< vector<Environment *> >::const_iterator i = environments.begin(); i != environments.end(); ++i)
	{
		// Installed environment depends on generated environment in same scope.

		network->addEdge(graphForEnvironment(i->at(0)), graphForEnvironment(i->at(1)));

		// Installed and generated environments depend on installed environments in broader scopes.

		for (vector< vector<Environment *> >::const_iterator ii = environments.begin(); ii != i; ++ii)
		{
			network->addEdge(graphForEnvironment(i->at(0)), graphForEnvironment(ii->at(0)));
			network->addEdge(graphForEnvironment(i->at(1)), graphForEnvironment(ii->at(0)));
		}
	}

	return network;
}

/**
 * Loads node classes, types, and library modules, and compiles and loads module source files, if they have not
 * already been loaded.
 *
 * If this is the first call to this function in which @ref shouldLoadAllModules is true, this function loads
 * all modules and sources found in the compiler's search paths. Otherwise, this function loads only the
 * modules listed in @a moduleKeys and their dependencies.
 *
 * This function needs to be called before compiling a composition, linking a composition, or getting a listing
 * of loaded modules. It does not need to be called before compiling a node class.
 *
 * This function calls @ref VuoCompilerDelegate::loadedModules, passing the list of already-compiled modules
 * loaded as a result of this call. For not-already-compiled sources loaded as a result of this call,
 * there are one or more additional calls to @ref VuoCompilerDelegate::loadedModules. This function waits for
 * the not-already-compiled sources to load before returning.
 *
 * If the caller has just placed a file in a module search path, there's no guarantee that it will be loaded
 * by the next call to this function. However, if the file exists before the first VuoCompiler instance is
 * created, it is guaranteed to be found by the first call to this function.
 */
void VuoCompiler::loadModulesIfNeeded(const set<string> &moduleKeys)
{
	__block bool willLoadAllModules = false;
	if (moduleKeys.empty())
	{
		dispatch_sync(modulesToLoadQueue, ^{
						  if (shouldLoadAllModules && ! hasLoadedAllModules) {
							  willLoadAllModules = true;
							  hasLoadedAllModules = true;
						  }
					  });
	}

	if (! willLoadAllModules && moduleKeys.empty())
		return;

	// Load modules and start sources compiling.

	__block set<dispatch_group_t> sourcesLoading;
	dispatch_sync(environmentQueue, ^{
					  sourcesLoading = loadModulesAndSources(moduleKeys, set<string>(), set<string>(),
															 moduleKeys, set<string>(), set<string>(),
															 willLoadAllModules, false, nullptr, nullptr, nullptr, "");
				  });

	// Wait for subcompositions and specialized node classes to finish compiling and their modules to be loaded
	// to ensure that the next call to getNodeClass() will return them.

	for (set<dispatch_group_t>::iterator i = sourcesLoading.begin(); i != sourcesLoading.end(); ++i)
	{
		dispatch_group_wait(*i, DISPATCH_TIME_FOREVER);
		dispatch_release(*i);
	}
}

/**
 * Loads node classes, types, and library modules, and schedules module source files to be compiled.
 *
 * Returns a dispatch group that the caller can use to wait until the sources scheduled for
 * compilation by this call have been compiled and their modules loaded.
 *
 * @threadQueue{environmentQueue}
 */
set<dispatch_group_t> VuoCompiler::loadModulesAndSources(const set<string> &modulesAddedKeys, const set<string> &modulesModifiedKeys, const set<string> &modulesRemovedKeys,
														 const set<string> &sourcesAddedKeys, const set<string> &sourcesModifiedKeys, const set<string> &sourcesRemovedKeys,
														 bool willLoadAllModules, bool shouldRecompileSourcesIfUnchanged,
														 Environment *currentEnvironment, VuoCompilerIssues *issuesForCurrentEnvironment,
														 std::function<void(void)> moduleLoadedCallback, const string &moduleAddedOrModifiedSourceCode)
{
	//VLog("C=%p E=%p -- %lu %lu %lu  %lu %lu %lu  %i %i", this, currentEnvironment,
		 //modulesAddedKeys.size(), modulesModifiedKeys.size(), modulesRemovedKeys.size(),
		 //sourcesAddedKeys.size(), sourcesModifiedKeys.size(), sourcesRemovedKeys.size(),
		 //willLoadAllModules, shouldRecompileSourcesIfUnchanged);
	//if (modulesAddedKeys.size() == 1) VLog("    %s", modulesAddedKeys.begin()->c_str());
	//if (modulesModifiedKeys.size() == 1) VLog("    %s", modulesModifiedKeys.begin()->c_str());
	//if (modulesRemovedKeys.size() == 1) VLog("    %s", modulesRemovedKeys.begin()->c_str());

	// Organize the modules, source files, and issues by environment.

	map<Environment *, set<string> > modulesAdded;
	map<Environment *, set<string> > modulesModified;
	map<Environment *, set<string> > modulesRemoved;
	map<Environment *, set<string> > sourcesAdded;
	map<Environment *, set<string> > sourcesModified;
	map<Environment *, set<string> > sourcesRemoved;
	map<Environment *, set<string> > potentialSpecializedModules;

	if (currentEnvironment)
	{
		modulesAdded[currentEnvironment] = modulesAddedKeys;
		modulesModified[currentEnvironment] = modulesModifiedKeys;
		modulesRemoved[currentEnvironment] = modulesRemovedKeys;
		sourcesAdded[currentEnvironment] = sourcesAddedKeys;
		sourcesModified[currentEnvironment] = sourcesModifiedKeys;
		sourcesRemoved[currentEnvironment] = sourcesRemovedKeys;
	}
	else
	{
		Environment *genEnv = nullptr;
		if (! willLoadAllModules)
		{
			// If compiling a top-level composition, generated modules that were directly requested (in modulesAddedKeys) are
			// added at the composition scope so they won't be cached. This prevents link errors when running multiple
			// compositions in the current process (https://b33p.net/kosada/node/14317).

			int scope = environments.size() - (lastCompositionIsSubcomposition ? 2 : 1);
			genEnv = environments.at(scope).at(1);
			potentialSpecializedModules[genEnv] = modulesAddedKeys;
		}

		for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
		{
			Environment *env = (*i).at(0);

			ModuleInfoIterator modulesAddedIter = (willLoadAllModules ? env->listAllModules() : env->listModules(modulesAddedKeys));
			ModuleInfoIterator sourcesAddedIter = (willLoadAllModules ? env->listAllSourceFiles() : env->listSourceFiles(sourcesAddedKeys));
			ModuleInfoIterator sourcesModifiedIter = (willLoadAllModules ? env->listAllSourceFiles() : env->listSourceFiles(sourcesModifiedKeys));

			ModuleInfo *moduleInfo;
			while ((moduleInfo = modulesAddedIter.next()))
			{
				string moduleKey = moduleInfo->getModuleKey();

				modulesAdded[env].insert(moduleKey);

				if (! willLoadAllModules)
				{
					auto foundIter = potentialSpecializedModules[genEnv].find(moduleKey);
					if (foundIter != potentialSpecializedModules[genEnv].end())
						potentialSpecializedModules[genEnv].erase(foundIter);
				}
			}

			// If a source file and a compiled file for the same module are in the same folder,
			// the compiled file takes precedence.
			auto isCompiledModuleAtSameSearchPath = [&env] (ModuleInfo *sourceInfo)
			{
				ModuleInfo *compiledModuleInfo = env->listModule(sourceInfo->getModuleKey());
				return (compiledModuleInfo && compiledModuleInfo->getSearchPath() == sourceInfo->getSearchPath());
			};

			while ((moduleInfo = sourcesAddedIter.next()))
			{
				if (isCompiledModuleAtSameSearchPath(moduleInfo))
					continue;

				sourcesAdded[env].insert( moduleInfo->getModuleKey() );
			}

			while ((moduleInfo = sourcesModifiedIter.next()))
			{
				if (isCompiledModuleAtSameSearchPath(moduleInfo))
					continue;

				sourcesModified[env].insert( moduleInfo->getModuleKey() );
			}
		}
	}

	map<Environment *, VuoCompilerIssues *> issues;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);
		issues[env] = (env == currentEnvironment && issuesForCurrentEnvironment ? issuesForCurrentEnvironment : new VuoCompilerIssues());
	}

	// Check for circular dependencies in added/modified sources.

	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		// Check for circular dependencies involving sources being loaded within this environment.
		// For circular dependencies involving sources in different environments,
		// an error will be reported elsewhere due to one of them being outside of the other's scope.
		set<VuoDirectedAcyclicGraph::Vertex *> circularDependencies = env->getCompositionDependencyGraph()->getCycleVertices();

		set<string> sourcesAddedModified;
		sourcesAddedModified.insert(sourcesAdded[env].begin(), sourcesAdded[env].end());
		sourcesAddedModified.insert(sourcesModified[env].begin(), sourcesModified[env].end());

		for (set<string>::iterator j = sourcesAddedModified.begin(); j != sourcesAddedModified.end(); ++j)
		{
			string moduleKey = *j;

			bool found = false;
			for (set<VuoDirectedAcyclicGraph::Vertex *>::iterator k = circularDependencies.begin(); k != circularDependencies.end(); ++k)
			{
				DependencyGraphVertex *vertex = static_cast<DependencyGraphVertex *>(*k);
				if (vertex->getDependency() == moduleKey)
				{
					found = true;
					break;
				}
			}

			if (found)
			{
				sourcesAdded[env].erase(moduleKey);
				sourcesModified[env].erase(moduleKey);

				VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling subcomposition", moduleKey,
									   "Subcomposition contains itself",
									   "%moduleKey contains an instance of itself, "
									   "or contains another subcomposition that contains an instance of %moduleKey.");
				issue.setModuleKey(moduleKey);

				ModuleInfo *sourceInfo = env->listSourceFile(moduleKey);
				if (sourceInfo)
					issue.setFilePath(sourceInfo->getFile()->path());

				issues[env]->append(issue);
			}
		}
	}

	// Update the longest downstream paths for added/modified sources.

	for (const vector<Environment *> &envs : environments)
	{
		Environment *env = envs.at(0);

		set<string> sourcesAddedModified;
		sourcesAddedModified.insert(sourcesAdded[env].begin(), sourcesAdded[env].end());
		sourcesAddedModified.insert(sourcesModified[env].begin(), sourcesModified[env].end());

		for (const string &moduleKey : sourcesAddedModified)
		{
			VuoDirectedAcyclicGraph::Vertex *vertex = env->getCompositionDependencyGraph()->findVertex(moduleKey);
			int pathLength = env->getCompositionDependencyGraph()->getLongestDownstreamPath(vertex);

			ModuleInfo *sourceInfo = env->listSourceFile(moduleKey);
			sourceInfo->setLongestDownstreamPath(pathLength);
		}
	}

	// Find all modules and sources that depend on the modules and sources being modified or removed.
	// Those that belong to one of this compiler's environments are used in subsequent stages.
	// Those that belong to some other environment are scheduled to be handled by a separate call to this function.

	map<Environment *, set<string> > modulesDepOnModulesModified;
	map<Environment *, set<string> > sourcesDepOnModulesModified;
	map<Environment *, set<string> > modulesDepOnModulesRemoved;
	map<Environment *, set<string> > sourcesDepOnModulesRemoved;

	{
		__block map<Environment *, set<string> > modulesDepOnModulesModified_otherCompiler;
		__block map<Environment *, set<string> > sourcesDepOnModulesModified_otherCompiler;
		__block map<Environment *, set<string> > modulesDepOnModulesRemoved_otherCompiler;
		__block map<Environment *, set<string> > sourcesDepOnModulesRemoved_otherCompiler;

		vector<VuoDirectedAcyclicNetwork *> searchDependencyGraphs;
		for (VuoCompiler *compiler : allCompilers)
			searchDependencyGraphs.push_back(compiler->dependencyGraph);

		VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph = (currentEnvironment ? currentEnvironment->getDependencyGraph() : nullptr);

		findDependentModulesAndSources(modulesModified, searchDependencyGraphs, currentEnvironmentDependencyGraph,
									   modulesDepOnModulesModified, modulesDepOnModulesModified_otherCompiler,
									   sourcesDepOnModulesModified, sourcesDepOnModulesModified_otherCompiler);

		findDependentModulesAndSources(modulesRemoved, searchDependencyGraphs, currentEnvironmentDependencyGraph,
									   modulesDepOnModulesRemoved, modulesDepOnModulesRemoved_otherCompiler,
									   sourcesDepOnModulesRemoved, sourcesDepOnModulesRemoved_otherCompiler);

		set<Environment *> otherEnvironments;
		for (map<Environment *, set<string> >::iterator i = modulesDepOnModulesModified_otherCompiler.begin(); i != modulesDepOnModulesModified_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);
		for (map<Environment *, set<string> >::iterator i = sourcesDepOnModulesModified_otherCompiler.begin(); i != sourcesDepOnModulesModified_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);
		for (map<Environment *, set<string> >::iterator i = modulesDepOnModulesRemoved_otherCompiler.begin(); i != modulesDepOnModulesRemoved_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);
		for (map<Environment *, set<string> >::iterator i = sourcesDepOnModulesRemoved_otherCompiler.begin(); i != sourcesDepOnModulesRemoved_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);

		for (Environment *env : otherEnvironments)
		{
			VuoCompiler *otherCompiler = nullptr;
			for (VuoCompiler *c : allCompilers)
				for (const vector<Environment *> &ee : c->environments)
					for (Environment *e : ee)
						if (e == env)
						{
							otherCompiler = c;
							goto foundOtherCompiler;
						}
			foundOtherCompiler:

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_sync(environmentQueue, ^{
					otherCompiler->loadModulesAndSources(set<string>(), modulesDepOnModulesModified_otherCompiler[env], modulesDepOnModulesRemoved_otherCompiler[env],
														 set<string>(), sourcesDepOnModulesModified_otherCompiler[env], sourcesDepOnModulesRemoved_otherCompiler[env],
														 false, true, env, nullptr, nullptr, "");
				});
			});
		}
	}

	// Unload:
	//   - modules that have been removed or modified
	//   - modules that depend on them

	map<Environment *, set<VuoCompilerModule *> > actualModulesRemoved;
	for (const vector<Environment *> &envs : environments)
	{
		for (Environment *env : envs)
		{
			set<string> modulesToUnload;
			modulesToUnload.insert(modulesRemoved[env].begin(), modulesRemoved[env].end());
			modulesToUnload.insert(modulesModified[env].begin(), modulesModified[env].end());
			modulesToUnload.insert(modulesDepOnModulesRemoved[env].begin(), modulesDepOnModulesRemoved[env].end());
			modulesToUnload.insert(modulesDepOnModulesModified[env].begin(), modulesDepOnModulesModified[env].end());

			actualModulesRemoved[env] = env->unloadCompiledModules(modulesToUnload);

			if (!env->isBuiltInOriginal() && !actualModulesRemoved[env].empty())
			{
				set<string> actualModulesRemovedKeys;
				for (auto m : actualModulesRemoved[env])
					actualModulesRemovedKeys.insert(m->getPseudoBase()->getModuleKey());
				VUserLog("Removed from %s environment: %s", env->getName().c_str(), VuoStringUtilities::join(actualModulesRemovedKeys, ", ").c_str());
			}
		}
	}

	// Load:
	//   - modules that have been added or modified
	//   - modules that they depend on
	//   - modules that depend on them that were just unloaded
	// Delete:
	//   - cached module files in `modulesToLoad` whose source files have been removed

	map<Environment *, set<string> > modulesToLoad;
	map<Environment *, map<string, string> > modulesToLoadSourceCode;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		if (! modulesAdded[env].empty())
			modulesToLoad[env].insert(modulesAdded[env].begin(), modulesAdded[env].end());
		if (! modulesModified[env].empty())
			modulesToLoad[env].insert(modulesModified[env].begin(), modulesModified[env].end());
		if (! modulesDepOnModulesModified[env].empty())
			modulesToLoad[env].insert(modulesDepOnModulesModified[env].begin(), modulesDepOnModulesModified[env].end());

		if (env == currentEnvironment && moduleLoadedCallback)
		{
			if (modulesAdded[env].size() == 1)
				modulesToLoadSourceCode[env][*modulesAdded[env].begin()] = moduleAddedOrModifiedSourceCode;
			else if (modulesModified[env].size() == 1)
				modulesToLoadSourceCode[env][*modulesModified[env].begin()] = moduleAddedOrModifiedSourceCode;
		}
	}

	map<Environment *, set<VuoCompilerModule *> > actualModulesAdded;
	while (! modulesToLoad.empty())
	{
		set<string> dependenciesToLoad;
		map<Environment *, set<string> > potentialSpecializedDependencies;
		for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
		{
			Environment *env = (*i).at(0);
			Environment *genEnv = (*i).at(1);

			set<VuoCompilerModule *> actualModulesLoaded = env->loadCompiledModules(modulesToLoad[env], modulesToLoadSourceCode[env]);

			actualModulesAdded[env].insert(actualModulesLoaded.begin(), actualModulesLoaded.end());
			modulesToLoad.erase(env);

			for (set<VuoCompilerModule *>::iterator j = actualModulesLoaded.begin(); j != actualModulesLoaded.end(); ++j)
			{
				set<string> dependencies = (*j)->getDependencies();
				dependenciesToLoad.insert(dependencies.begin(), dependencies.end());
				potentialSpecializedDependencies[genEnv].insert(dependencies.begin(), dependencies.end());
			}

			if (!env->isBuiltInOriginal() && !actualModulesLoaded.empty())
			{
				map<string, string> actualFilesAndHashesLoaded;
				for (auto module : actualModulesLoaded)
				{
					string path, hash;
					if (module->getPseudoBase()->getNodeSet())
						path = module->getPseudoBase()->getNodeSet()->getArchivePath();
					else
					{
						auto cnc = dynamic_cast<VuoCompilerNodeClass *>(module);
						if (cnc && !cnc->getSourcePath().empty())
						{
							path = cnc->getSourcePath();
							if (!cnc->getSourceCode().empty())
								// Use the latest source code, if it's been modified without saving.
								hash = VuoStringUtilities::calculateSHA256(cnc->getSourceCode());
						}
						else
							path = module->getModulePath();
					}

					if (hash.empty())
						try
						{
							hash = VuoFileUtilities::calculateFileSHA256(path);
						}
						catch (VuoException &e) {}

					actualFilesAndHashesLoaded[path] = hash;
				}

				for (pair<string, string> item : actualFilesAndHashesLoaded)
					VUserLog("Loaded into %s environment:  [%8.8s] %s", env->getName().c_str(), item.second.c_str(), item.first.c_str());
			}
		}

		for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
		{
			Environment *env = (*i).at(0);

			ModuleInfoIterator dependenciesInEnv = env->listModules(dependenciesToLoad);
			ModuleInfo *moduleInfo;
			while ((moduleInfo = dependenciesInEnv.next()))
			{
				modulesToLoad[env].insert( moduleInfo->getModuleKey() );

				for (map<Environment *, set<string> >::iterator j = potentialSpecializedDependencies.begin(); j != potentialSpecializedDependencies.end(); ++j)
				{
					auto foundIter = j->second.find( moduleInfo->getModuleKey() );
					if (foundIter != j->second.end())
						j->second.erase(foundIter);
				}
			}
		}

		for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
		{
			Environment *genEnv = (*i).at(1);
			potentialSpecializedModules[genEnv].insert(potentialSpecializedDependencies[genEnv].begin(), potentialSpecializedDependencies[genEnv].end());
		}
	}

	// Load asynchronously:
	//   - specializations of generic modules

	set<dispatch_group_t> specializedModulesLoading;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *genEnv = (*i).at(1);
		set<dispatch_group_t> s = genEnv->loadSpecializedModules(potentialSpecializedModules[genEnv], this, llvmQueue);
		specializedModulesLoading.insert(s.begin(), s.end());
	}

	// Notify those waiting on a source file to be compiled that its module has now been loaded.

	if (moduleLoadedCallback)
		moduleLoadedCallback();

	// Move modified modules from `actualModulesAdded` and `actualModulesRemoved` to `actualModulesModified`.

	map<Environment *, set< pair<VuoCompilerModule *, VuoCompilerModule *> > > actualModulesModified;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		for (set<VuoCompilerModule *>::iterator add = actualModulesAdded[env].begin(); add != actualModulesAdded[env].end(); )
		{
			set<VuoCompilerModule *>::iterator rem;
			for (rem = actualModulesRemoved[env].begin(); rem != actualModulesRemoved[env].end(); ++rem)
				if ((*rem)->getPseudoBase()->getModuleKey() == (*add)->getPseudoBase()->getModuleKey())
					break;

			if (rem != actualModulesRemoved[env].end())
			{
				actualModulesModified[env].insert( make_pair(*rem, *add) );
				actualModulesRemoved[env].erase(rem);
				actualModulesAdded[env].erase(add++);
			}
			else
				++add;
		}
	}

	// Reify port types on node classes (if needed).

	bool wereModulesAddedOrModified = false;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);
		if (! (actualModulesAdded[env].empty() && actualModulesModified[env].empty()) )
		{
			wereModulesAddedOrModified = true;
			break;
		}
	}

	if (wereModulesAddedOrModified)
	{
		map<string, VuoCompilerType *> inheritedTypes;
		for (const vector<Environment *> &envs : environments)
		{
			for (Environment *env : envs)
			{
				env->reifyPortTypes(inheritedTypes);
				map<string, VuoCompilerType *> envTypes = env->getTypes();
				inheritedTypes.insert(envTypes.begin(), envTypes.end());
			}
		}
	}

	// Delete cached compiled module files and call this function recursively for:
	//   - modules whose source files have been removed
	//   - modules whose source files depend on them

	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		set<string> sourcesToUnload;
		sourcesToUnload.insert(sourcesRemoved[env].begin(), sourcesRemoved[env].end());
		sourcesToUnload.insert(sourcesDepOnModulesRemoved[env].begin(), sourcesDepOnModulesRemoved[env].end());
		if (! sourcesToUnload.empty())
		{
			string moduleSearchPath = env->getModuleSearchPaths().front();

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				VuoCompiler *otherCompiler = new VuoCompiler(moduleSearchPath + "/unused", target);

				dispatch_sync(environmentQueue, ^{
					otherCompiler->loadModulesAndSources(set<string>(), set<string>(), sourcesToUnload,
														 set<string>(), set<string>(), set<string>(),
														 false, false, env, nullptr, nullptr, "");
				});

				delete otherCompiler;
			});
		}

		if (!env->isBuiltInOriginal() && !sourcesToUnload.empty())
			VUserLog("Deleting from %s environment: %s", env->getName().c_str(), VuoStringUtilities::join(sourcesToUnload, ", ").c_str());

		env->deleteModulesCompiledFromSourceCode(sourcesToUnload);
	}

	// Compile asynchronously:
	//   - source files that have been added or modified
	//   - source files that depend on them
	//   - source files that depend on modules that have been modified

	map<Environment *, set<string> > sourcesDepOnModulesAdded;
	{
		map<Environment *, set<string> > modulesDepOnModulesAdded;                        // unused
		__block map<Environment *, set<string> > modulesDepOnModulesAdded_otherCompiler;  //
		__block map<Environment *, set<string> > sourcesDepOnModulesAdded_otherCompiler;

		map<Environment *, set<string> > actualModuleKeysAdded;
		for (const vector<Environment *> &envs : environments)
		{
			Environment *env = envs.at(0);
			for (VuoCompilerModule *module : actualModulesAdded[env])
				actualModuleKeysAdded[env].insert( module->getPseudoBase()->getModuleKey() );
		}

		vector<VuoDirectedAcyclicNetwork *> searchDependencyGraphs;
		searchDependencyGraphs.push_back(compositionDependencyGraph);
		for (map<string, vector<Environment *> >::iterator ii = environmentsForCompositionFamily[target].begin(); ii != environmentsForCompositionFamily[target].end(); ++ii)
		{
			vector< vector<Environment *> > otherEnvs = sharedEnvironments[target];
			otherEnvs.push_back(ii->second);
			VuoDirectedAcyclicNetwork *other = makeDependencyNetwork(otherEnvs, ^VuoDirectedAcyclicGraph * (Environment *env) { return env->getCompositionDependencyGraph(); });
			searchDependencyGraphs.push_back(other);
		}

		VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph = (currentEnvironment ? currentEnvironment->getCompositionDependencyGraph() : nullptr);

		findDependentModulesAndSources(actualModuleKeysAdded, searchDependencyGraphs, currentEnvironmentDependencyGraph,
									   modulesDepOnModulesAdded, modulesDepOnModulesAdded_otherCompiler,
									   sourcesDepOnModulesAdded, sourcesDepOnModulesAdded_otherCompiler);

		set<Environment *> otherEnvironments;
		for (map<Environment *, set<string> >::iterator i = sourcesDepOnModulesAdded_otherCompiler.begin(); i != sourcesDepOnModulesAdded_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);

		for (Environment *env : otherEnvironments)
		{
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				string moduleSearchPath = env->getModuleSearchPaths().front();
				VuoCompiler *otherCompiler = new VuoCompiler(moduleSearchPath + "/unused", target);

				dispatch_sync(environmentQueue, ^{
					otherCompiler->loadModulesAndSources(set<string>(), set<string>(), set<string>(),
														 sourcesDepOnModulesAdded_otherCompiler[env], set<string>(), set<string>(),
														 false, true, env, nullptr, nullptr, "");
				});

				delete otherCompiler;
			});
		}
	}

	set<dispatch_group_t> sourcesLoading;
	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		set<string> sourcesToCompile;
		sourcesToCompile.insert(sourcesAdded[env].begin(), sourcesAdded[env].end());
		sourcesToCompile.insert(sourcesModified[env].begin(), sourcesModified[env].end());

		if (sourcesToCompile.size() == 0)
			continue;

		set<dispatch_group_t> s = env->compileModulesFromSourceCode(sourcesToCompile, moduleSourceCompilersExist, shouldRecompileSourcesIfUnchanged);
		sourcesLoading.insert(s.begin(), s.end());
	}

	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);

		set<string> sourcesToCompile;
		sourcesToCompile.insert(sourcesDepOnModulesAdded[env].begin(), sourcesDepOnModulesAdded[env].end());
		sourcesToCompile.insert(sourcesDepOnModulesModified[env].begin(), sourcesDepOnModulesModified[env].end());

		if (sourcesToCompile.size() == 0)
			continue;

		env->compileModulesFromSourceCode(sourcesToCompile, moduleSourceCompilersExist, true);
	}

	// Notify compiler delegates.

	for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		Environment *env = (*i).at(0);
		env->notifyCompilers(actualModulesAdded[env], actualModulesModified[env], actualModulesRemoved[env], issues[env]);
	}

	// Since the dispatch groups for specialized modules are temporary (caller is responsible for releasing them)
	// but the dispatch groups for module sources should stay alive as long as the ModuleInfo that contains them,
	// retain the dispatch groups for module sources so that all dispatch groups can be safely released by the caller.

	for (const dispatch_group_t &group : sourcesLoading)
		dispatch_retain(group);

	set<dispatch_group_t> loadingGroups;
	loadingGroups.insert(specializedModulesLoading.begin(), specializedModulesLoading.end());
	loadingGroups.insert(sourcesLoading.begin(), sourcesLoading.end());
	return loadingGroups;
}

/**
 * Helper function for VuoCompiler::loadModulesAndSources. Finds modules and sources that depend on @a changedModules.
 *
 * @param changedModules A set of modules being added, modified, or removed.
 * @param searchDependencyGraphs The dependency graphs to search for dependencies.
 * @param currentEnvironmentDependencyGraph If working within `currentEnvironment`, the dependency graph for that environment.
 * @param[out] modulesDepOnChangedModules_this Dependent modules that belong to this VuoCompiler.
 * @param[out] modulesDepOnChangedModules_other Dependent modules that belong to another VuoCompiler.
 * @param[out] sourcesDepOnChangedModules_this Dependent sources that belong to this VuoCompiler.
 * @param[out] sourcesDepOnChangedModules_other Dependent sources that belong to another VuoCompiler.
 */
void VuoCompiler::findDependentModulesAndSources(map<Environment *, set<string> > &changedModules,
												 const vector<VuoDirectedAcyclicNetwork *> &searchDependencyGraphs,
												 VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph,
												 map<Environment *, set<string> > &modulesDepOnChangedModules_this,
												 map<Environment *, set<string> > &modulesDepOnChangedModules_other,
												 map<Environment *, set<string> > &sourcesDepOnChangedModules_this,
												 map<Environment *, set<string> > &sourcesDepOnChangedModules_other)
{
	for (const vector<Environment *> &envs : environments)
	{
		Environment *env = envs.at(0);

		for (const string &module : changedModules[env])
		{
			set<VuoDirectedAcyclicGraph::Vertex *> dependents;
			for (VuoDirectedAcyclicNetwork *searchDependencyGraph : searchDependencyGraphs)
			{
				vector<VuoDirectedAcyclicGraph::Vertex *> moduleVertices;
				if (currentEnvironmentDependencyGraph)
				{
					// If a module with the same module key is installed in multiple locations,
					// only consider the one being modified or removed.
					VuoDirectedAcyclicGraph::Vertex *mv = currentEnvironmentDependencyGraph->findVertex(module);
					if (mv)
						moduleVertices.push_back(mv);
				}
				else
					moduleVertices = searchDependencyGraph->findVertex(module);

				for (VuoDirectedAcyclicGraph::Vertex *moduleVertexRaw : moduleVertices)
				{
					DependencyGraphVertex *moduleVertex = static_cast<DependencyGraphVertex *>(moduleVertexRaw);
					if (moduleVertex->getEnvironment())
					{
						vector<VuoDirectedAcyclicGraph::Vertex *> upstream = searchDependencyGraph->getUpstreamVertices(moduleVertex);
						dependents.insert(upstream.begin(), upstream.end());
					}
				}
			}

			set< pair<Environment *, string> > dependentsMap;
			for (VuoDirectedAcyclicGraph::Vertex *dependentVertexRaw : dependents)
			{
				DependencyGraphVertex *v = static_cast<DependencyGraphVertex *>(dependentVertexRaw);
				Environment *dependentEnv = v->getEnvironment();
				if (! dependentEnv)
					continue;

				string dependent = v->getDependency();

				dependentsMap.insert({dependentEnv, dependent});
			}

			// In case `module` is a generic node class, check the generated environment at the same scope for any
			// specializations of the node class, and add them to the list of dependencies.
			// (They aren't in the dependency graph since the graph edge goes from installed to generated.)
			for (auto i : envs.at(1)->getNodeClasses())
			{
				VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(i.second);
				if (specializedNodeClass->getOriginalGenericNodeClassName() == module)
					dependentsMap.insert({envs.at(1), i.first});
			}

			for (auto i : dependentsMap)
			{
				Environment *dependentEnv = i.first;
				string dependent = i.second;

				// Skip if the dependent module is already being modified/removed in its own right
				// (e.g. if the module depends on another in the same node set and the node set is being removed).
				if (changedModules[dependentEnv].find(dependent) != changedModules[dependentEnv].end())
					continue;

				ModuleInfo *foundSourceInfo = dependentEnv->listSourceFile(dependent);
				ModuleInfo *foundModuleInfo = dependentEnv->listModule(dependent);

				bool belongsToCurrentCompiler = false;
				for (const vector<Environment *> &envs2 : environments)
				{
					if (find(envs2.begin(), envs2.end(), dependentEnv) != envs2.end())
					{
						belongsToCurrentCompiler = true;
						break;
					}
				}

				map<Environment *, set<string> > *whicheverDependents = nullptr;
				ModuleInfo *moduleInfo = nullptr;
				if (foundSourceInfo)
				{
					moduleInfo = foundSourceInfo;
					whicheverDependents = (belongsToCurrentCompiler ? &sourcesDepOnChangedModules_this : &sourcesDepOnChangedModules_other);
				}
				else if (foundModuleInfo)
				{
					moduleInfo = foundModuleInfo;
					whicheverDependents = (belongsToCurrentCompiler ? &modulesDepOnChangedModules_this : &modulesDepOnChangedModules_other);
				}
				else  // Module in generated environment
				{
					whicheverDependents = (belongsToCurrentCompiler ? &modulesDepOnChangedModules_this : &modulesDepOnChangedModules_other);
				}

				(*whicheverDependents)[dependentEnv].insert(dependent);
				if (moduleInfo)
					moduleInfo->setAttempted(false);
			}
		}
	}
}

/**
 * Notifies the delegate (if any) of modules loaded/unloaded.
 */
void VuoCompiler::loadedModules(map<string, VuoCompilerModule *> modulesAdded,
								map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModified,
								map<string, VuoCompilerModule *> modulesRemoved,
								VuoCompilerIssues *issues, void *delegateDataV, Environment *currentEnvironment)
{
	//VLog("C=%p                   %lu %lu %lu", this, modulesAdded.size(), modulesModified.size(), modulesRemoved.size());

	// If a module is added, superseding a version of the same module installed at a broader scope,
	// notify this VuoCompiler that the module has been modified rather than added.
	//
	// If a module is added, but a version of the same module is already installed at a narrower scope,
	// don't notify this VuoCompiler, since it will continue to use the version at the narrower scope.
	//
	// Same idea when a module is modified or removed while another version is installed at a different scope.

	auto findVersionsOfModule = [this, currentEnvironment] (const string &moduleKey)
	{
		vector< pair<Environment *, VuoCompilerModule *> > moduleVersions;
		for (const vector<Environment *> &envs : environments)
		{
			Environment *env = envs.at(0);
			VuoCompilerModule *module = env->findModule(moduleKey);
			if (module || env == currentEnvironment)
				moduleVersions.push_back( make_pair(env, module) );
		}
		return moduleVersions;
	};

	for (map<string, VuoCompilerModule *>::iterator i = modulesAdded.begin(); i != modulesAdded.end(); )
	{
		string moduleKey = i->first;
		VuoCompilerModule *moduleAdded = i->second;

		vector< pair<Environment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

		if (moduleVersions.size() > 1)
		{
			modulesAdded.erase(i++);

			if (moduleVersions.back().second == moduleAdded)
			{
				VuoCompilerModule *moduleSuperseded = moduleVersions.at(moduleVersions.size()-2).second;
				modulesModified[moduleKey] = make_pair(moduleSuperseded, moduleAdded);
			}
		}
		else
			++i;
	}

	for (map<string, pair<VuoCompilerModule *, VuoCompilerModule *> >::iterator i = modulesModified.begin(); i != modulesModified.end(); )
	{
		string moduleKey = i->first;
		VuoCompilerModule *moduleModified = i->second.second;

		vector< pair<Environment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

		if (moduleVersions.size() > 1 && moduleVersions.back().second != moduleModified)
			modulesModified.erase(i++);
		else
			++i;
	}

	for (map<string, VuoCompilerModule *>::iterator i = modulesRemoved.begin(); i != modulesRemoved.end(); )
	{
		string moduleKey = i->first;
		VuoCompilerModule *moduleRemoved = i->second;

		vector< pair<Environment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

		if (moduleVersions.size() > 1)
		{
			modulesRemoved.erase(i++);

			if (moduleVersions.back().first == currentEnvironment)
			{
				VuoCompilerModule *moduleUnsuperseded = moduleVersions.at(moduleVersions.size()-2).second;
				modulesModified[moduleKey] = make_pair(moduleRemoved, moduleUnsuperseded);
			}
		}
		else
			++i;
	}

	dispatch_async(delegateQueue, ^{
					   VuoCompilerDelegate::LoadedModulesData *delegateData = static_cast<VuoCompilerDelegate::LoadedModulesData *>(delegateDataV);

					   if (delegate && ! (modulesAdded.empty() && modulesModified.empty() && modulesRemoved.empty() && issues->isEmpty()))
					   {
						   delegate->enqueueData(delegateData);
						   delegate->loadedModules(modulesAdded, modulesModified, modulesRemoved, issues);
					   }
					   else
					   {
						   delegateData->release();
					   }
				   });
}

/**
 * Loads a node class that lives in memory instead of in a file.
 *
 * @threadQueue{environmentQueue}
 */
void VuoCompiler::loadNodeClassGeneratedAtRuntime(VuoCompilerNodeClass *nodeClass, Environment *env)
{
	Module *module = nodeClass->getModule();
	if (module)
	{
		dispatch_sync(llvmQueue, ^{
			setTargetForModule(nodeClass->getModule(), env->getTarget());
		});
	}

	dispatch_sync(environmentQueue, ^{
		env->replaceNodeClass(nodeClass);
	});

	__block map<string, VuoCompilerType *> inheritedTypes;
	void (^envReifyPortTypes)(Environment *) = ^void (Environment *env) {
		env->reifyPortTypes(inheritedTypes);
		map<string, VuoCompilerType *> currentTypes = env->getTypes();
		inheritedTypes.insert(currentTypes.begin(), currentTypes.end());
	};
	applyToAllEnvironments(envReifyPortTypes);
}

/**
 * Updates the nodes and ports of the composition to have the correct backing types for generic types.
 */
void VuoCompiler::reifyGenericPortTypes(VuoCompilerComposition *composition)
{
	for (VuoCompilerNode *node : composition->getCachedGraph(this)->getNodes())
		reifyGenericPortTypes(node->getBase());

	composition->invalidateCachedGraph();
}

/**
 * Updates the ports of the node to have the correct backing types for generic types.
 */
void VuoCompiler::reifyGenericPortTypes(VuoNode *node)
{
	VuoCompilerSpecializedNodeClass *nodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node->getNodeClass()->getCompiler());
	if (! nodeClass)
		return;

	// Reify any generic types on the node that don't already have a compiler detail.

	vector<VuoPort *> inputPorts = node->getInputPorts();
	vector<VuoPort *> outputPorts = node->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

	for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
	{
		VuoCompilerPort *port = static_cast<VuoCompilerPort *>((*j)->getCompiler());
		VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(port->getDataVuoType());
		if (! genericType)
			continue;

		if (! genericType->hasCompiler())
		{
			VuoCompilerType * (^compilerGetType)(string) = ^VuoCompilerType * (string moduleKey) {
				return getType(moduleKey);
			};

			VuoCompilerGenericType *reifiedType = VuoCompilerGenericType::newGenericType(genericType, compilerGetType);
			if (reifiedType)
				port->setDataVuoType(reifiedType->getBase());
		}
	}

	// Update the node class's backing to match the node's backing.

	nodeClass->updateBackingNodeClass(node, this);
}

/**
 * Compiles a node class, port type, or library module to LLVM bitcode.
 *
 * @param inputPath The file to compile, containing a C implementation of the node class, port type, or library module.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 */
void VuoCompiler::compileModule(string inputPath, string outputPath)
{
	compileModule(inputPath, outputPath, vector<string>());
}

/**
 * Compiles a node class, port type, or library module to LLVM bitcode.
 *
 * @param inputPath The file to compile, containing a C implementation of the node class, port type, or library module.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 * @param includePaths Directories with header files to be included when compiling.
 */
void VuoCompiler::compileModule(string inputPath, string outputPath, const vector<string> &includePaths)
{
	if (isVerbose)
		print();

	vector<string> allIncludePaths = includePaths;
	string preprocessedInputPath = inputPath;

	string tmpPreprocessedInputDir;
	string dir, file, ext;
	VuoFileUtilities::splitPath(inputPath, dir, file, ext);
	if (VuoFileUtilities::isCSourceExtension(ext))
	{
		string inputContents = VuoFileUtilities::readFileToString(inputPath);
		string preprocessedInputContents = inputContents;
		VuoCompilerSpecializedNodeClass::replaceGenericTypesWithBacking(preprocessedInputContents);
		if (inputContents != preprocessedInputContents)
		{
			// Unspecialized generic node class
			allIncludePaths.push_back(dir.empty() ? "." : dir);
			tmpPreprocessedInputDir = VuoFileUtilities::makeTmpDir(file);
			preprocessedInputPath = tmpPreprocessedInputDir + "/" + file + "." + ext;
			VuoFileUtilities::preserveOriginalFileName(preprocessedInputContents, file + "." + ext);
			VuoFileUtilities::writeStringToFile(preprocessedInputContents, preprocessedInputPath);
		}
	}
	else if (ext == "fs")
	{
		VuoFileUtilities::File vuf(dir, file + "." + ext);
		VuoModuleCompiler *moduleCompiler = VuoModuleCompiler::newModuleCompiler("isf", getModuleKeyForPath(inputPath), &vuf);
		if (moduleCompiler)
		{
			auto getType = [this] (const string &moduleKey) { return this->getType(moduleKey); };
			VuoCompilerIssues *issues = new VuoCompilerIssues();
			Module *module = moduleCompiler->compile(getType, llvmQueue, issues);
			if (module)
				dispatch_sync(llvmQueue, ^{
					setTargetForModule(module, target);
					writeModuleToBitcode(module, outputPath);
#if VUO_PRO
					string dependencyOutputPath = _dependencyOutput();
					if (!dependencyOutputPath.empty())
					{
						string outputObjectPath = dependencyOutputPath.substr(0, dependencyOutputPath.length() - 2);
						VuoFileUtilities::writeStringToFile(outputObjectPath + ": " + inputPath, dependencyOutputPath);
					}
#endif
				});

			if (!issues->isEmpty())
				throw VuoCompilerException(issues, true);
			delete issues;
		}
		return;
	}

	vector<string> extraArgs;
	for (vector<string>::iterator i = allIncludePaths.begin(); i != allIncludePaths.end(); ++i)
	{
		extraArgs.push_back("-I");
		extraArgs.push_back(*i);
	}


	// When compiling on a development workstation or Jenkins, use the Conan-packaged macOS SDK, since it includes headers.
	// When compiling on an end-user system, no SDK is needed.
	string buildTimeMacOSSDKFolder = MACOS_SDK_ROOT;
	if (VuoFileUtilities::fileExists(buildTimeMacOSSDKFolder))
	{
		extraArgs.push_back("-isysroot");
		extraArgs.push_back(buildTimeMacOSSDKFolder);
	}


	__block vector<string> headerSearchPaths;
	void (^envGetHeaderSearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getHeaderSearchPaths();
		headerSearchPaths.insert(headerSearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetHeaderSearchPaths);

	auto issues = new VuoCompilerIssues;
	__block Module *module;
	dispatch_sync(llvmQueue, ^{
					  module = readModuleFromC(preprocessedInputPath, headerSearchPaths, extraArgs, issues);
				  });
	string moduleKey = getModuleKeyForPath(inputPath);
	if (! tmpPreprocessedInputDir.empty())
	{
		remove(tmpPreprocessedInputDir.c_str());
		issues->setFilePath(inputPath);
	}
	if (! module)
		throw VuoCompilerException(issues, true);
	delete issues;

	dispatch_sync(llvmQueue, ^{
					  VuoCompilerModule *compilerModule = VuoCompilerModule::newModule(moduleKey, module, "", VuoCompilerCompatibility::compatibilityWithAnySystem());
					  if (! compilerModule)
					  {
						  VUserLog("Error: Didn't recognize '%s' as a node class, type, or library.", inputPath.c_str());
						  return;
					  }

					  setTargetForModule(module, target);
					  writeModuleToBitcode(module, outputPath);

					  delete module;
				  });
}

/**
 * Compiles a composition to LLVM bitcode (without writing it to file).
 */
Module * VuoCompiler::compileCompositionToModule(VuoCompilerComposition *composition, const string &moduleKey, bool isTopLevelComposition,
												 VuoCompilerIssues *issues)
{
	composition->check(issues);

	reifyGenericPortTypes(composition);

	VuoCompilerBitcodeGenerator *generator = VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(composition,
																											 isTopLevelComposition,
																											 moduleKey, this);

	__block Module *module = nullptr;
	dispatch_sync(llvmQueue, ^{
		try
		{
			module = generator->generateBitcode();
			setTargetForModule(module, target);
		}
		catch (VuoCompilerException &e)
		{
			if (issues)
				issues->append(e.getIssues());
			else
				VUserLog("%s", e.getIssues()->getLongDescription(false).c_str());
		}
	});

	delete generator;

	return module;
}

/**
 * Compiles a composition to LLVM bitcode.
 *
 * @param composition The composition to compile.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 * @param isTopLevelComposition True if the composition is top-level, false if it's a subcomposition.
 * @param issues Issues encountered while compiling the composition are appended to this.
 * @throw VuoCompilerException The composition is invalid.
 * @version200New
 */
void VuoCompiler::compileComposition(VuoCompilerComposition *composition, string outputPath, bool isTopLevelComposition,
									 VuoCompilerIssues *issues)
{
	string moduleKey = getModuleKeyForPath(outputPath);
	Module *module = compileCompositionToModule(composition, moduleKey, isTopLevelComposition, issues);
	if (!module)
		throw VuoCompilerException(issues, false);

	dispatch_sync(llvmQueue, ^{
					  writeModuleToBitcode(module, outputPath);
				  });
}

/**
 * Compiles a composition, read from file, to LLVM bitcode.
 *
 * @param inputPath The .vuo file containing the composition. If you haven't already specified the
 *     composition path in the constructor or @ref VuoCompiler::setCompositionPath, then @a inputPath will be used
 *     to locate composition-local modules.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 * @param isTopLevelComposition True if the composition is top-level, false if it's a subcomposition.
 * @param issues Issues encountered while compiling the composition are appended to this.
 * @throw VuoCompilerException The composition is invalid.
 * @version200New
 */
void VuoCompiler::compileComposition(string inputPath, string outputPath, bool isTopLevelComposition,
									 VuoCompilerIssues *issues)
{
	VUserLog("Compiling '%s' (%s)…", inputPath.c_str(), target.c_str());
	if (isVerbose)
		print();

	if (getCompositionLocalPath().empty())
		setCompositionPath(inputPath);

	if (!VuoFileUtilities::fileContainsReadableData(inputPath))
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "opening composition", inputPath,
							   "", "The composition file couldn't be read or was empty.");
		throw VuoCompilerException(issue);
	}

	try
	{
		string compositionString = VuoFileUtilities::readFileToString(inputPath);
		compileCompositionString(compositionString, outputPath, isTopLevelComposition, issues);
	}
	catch (VuoCompilerException &e)
	{
		if (e.getIssues())
			e.getIssues()->setFilePathIfEmpty(inputPath);
		if (!issues && e.getIssues())
			VUserLog("%s", e.getIssues()->getLongDescription(false).c_str());
		throw;
	}

	VUserLog("Done.");
}

/**
 * Compiles the composition, read from a string, to LLVM bitcode.
 *
 * @param compositionString A string containing the composition.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 * @param isTopLevelComposition True if the composition is top-level, false if it's a subcomposition.
 * @param issues Issues encountered while compiling the composition are appended to this.
 * @throw VuoCompilerException The composition is invalid.
 * @version200New
 */
void VuoCompiler::compileCompositionString(const string &compositionString, string outputPath, bool isTopLevelComposition,
										   VuoCompilerIssues *issues)
{
	VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, this);
	compileComposition(composition, outputPath, isTopLevelComposition, issues);

	VuoComposition *baseComposition = composition->getBase();
	delete composition;
	delete baseComposition;
}

/**
 * Turns a compiled composition into an executable by
 * linking in all of its dependencies and adding a main function.
 *
 * @param inputPath Path to the compiled composition (an LLVM bitcode file).
 * @param outputPath Path where the resulting executable should be placed.
 * @param optimization Controls the time it takes to link the composition and the size and dependencies of the
 *			resulting executable.
 * @param rPath An optional @c -rpath argument to be passed to `ld`. If empty, defaults to all run-path search paths available
 *          to this compiler's environments.
 * @param shouldAdHocCodeSign  Whether to ad-hoc code-sign the generated executable.  Disable to improve performance (e.g., for systems that don't require code-signing, or if you will later be code-signing the whole bundle).
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition,
 *			or the linker encountered errors and failed.
 * @version200Changed{Removed `isApp` argument.}
 */
void VuoCompiler::linkCompositionToCreateExecutable(string inputPath, string outputPath, Optimization optimization, string rPath, bool shouldAdHocCodeSign)
{
	vector<string> rPaths = ! rPath.empty() ? vector<string>(1, rPath) : getRunPathSearchPaths(environments.back().front());
	linkCompositionToCreateExecutableOrDynamicLibrary(inputPath, outputPath, optimization, false, rPaths, shouldAdHocCodeSign);
}

/**
 * Turns a compiled composition into a dynamic library by
 * linking in all of its dependencies.
 *
 * If you plan to run multiple compositions, or multiple instances of the same composition, in the same process
 * (@ref VuoRunner::newCurrentProcessRunnerFromDynamicLibrary), @a optimization should be `Optimization_FastBuild`
 * or `Optimization_FastBuildExistingCache`. This prevents conflicts between Objective-C classes (from nodes)
 * being defined in multiple loaded dynamic libraries.
 *
 * @param inputPath Path to the compiled composition (an LLVM bitcode file).
 * @param outputPath Path where the resulting dynamic library should be placed.
 * @param optimization Controls the time it takes to link the composition and the size and dependencies of the
 *			resulting dynamic library.
 * @param shouldAdHocCodeSign  Whether to ad-hoc code-sign the generated dylib.  Disable to improve performance (e.g., for systems that don't require code-signing, or if you will later be code-signing the whole bundle).
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition,
 *			or the linker encountered errors and failed.
 */
void VuoCompiler::linkCompositionToCreateDynamicLibrary(string inputPath, string outputPath, Optimization optimization, bool shouldAdHocCodeSign)
{
	vector<string> rPaths = getRunPathSearchPaths(environments.back().front());
	linkCompositionToCreateExecutableOrDynamicLibrary(inputPath, outputPath, optimization, true, rPaths, shouldAdHocCodeSign);
}

/**
 * Creates an executable or dynamic library that contains the composition and its dependencies.
 *
 * If creating an executable, a main function is added.
 *
 * @param compiledCompositionPath Path to the compiled composition (n LLVM bitcode file).
 * @param linkedCompositionPath Path where the resulting executable or dynamic library should be placed.
 * @param optimization Controls the time it takes to link the composition and the size and dependencies of the
 *			resulting executable or dynamic library.
 * @param isDylib True if creating a dynamic library, false if creating an executable.
 * @param rPaths The `-rpath` arguments to be passed to `ld`.
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition,
 *			or the linker encountered errors and failed.
 */
void VuoCompiler::linkCompositionToCreateExecutableOrDynamicLibrary(string compiledCompositionPath, string linkedCompositionPath,
																	Optimization optimization, bool isDylib, const vector<string> &rPaths,
																	bool shouldAdHocCodeSign)
{
	if (isVerbose)
		print();

	if (optimization == Optimization_FastBuildExistingCache)
		shouldLoadAllModules = false;

	set<string> dependencies = getDependenciesForComposition(compiledCompositionPath);
	dependencies.insert(getRuntimeDependency());
	if (! isDylib)
		dependencies.insert(getRuntimeMainDependency());

	set<Module *> modules;
	set<string> libraries;
	set<string> frameworks;
	getLinkerInputs(dependencies, optimization, modules, libraries, frameworks);

	libraries.insert(compiledCompositionPath);

	link(linkedCompositionPath, modules, libraries, frameworks, isDylib, rPaths, shouldAdHocCodeSign);
}

/**
 * Creates one dynamic library for the composition by itself and, if needed, additional dynamic libraries for the
 * node classes and other resources that are dependencies of the composition.
 *
 * @param compiledCompositionPath Path to the compiled composition (an LLVM bitcode file).
 * @param linkedCompositionPath Path where the resulting dynamic library for the composition should be placed.
 *     Pass a different path each time this function is called (to ensure that the OS will agree to swap out the library
 *     during live coding).
 * @param runningCompositionLibraries Information about libraries referenced by the composition. On the first call to this function,
 *     pass in a newly constructed object. On subsequent calls, pass in the same object. It gets updated with each call.
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition,
 *     or the linker encountered errors and failed. @a runningCompositionLibraries is unchanged.
 * @version200Changed{Replaced `newLinkedResourcePath`, `alreadyLinkedResourcePaths`, `alreadyLinkedResources` arguments with `runningCompositionLibraries`.}
 */
void VuoCompiler::linkCompositionToCreateDynamicLibraries(string compiledCompositionPath, string linkedCompositionPath,
														  VuoRunningCompositionLibraries *runningCompositionLibraries)
{
	if (isVerbose)
		print();

	bool shouldAdHocCodeSign = false;
#if __arm64__
	shouldAdHocCodeSign = true;
#endif

	// Get the dependencies used by the new resources and not the previous resources.

	set<string> carriedOverDependencies = runningCompositionLibraries->getDependenciesLoaded();
	set<string> allDependencies = getDependenciesForComposition(compiledCompositionPath);
	set<string> addedDependencies;
	std::set_difference(allDependencies.begin(), allDependencies.end(),
						carriedOverDependencies.begin(), carriedOverDependencies.end(),
						std::inserter(addedDependencies, addedDependencies.end()));

	// Get the libraries and frameworks used by the previous resources.

	vector<string> carriedOverNonUnloadableLibraries = runningCompositionLibraries->getNonUnloadableLibrariesLoaded();
	vector<string> carriedOverUnloadableLibraries = runningCompositionLibraries->getUnloadableLibrariesLoaded();
	set<string> carriedOverExternalLibraries = runningCompositionLibraries->getExternalLibraries();
	set<string> carriedOverFrameworks = runningCompositionLibraries->getExternalFrameworks();

	// Link the new resource dylibs, if needed.

	string nonUnloadableResourcePath;
	string unloadableResourcePath;
	set<string> nonUnloadableDependencies;
	set<string> unloadableDependencies;
	map<string, set<string> > builtInCacheDependencies;
	map<string, set<string> > userCacheDependencies;
	set<string> builtInLibraries;
	set<string> userLibraries;
	set<string> addedExternalLibraries;
	set<string> addedFrameworks;
	set<string> allFrameworks;
	if (! addedDependencies.empty())
	{
		// Get the modules, libraries, and frameworks that will provide the composition's dependencies.

		set<string> builtInModuleAndLibraryDependencies;
		set<string> userModuleAndLibraryDependencies;
		set<Module *> builtInModules;
		set<Module *> userModules;

		getLinkerInputs(addedDependencies, Optimization_FastBuild,
						builtInModuleAndLibraryDependencies, userModuleAndLibraryDependencies, builtInCacheDependencies, userCacheDependencies,
						builtInModules, userModules, builtInLibraries, userLibraries, addedExternalLibraries, addedFrameworks);

		allFrameworks.insert(carriedOverFrameworks.begin(), carriedOverFrameworks.end());
		allFrameworks.insert(addedFrameworks.begin(), addedFrameworks.end());

		string dir, linkedCompositionFile, ext;
		VuoFileUtilities::splitPath(linkedCompositionPath, dir, linkedCompositionFile, ext);

		// For any module caches that were rebuilt, remove the previous revision from the lists of libraries to link to and load.

		vector<string> carriedOverUserCacheLibraries = runningCompositionLibraries->getUnloadableCacheLibrariesLoaded();

		__block vector<string> currentCacheLibraries;
		applyToAllEnvironments(^void (Environment *env) {
			currentCacheLibraries.push_back( env->getCurrentModuleCacheDylib() );
		});

		for (string cachePath : carriedOverUserCacheLibraries)
		{
			for (string currentCachePath : currentCacheLibraries)
			{
				if (VuoFileUtilities::areDifferentRevisionsOfSameModuleCacheDylib(cachePath, currentCachePath))
				{
					set<string> dependenciesInCache = runningCompositionLibraries->enqueueCacheLibraryToUnload(cachePath);

					userCacheDependencies[currentCachePath].insert(dependenciesInCache.begin(), dependenciesInCache.end());

					auto cacheDependenciesIter = userCacheDependencies.find(cachePath);
					if (cacheDependenciesIter != userCacheDependencies.end())
					{
						userCacheDependencies[currentCachePath].insert(cacheDependenciesIter->second.begin(), cacheDependenciesIter->second.end());
						userCacheDependencies.erase(cacheDependenciesIter);
					}

					auto carriedOverIter = find(carriedOverUnloadableLibraries.begin(), carriedOverUnloadableLibraries.end(), cachePath);
					if (carriedOverIter != carriedOverUnloadableLibraries.end())
						*carriedOverIter = currentCachePath;
				}
			}
		}

		// If any module caches were rebuilt, prepare to replace the existing user resource dylibs with the new resource dylib created below.

		bool wasModuleCacheRebuilt = runningCompositionLibraries->hasCacheLibraryEnqueuedToUnload();
		if (wasModuleCacheRebuilt)
		{
			vector<string> carriedOverResourceLibraries = runningCompositionLibraries->getUnloadableResourceLibrariesLoaded();

			vector<string> carriedOverUnloadableMinusResourceLibraries;
			std::set_difference(carriedOverUnloadableLibraries.begin(), carriedOverUnloadableLibraries.end(),
								carriedOverResourceLibraries.begin(), carriedOverResourceLibraries.end(),
								std::back_inserter(carriedOverUnloadableMinusResourceLibraries));

			carriedOverUnloadableLibraries = carriedOverUnloadableMinusResourceLibraries;

			set<string> dependenciesInResourceLibraries = runningCompositionLibraries->enqueueAllUnloadableResourceLibrariesToUnload();
			userModuleAndLibraryDependencies.insert(dependenciesInResourceLibraries.begin(), dependenciesInResourceLibraries.end());

			set<string> builtInModuleAndLibraryDependencies_tmp;
			set<string> userModuleAndLibraryDependencies_tmp;
			map<string, set<string> > builtInCacheDependencies_tmp;
			set<Module *> builtInModules_tmp;
			set<string> builtInLibraries_tmp;
			set<string> externalLibraries_tmp;
			set<string> externalFrameworks_tmp;

			getLinkerInputs(userModuleAndLibraryDependencies, Optimization_FastBuild,
							builtInModuleAndLibraryDependencies_tmp, userModuleAndLibraryDependencies_tmp, builtInCacheDependencies_tmp, userCacheDependencies,
							builtInModules_tmp, userModules, builtInLibraries_tmp, userLibraries, externalLibraries_tmp, externalFrameworks_tmp);
		}

		// If built-in dependencies were added, create an additional resource dylib.

		if (! builtInModules.empty() || builtInLibraries.size() > builtInCacheDependencies.size())
		{
			nonUnloadableResourcePath = VuoFileUtilities::makeTmpFile(linkedCompositionFile + "-resource-nonunloadable", "dylib");
			nonUnloadableDependencies = builtInModuleAndLibraryDependencies;

			set<string> librariesForNonUnloadableResource = builtInLibraries;
			librariesForNonUnloadableResource.insert(carriedOverNonUnloadableLibraries.begin(), carriedOverNonUnloadableLibraries.end());
			librariesForNonUnloadableResource.insert(carriedOverExternalLibraries.begin(), carriedOverExternalLibraries.end());
			librariesForNonUnloadableResource.insert(addedExternalLibraries.begin(), addedExternalLibraries.end());

			vector<string> rPaths = getRunPathSearchPaths(environments.front().front());

			link(nonUnloadableResourcePath, builtInModules, librariesForNonUnloadableResource, allFrameworks, true, rPaths, shouldAdHocCodeSign);

			for (set<string>::iterator i = builtInLibraries.begin(); i != builtInLibraries.end(); )
			{
				if (! VuoStringUtilities::endsWith(*i, ".dylib"))
					builtInLibraries.erase(i++);
				else
					i++;
			}

			for (set<string>::iterator i = addedExternalLibraries.begin(); i != addedExternalLibraries.end(); )
			{
				if (! VuoStringUtilities::endsWith(*i, ".dylib"))
					addedExternalLibraries.erase(i++);
				else
					i++;
			}
		}

		// If user dependencies were added or module caches were rebuilt, create an additional resource dylib.

		if (! userModules.empty() || userLibraries.size() > userCacheDependencies.size() || wasModuleCacheRebuilt)
		{
			unloadableResourcePath = VuoFileUtilities::makeTmpFile(linkedCompositionFile + "-resource-unloadable", "dylib");
			unloadableDependencies = userModuleAndLibraryDependencies;

			set<string> librariesForUnloadableResource = userLibraries;
			librariesForUnloadableResource.insert(builtInLibraries.begin(), builtInLibraries.end());
			librariesForUnloadableResource.insert(carriedOverUnloadableLibraries.begin(), carriedOverUnloadableLibraries.end());
			librariesForUnloadableResource.insert(carriedOverNonUnloadableLibraries.begin(), carriedOverNonUnloadableLibraries.end());
			librariesForUnloadableResource.insert(carriedOverExternalLibraries.begin(), carriedOverExternalLibraries.end());
			librariesForUnloadableResource.insert(addedExternalLibraries.begin(), addedExternalLibraries.end());
			if (! nonUnloadableResourcePath.empty())
				librariesForUnloadableResource.insert(nonUnloadableResourcePath);

			// This is usually correct, but may fail in the case where there are two identically-named dylibs at different
			// levels of scope. However, it's the best we can do as long as modules at the system, user, and composition
			// levels of scope are all combined into one resource dylib. (https://b33p.net/kosada/vuo/vuo/-/merge_requests/196#note_2148884)
			vector<string> rPaths = getRunPathSearchPaths(environments.back().front());

			link(unloadableResourcePath, userModules, librariesForUnloadableResource, allFrameworks, true, rPaths, shouldAdHocCodeSign);

			for (set<string>::iterator i = userLibraries.begin(); i != userLibraries.end(); )
			{
				if (! VuoStringUtilities::endsWith(*i, ".dylib"))
					userLibraries.erase(i++);
				else
					i++;
			}
		}
	}

	// Get the Vuo runtime dependency.

	set<string> vuoRuntimePaths;
	{
		set<Module *> modules;
		set<string> libraries;
		set<string> frameworks;

		set<string> dependencies;
		dependencies.insert(getRuntimeDependency());
		getLinkerInputs(dependencies, Optimization_FastBuild, modules, libraries, frameworks);
		vuoRuntimePaths = libraries;
	}

	// Link the composition.

	{
		set<Module *> modules;
		set<string> libraries;

		libraries.insert(compiledCompositionPath);
		libraries.insert(carriedOverExternalLibraries.begin(), carriedOverExternalLibraries.end());
		libraries.insert(addedExternalLibraries.begin(), addedExternalLibraries.end());
		libraries.insert(carriedOverNonUnloadableLibraries.begin(), carriedOverNonUnloadableLibraries.end());
		libraries.insert(carriedOverUnloadableLibraries.begin(), carriedOverUnloadableLibraries.end());
		libraries.insert(builtInLibraries.begin(), builtInLibraries.end());
		libraries.insert(userLibraries.begin(), userLibraries.end());
		if (! nonUnloadableResourcePath.empty())
			libraries.insert(nonUnloadableResourcePath);
		if (! unloadableResourcePath.empty())
			libraries.insert(unloadableResourcePath);
		libraries.insert(vuoRuntimePaths.begin(), vuoRuntimePaths.end());
		vector<string> rPaths = getRunPathSearchPaths(environments.front().front());
		link(linkedCompositionPath, modules, libraries, allFrameworks, true, rPaths, shouldAdHocCodeSign);
	}

	// Now that we're past the point where an exception can be thrown, update the RunningCompositionLibraries.

	if (! nonUnloadableResourcePath.empty())
		runningCompositionLibraries->enqueueResourceLibraryToLoad(nonUnloadableResourcePath, nonUnloadableDependencies, false);

	if (! unloadableResourcePath.empty())
		runningCompositionLibraries->enqueueResourceLibraryToLoad(unloadableResourcePath, unloadableDependencies, true);

	for (map<string, set<string> >::iterator i = builtInCacheDependencies.begin(); i != builtInCacheDependencies.end(); ++i)
		runningCompositionLibraries->enqueueCacheLibraryToLoad(i->first, i->second, false);

	for (map<string, set<string> >::iterator i = userCacheDependencies.begin(); i != userCacheDependencies.end(); ++i)
		runningCompositionLibraries->enqueueCacheLibraryToLoad(i->first, i->second, true);

	runningCompositionLibraries->addExternalFrameworks(addedFrameworks);
	runningCompositionLibraries->addExternalLibraries(addedExternalLibraries);
}

/**
 * Returns the names of dependencies (node classes, types, libraries, and frameworks)
 * needed for linking the composition.
 *
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition.
 */
set<string> VuoCompiler::getDependenciesForComposition(const string &compiledCompositionPath)
{
	double t0 = VuoLogGetTime();

	// Add the node classes in the top-level composition and their dependencies.
	__block set<string> directDependencies;
	string moduleKey = getModuleKeyForPath(compiledCompositionPath);
	Module *module = readModuleFromBitcode(compiledCompositionPath, getTargetArch(target));
	dispatch_sync(llvmQueue, ^{
					  VuoCompilerModule *compilerModule = VuoCompilerModule::newModule(moduleKey, module, "", VuoCompilerCompatibility::compatibilityWithAnySystem());
					  directDependencies = compilerModule->getDependencies();
					  delete compilerModule;
					  delete module;
				  });

	try
	{
		auto deps = getDependenciesForComposition(directDependencies, true);
		VUserLog("Gathering dependencies for '%s' took %5.2fs", compiledCompositionPath.c_str(), VuoLogGetTime() - t0);
		return deps;
	}
	catch (VuoCompilerException &e)
	{
		e.getIssues()->setFilePathIfEmpty(compiledCompositionPath);
		throw;
	}
}

/**
 * Returns the names of dependencies (node classes and types) directly referenced by the composition.
 * Does not include dependencies of dependencies.
 *
 * @version200New
 */
set<string> VuoCompiler::getDirectDependenciesForComposition(VuoCompilerComposition *composition)
{
	set<string> directDependencies;

	set<VuoCompilerNode *> nodes = composition->getCachedGraph(this)->getNodes();
	for (VuoCompilerNode *node : nodes)
		if (node->getBase()->getNodeClass()->hasCompiler())
			directDependencies.insert( node->getBase()->getNodeClass()->getCompiler()->getDependencyName() );

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();
	vector<VuoPublishedPort *> publishedPorts;
	publishedPorts.insert(publishedPorts.end(), publishedInputPorts.begin(), publishedInputPorts.end());
	publishedPorts.insert(publishedPorts.end(), publishedOutputPorts.begin(), publishedOutputPorts.end());
	for (VuoPublishedPort *publishedPort : publishedPorts)
	{
		if (publishedPort->getClass()->hasCompiler())
		{
			VuoType *portType = static_cast<VuoCompilerPortClass *>( publishedPort->getClass()->getCompiler() )->getDataVuoType();
			if (portType)
			{
				string dependency;
				VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(portType);
				if (genericType)
				{
					VuoGenericType::Compatibility compatibility;
					vector<string> compatibleTypeNames = genericType->getCompatibleSpecializedTypes(compatibility);
					dependency = VuoCompilerGenericType::chooseBackingTypeName(portType->getModuleKey(), compatibleTypeNames);
				}
				else
					dependency = portType->getModuleKey();

				directDependencies.insert(dependency);
			}
		}
	}

	return directDependencies;
}

/**
 * Returns the names of dependencies (node classes, types, libraries, and frameworks)
 * needed for linking the composition.
 *
 * @version200New
 */
set<string> VuoCompiler::getDependenciesForComposition(VuoCompilerComposition *composition)
{
	set<string> directDependencies = getDirectDependenciesForComposition(composition);
	return getDependenciesForComposition(directDependencies, false);
}

/**
 * Returns the absolute paths of dylibs needed for linking the composition.
 *
 * @version200New
 */
set<string> VuoCompiler::getDylibDependencyPathsForComposition(VuoCompilerComposition *composition)
{
	__block vector<string> librarySearchPaths;
	applyToInstalledEnvironments(^void (Environment *env) {
		vector<string> result = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), result.begin(), result.end());
	});

	set<string> dylibDeps;
	for (string dep : getDependenciesForComposition(composition))
	{
		string path = getLibraryPath(dep, librarySearchPaths);
		if (VuoStringUtilities::endsWith(path, ".dylib"))
			dylibDeps.insert(path);
	}

	return dylibDeps;
}

/**
 * Returns the names of dependencies (node classes, types, libraries, and frameworks)
 * needed for linking the composition.
 *
 * This includes the composition's nodes, their dependencies, and the libraries needed
 * by every linked composition. It does not include the Vuo runtime or a main function.
 *
 * If @a checkCompatibility is true, this function throws an exception if it encounters an
 * incompatible dependency. Otherwise, it merely excludes the incompatible dependency
 * from the returned set.
 *
 * @throw VuoCompilerException At least one of the dependencies is incompatible with the targets for building the composition.
 */
set<string> VuoCompiler::getDependenciesForComposition(const set<string> &directDependencies, bool checkCompatibility)
{
	// Make sure that any compiler-generated node classes have been generated and added to the dependency graph.
	for (set<string>::const_iterator i = directDependencies.begin(); i != directDependencies.end(); ++i)
		getNodeClass(*i);

	set<string> dependencies;
	for (set<string>::iterator i = directDependencies.begin(); i != directDependencies.end(); ++i)
	{
		string moduleKey = *i;

		dependencies.insert(moduleKey);

		// First pass: Find all dependencies of the direct dependency that have been loaded so far.
		vector<VuoDirectedAcyclicGraph::Vertex *> firstPassVertices = dependencyGraph->findVertex(moduleKey);
		set<VuoDirectedAcyclicGraph::Vertex *> firstPassDependencies(firstPassVertices.begin(), firstPassVertices.end());
		for (vector<VuoDirectedAcyclicGraph::Vertex *>::iterator j = firstPassVertices.begin(); j != firstPassVertices.end(); ++j)
		{
			vector<VuoDirectedAcyclicGraph::Vertex *> downstream = dependencyGraph->getDownstreamVertices(*j);
			firstPassDependencies.insert(downstream.begin(), downstream.end());
		}

		// Make sure that any compiler-generated node classes in subcompositions have been generated and added to the dependency graph.
		for (set<VuoDirectedAcyclicGraph::Vertex *>::iterator j = firstPassDependencies.begin(); j != firstPassDependencies.end(); ++j)
		{
			DependencyGraphVertex *v = static_cast<DependencyGraphVertex *>(*j);
			getNodeClass(v->getDependency());
		}

		// Second pass: Find all dependencies of the direct dependency, this time including dependencies of the node classes just generated.
		vector<VuoDirectedAcyclicGraph::Vertex *> moduleVertices = dependencyGraph->findVertex(moduleKey);
		for (vector<VuoDirectedAcyclicGraph::Vertex *>::iterator j = moduleVertices.begin(); j != moduleVertices.end(); ++j)
		{
			vector<VuoDirectedAcyclicGraph::Vertex *> downstream = dependencyGraph->getDownstreamVertices(*j);
			for (VuoDirectedAcyclicGraph::Vertex *v : downstream)
				dependencies.insert( static_cast<DependencyGraphVertex *>(v)->getDependency() );
		}
	}

	// Check that the dependencies are compatible with the compiler's target.
	if (checkCompatibility)
	{
		VuoCompilerCompatibility neededCompatibility = VuoCompilerCompatibility::compatibilityWithTargetTriple(target);
		VuoCompilerCompatibility actualCompatibility = getCompatibilityOfDependencies(dependencies);

		if (! actualCompatibility.isCompatibleWith(neededCompatibility))
		{
			VuoCompilerIssues *issues = new VuoCompilerIssues;
			set<VuoCompilerNodeClass *> nodeClassesReported;

			for (string moduleKey : dependencies)
			{
				VuoCompilerModule *module = getModule(moduleKey);
				if (module && ! module->getCompatibleTargets().isCompatibleWith(neededCompatibility))
				{
					vector<VuoCompilerNodeClass *> incompatibleNodeClasses;

					VuoCompilerNodeClass *moduleAsNodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
					if (moduleAsNodeClass)
						incompatibleNodeClasses.push_back(moduleAsNodeClass);
					else
					{
						vector<VuoDirectedAcyclicGraph::Vertex *> moduleVertices = dependencyGraph->findVertex(moduleKey);
						for (VuoDirectedAcyclicGraph::Vertex *v : moduleVertices)
						{
							vector<VuoDirectedAcyclicGraph::Vertex *> upstream = dependencyGraph->getUpstreamVertices(v);

							vector<string> upstreamModuleKeys;
							std::transform(upstream.begin(), upstream.end(),
										   std::back_inserter(upstreamModuleKeys),
										   [](VuoDirectedAcyclicGraph::Vertex *u){ return static_cast<DependencyGraphVertex *>(u)->getDependency(); });

							vector<string> potentialNodeClassNames;
							std::set_intersection(directDependencies.begin(), directDependencies.end(),
												  upstreamModuleKeys.begin(), upstreamModuleKeys.end(),
												  std::back_inserter(potentialNodeClassNames));

							for (string nodeClassName : potentialNodeClassNames)
							{
								VuoCompilerNodeClass *upstreamNodeClass = getNodeClass(nodeClassName);
								if (upstreamNodeClass)
									incompatibleNodeClasses.push_back(upstreamNodeClass);
							}
						}
					}

					if (incompatibleNodeClasses.empty())
					{
						VuoCompilerIssue issue(VuoCompilerIssue::Error, "linking composition", "",
											   "Dependencies incompatible with system",
											   "%module is only compatible with " + module->getCompatibleTargets().toString() +
											   ", so this composition can't run on " + neededCompatibility.toString() + ".");
						issue.setModule(module->getPseudoBase());
						issues->append(issue);
					}
					else
					{
						for (VuoCompilerNodeClass *nodeClass : incompatibleNodeClasses)
						{
							if (nodeClassesReported.find(nodeClass) != nodeClassesReported.end())
								continue;

							nodeClassesReported.insert(nodeClass);

							VuoCompilerIssue issue(VuoCompilerIssue::Error, "linking composition", "",
												   "Nodes incompatible with system",
												   "%module is only compatible with " + module->getCompatibleTargets().toString() +
												   ", so this composition can't run on " + neededCompatibility.toString() + ".");
							issue.setModule(nodeClass->getPseudoBase());
							issues->append(issue);
						}
					}
				}
			}

			if (issues->isEmpty())
			{
				VuoCompilerIssue issue(VuoCompilerIssue::Error, "linking composition", "",
									   "Dependencies incompatible with system",
									   "Some dependencies of this composition are only compatible with " + actualCompatibility.toString() +
									   ", so this composition can't run on " + neededCompatibility.toString() + ".");
				issues->append(issue);
			}

			throw VuoCompilerException(issues, true);
		}
	}

	// Add the libraries needed by every linked composition.
	vector<string> coreDependencies = getCoreVuoDependencies();
	dependencies.insert(coreDependencies.begin(), coreDependencies.end());

	return dependencies;
}

/**
 * Returns a description of the set of systems that all of @a dependencies are able to run on.
 */
VuoCompilerCompatibility VuoCompiler::getCompatibilityOfDependencies(const set<string> &dependencies)
{
	VuoCompilerCompatibility compatibility(nullptr);

	for (string dependency : dependencies)
	{
		VuoCompilerModule *module = getModule(dependency);
		if (module)
			compatibility = compatibility.intersection(module->getCompatibleTargets());
	}

	return compatibility;
}

/**
 * From a list of names of dependencies, gets the modules, library paths, and frameworks
 * to be passed to the linker.
 */
void VuoCompiler::getLinkerInputs(const set<string> &dependencies, Optimization optimization,
								  set<Module *> &modules, set<string> &libraries, set<string> &frameworks)
{
	set<string> builtInModuleAndLibraryDependencies;
	set<string> userModuleAndLibraryDependencies;
	map<string, set<string> > builtInCacheDependencies;
	map<string, set<string> > userCacheDependencies;
	set<Module *> builtInModules;
	set<Module *> userModules;
	set<string> builtInLibraries;
	set<string> userLibraries;
	set<string> externalLibraries;

	getLinkerInputs(dependencies, optimization,
					builtInModuleAndLibraryDependencies, userModuleAndLibraryDependencies, builtInCacheDependencies, userCacheDependencies,
					builtInModules, userModules, builtInLibraries, userLibraries, externalLibraries, frameworks);

	modules.insert(builtInModules.begin(), builtInModules.end());
	modules.insert(userModules.begin(), userModules.end());
	libraries.insert(builtInLibraries.begin(), builtInLibraries.end());
	libraries.insert(userLibraries.begin(), userLibraries.end());
	libraries.insert(externalLibraries.begin(), externalLibraries.end());
}

/**
 * From a list of names of dependencies, gets the modules, library paths, and frameworks
 * to be passed to the linker — separating the built-in modules and libraries from those
 * installed by the user.
 *
 * The purpose of separating built-in from user modules is to work around a limitation of Objective-C.
 * When Objective-C code is in a dynamic library, the dynamic library can't be unloaded, so the
 * modules in the dynamic library can't be modified in a live-coding reload.
 * Some built-in modules contain Objective-C code, but the built-in modules don't need to be unloaded.
 * Assuming the user modules don't contain any Objective-C code, a dynamic library containing them
 * will be able to be unloaded.
 * (https://stackoverflow.com/questions/8793099/unload-dynamic-library-needs-two-dlclose-calls)
 */
void VuoCompiler::getLinkerInputs(const set<string> &dependencies, Optimization optimization,
								  set<string> &builtInModuleAndLibraryDependencies, set<string> &userModuleAndLibraryDependencies,
								  map<string, set<string> > &builtInCacheDependencies, map<string, set<string> > &userCacheDependencies,
								  set<Module *> &builtInModules, set<Module *> &userModules,
								  set<string> &builtInLibraries, set<string> &userLibraries,
								  set<string> &externalLibraries, set<string> &externalFrameworks)
{
	bool shouldUseModuleCache = (optimization == Optimization_FastBuild || optimization == Optimization_FastBuildExistingCache);
	if (shouldUseModuleCache)
		useModuleCache(true, optimization == Optimization_FastBuildExistingCache);

	__block vector<string> librarySearchPaths;
	void (^envGetLibrarySearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetLibrarySearchPaths);

	for (set<string>::iterator i = dependencies.begin(); i != dependencies.end(); ++i)
	{
		string dependency = *i;

		bool foundInCache = false;
		string moduleCachePath;
		bool isInBuiltInModuleCache = false;
		if (shouldUseModuleCache)
			foundInCache = findInModuleCache(dependency, moduleCachePath, isInBuiltInModuleCache);

		if (foundInCache)
		{
			if (isInBuiltInModuleCache)
			{
				builtInLibraries.insert(moduleCachePath);
				builtInCacheDependencies[moduleCachePath].insert(dependency);
			}
			else
			{
				userLibraries.insert(moduleCachePath);
				userCacheDependencies[moduleCachePath].insert(dependency);
			}
		}
		else
		{
			__block VuoCompilerModule *module = NULL;
			void (^envFindModule)(Environment *) = ^void (Environment *env) {
				VuoCompilerModule *result = env->findModule(dependency);
				if (result)
					module = result;
			};
			applyToAllEnvironments(envFindModule);

			if (module)
			{
				VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
				if (! (nodeClass && VuoCompilerSpecializedNodeClass::hasGenericPortTypes(nodeClass)) )  // Skip not-fully-specialized generic modules
				{
					string modulePath = module->getModulePath();
					if (! modulePath.empty() && dynamic_cast<VuoCompilerType *>(module))
					{
						if (module->isBuiltIn())
							builtInLibraries.insert(modulePath);
						else
							userLibraries.insert(modulePath);
					}
					else
					{
						if (module->isBuiltIn())
							builtInModules.insert(module->getModule());
						else
							userModules.insert(module->getModule());
					}

					if (module->isBuiltIn())
						builtInModuleAndLibraryDependencies.insert(dependency);
					else
						userModuleAndLibraryDependencies.insert(dependency);
				}
			}
			else
			{
				if (VuoStringUtilities::endsWith(dependency, ".framework"))
					externalFrameworks.insert(dependency);
				else
				{
					string dependencyPath = getLibraryPath(dependency, librarySearchPaths);
					if (! dependencyPath.empty())
						externalLibraries.insert(dependencyPath);

					// On macOS 11, libc.dylib and libobjc.dylib are not present,
					// but we can still link since Vuo.framework includes the TBDs.
					else if (dependency != "c"
						  && dependency != "objc")
						VUserLog("Warning: Could not locate dependency '%s'.", dependency.c_str());
				}
			}
		}
	}
}

/**
 * Returns the path of the library called @a dependency if it's located in @a librarySearchPaths or `/usr/lib`,
 * or returns the path as-is if it's an absolute path and it exists,
 * otherwise an empty string.
 */
string VuoCompiler::getLibraryPath(const string &dependency, vector<string> librarySearchPaths)
{
	if (dependency[0] == '/' && VuoFileUtilities::fileExists(dependency))
		return dependency;

	// Put the system library folder last in the list — prefer to use the libraries bundled in Vuo.framework.
	// Don't attempt to use OpenSSL from the system library folder, since macOS 10.15 prevents linking to it.
	if (dependency != "crypto"
	 && dependency != "ssl")
		 librarySearchPaths.push_back("/usr/lib");

	for (auto &path : librarySearchPaths)
	{
		vector<string> variations;
		variations.push_back(path + "/"    + dependency);
		variations.push_back(path + "/lib" + dependency);
		variations.push_back(path + "/lib" + dependency + ".dylib");
		variations.push_back(path + "/lib" + dependency + ".a");
		for (auto &variation : variations)
			if (VuoFileUtilities::fileExists(variation))
				return variation;
	}

	return "";
}

/**
 * Attempts to make the module caches available for all of this compiler's environments.
 *
 * Assumes @ref shouldLoadAllModules is true.
 *
 * @see VuoCompiler::Environment::useModuleCache
 */
void VuoCompiler::useModuleCache(bool shouldUseExistingBuiltInCaches, bool shouldUseExistingOtherCaches)
{
	loadModulesIfNeeded();
	dispatch_group_wait(moduleSourceCompilersExist, DISPATCH_TIME_FOREVER);  // Wait for any previous loadModulesIfNeeded() calls to complete.

	// Iterate through the environments in the order that the caches need to be built.

	dispatch_sync(environmentQueue, ^{
					  set<string> dylibsForCachesOfInstalledModules;
					  set<string> frameworksForCachesOfInstalledModules;
					  unsigned long lastPrerequisiteModuleCacheRebuild = 0;
					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
					  {
						  set<string> dylibsForCacheOfGeneratedModules;
						  set<string> frameworksForCacheOfGeneratedModules;

						  for (int j = i->size() - 1; j >= 0; --j)
						  {
							  Environment *env = i->at(j);
							  bool installed = (j == 0);

							  set<string> cacheableModulesAndDependencies;
							  set<string> dylibsNeededToLinkToThisCache;
							  set<string> frameworksNeededToLinkToThisCache;
							  env->getCacheableModulesAndDependencies(cacheableModulesAndDependencies,
																	  dylibsNeededToLinkToThisCache, frameworksNeededToLinkToThisCache);

							  set<string> accumulatedDylibs;
							  accumulatedDylibs.insert(dylibsNeededToLinkToThisCache.begin(), dylibsNeededToLinkToThisCache.end());
							  accumulatedDylibs.insert(dylibsForCachesOfInstalledModules.begin(), dylibsForCachesOfInstalledModules.end());
							  accumulatedDylibs.insert(dylibsForCacheOfGeneratedModules.begin(), dylibsForCacheOfGeneratedModules.end());

							  set<string> accumulatedFrameworks;
							  accumulatedFrameworks.insert(frameworksNeededToLinkToThisCache.begin(), frameworksNeededToLinkToThisCache.end());
							  accumulatedFrameworks.insert(frameworksForCachesOfInstalledModules.begin(), frameworksForCachesOfInstalledModules.end());
							  accumulatedFrameworks.insert(frameworksForCacheOfGeneratedModules.begin(), frameworksForCacheOfGeneratedModules.end());

							  bool shouldUseExistingCache = (env->isBuiltIn() ? shouldUseExistingBuiltInCaches : shouldUseExistingOtherCaches);
							  env->useModuleCache(shouldUseExistingCache, this, cacheableModulesAndDependencies,
												  accumulatedDylibs, accumulatedFrameworks, lastPrerequisiteModuleCacheRebuild);

							  string cacheDylib = env->getCurrentModuleCacheDylib();
							  accumulatedDylibs.insert(cacheDylib);
							  dylibsForCachesOfInstalledModules.insert(cacheDylib);

							  lastPrerequisiteModuleCacheRebuild = max(lastPrerequisiteModuleCacheRebuild, env->getLastModuleCacheRebuild());

							  if (installed)
							  {
								  dylibsForCachesOfInstalledModules.insert(dylibsNeededToLinkToThisCache.begin(), dylibsNeededToLinkToThisCache.end());
								  frameworksForCachesOfInstalledModules.insert(frameworksNeededToLinkToThisCache.begin(), frameworksNeededToLinkToThisCache.end());
							  }
							  else
							  {
								  dylibsForCacheOfGeneratedModules.insert(dylibsNeededToLinkToThisCache.begin(), dylibsNeededToLinkToThisCache.end());
								  frameworksForCacheOfGeneratedModules.insert(frameworksNeededToLinkToThisCache.begin(), frameworksNeededToLinkToThisCache.end());
							  }
						  }
					  }
				  });

	Environment::waitForModuleCachesToBuild();
}

/**
 * Returns true if the module or other dependency with key/name @a moduleOrDependency is located in one of the
 * module caches available to this compiler; also outputs information about the cache. Returns false otherwise.
 *
 * Assumes @ref useModuleCache has been called.
 *
 * @see VuoCompiler::Environment::findInModuleCache
 */
bool VuoCompiler::findInModuleCache(const string &moduleOrDependency, string &cachePath, bool &isBuiltinCache)
{
	__block bool found = false;
	__block string outPath;
	__block bool outBuiltin;
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<Environment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
					  {
						  bool builtin = (i == environments.begin());

						  for (int j = i->size() - 1; j >= 0; --j)
						  {
							  Environment *env = i->at(j);

							  string resultPath;
							  bool resultFound = env->findInModuleCache(moduleOrDependency, resultPath);
							  if (resultFound)
							  {
								  found = true;
								  outPath = resultPath;
								  outBuiltin = builtin;
							  }
						  }
					  }
				  });

	cachePath = outPath;
	isBuiltinCache = outBuiltin;
	return found;
}

/**
 * Asynchronously prepares the cache that enables compositions to build faster.
 *
 * The first time a composition is built with the "faster build" optimization, it waits until the cache
 * is prepared. Preparing the cache may take several seconds. By calling this function, the cache can be
 * prepared in advance, so that there's no delay when building the first composition.
 */
void VuoCompiler::prepareForFastBuild(void)
{
	dispatch_group_async(moduleCacheBuilding, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							 useModuleCache(true, false);
						 });
}

/**
 * Returns this compiler instance's LLVM target triple.
 * E.g., `x86_64-apple-macosx10.10.0`.
 */
string VuoCompiler::getTarget(void)
{
	return target;
}

/**
 * Returns this compiler instance's CPU architecture.
 * E.g., `x86_64` or `arm64`.
 */
string VuoCompiler::getArch(void)
{
	return getTargetArch(target);
}

/**
 * Generates the caches of built-in modules and places them inside of Vuo.framework.
 *
 * This function is intended for running Vuo in a special mode just to generate the caches and then exit.
 * It's called while building Vuo itself, toward the end of the process of creating Vuo.framework.
 *
 * Assumes that no VuoCompiler instances have been constructed before this call.
 *
 * @param vuoFrameworkPath The absolute path of Vuo.framework, as would be returned by
 *     @ref VuoFileUtilities::getVuoFrameworkPath if the Vuo.framework dynamic library were loaded.
 * @param target The LLVM target-triple to build.
 * @version200New
 */
void VuoCompiler::generateBuiltInModuleCaches(string vuoFrameworkPath, string target)
{
	vuoFrameworkInProgressPath = vuoFrameworkPath;

	VuoCompiler compiler("", target);
	compiler.useModuleCache(false, true);
}

/**
 * Deletes:
 *    - any composition-local module cache directories that have not been accessed for more than 30 days.
 *    - any pid-specific module cache directories for pids that are not currently running.
 *
 * @version200New
 */
void VuoCompiler::deleteOldModuleCaches(void)
{
	unsigned long maxSeconds = 30 * 24 * 60 * 60;  // 30 days

	set<VuoFileUtilities::File *> cacheDirs = VuoFileUtilities::findAllFilesInDirectory(VuoFileUtilities::getCachePath());
	for (set<VuoFileUtilities::File *>::iterator i = cacheDirs.begin(); i != cacheDirs.end(); ++i)
	{
		string path = (*i)->path();

		string file = (*i)->basename();
		if (file != "Builtin" && file != "System" && file != "User")
		{
			unsigned long fileSeconds = VuoFileUtilities::getSecondsSinceFileLastAccessed(path);
			if (fileSeconds > maxSeconds)
				VuoFileUtilities::deleteDir(path);
		}

		if (VuoStringUtilities::beginsWith(file, Environment::pidCacheDirPrefix))
		{
			string pidAsString = file.substr(Environment::pidCacheDirPrefix.length());
			int pid = atoi(pidAsString.c_str());
			if (kill(pid, 0) != 0)  // no running process has this pid
				VuoFileUtilities::deleteDir(path);
		}

		delete *i;
	}
}

/**
 * Controls whether the VuoCompiler should load all node classes, types, and library modules from the
 * module search paths the first time a module is needed.
 */
void VuoCompiler::setLoadAllModules(bool shouldLoadAllModules)
{
	dispatch_sync(modulesToLoadQueue, ^{
					  this->shouldLoadAllModules = shouldLoadAllModules;
				  });
}

/**
 * Links the given modules, libraries, and frameworks to create an executable or dynamic library.
 *
 * @param outputPath The resulting executable or dynamic library.
 * @param modules The LLVM modules to link in.
 * @param libraries The libraries to link in. If building an executable, one of them should contain a main function.
 * @param frameworks The frameworks to link in.
 * @param isDylib If true, the output file will be a dynamic library. Otherwise, it will be an executable.
 * @param rPaths The `-rpath` (run-path search path) arguments to be passed to `ld`.
 * @param shouldAdHocCodeSign  Whether to ad-hoc code-sign the generated binary.  Disable to improve performance (e.g., for live-editing on systems that don't require code-signing).
 * @throw VuoCompilerException clang or ld failed to link the given dependencies.
 */
void VuoCompiler::link(string outputPath, const set<Module *> &modules, const set<string> &libraries, const set<string> &frameworks, bool isDylib, const vector<string> &rPaths, bool shouldAdHocCodeSign, VuoCompilerIssues *issues)
{
	VUserLog("Linking '%s' (%s)…", outputPath.c_str(), getTargetArch(target).c_str());
	// https://stackoverflow.com/questions/11657529/how-to-generate-an-executable-from-an-llvmmodule

	bool ownsIssues = false;
	if (! issues)
	{
		issues = new VuoCompilerIssues;
		ownsIssues = true;
	}

	// Write all the modules with renamed symbols to a composite module file (since the linker can't operate on in-memory modules).
	string compositeModulePath = VuoFileUtilities::makeTmpFile("composite", "bc");
	dispatch_sync(llvmQueue, ^{
					  double t0 = VuoLogGetTime();

					  unique_ptr<Module> compositeModule(new Module("composite", *globalLLVMContext));
					  Linker linker(*compositeModule);
					  setTargetForModule(compositeModule.get(), target);

					  for (auto i : modules)
					  {
						  unique_ptr<Module> upi = llvm::CloneModule(i);
						  if (linker.linkInModule(std::move(upi)))
						  {
							  VuoCompilerIssue issue(VuoCompilerIssue::IssueType::Error, "linking composite module", "",
													 "", "Failed to link in the module with ID '" + i->getModuleIdentifier() + "'");
							  issues->append(issue);
						  }
					  }

					  writeModuleToBitcode(compositeModule.get(), compositeModulePath);

					  VUserLog("\tLinkModules took %5.2fs", VuoLogGetTime() - t0);
				  });


	// llvm-3.1/llvm/tools/clang/tools/driver/driver.cpp

	// Invoke clang as `clang++` so it includes the C++ standard libraries.
	string clangPath(getClangPath() + "++");

	vector<const char *> args;
	vector<char *> argsToFree;
	args.push_back(clangPath.c_str());

	{
		char *outputPathZ = strdup(("-o" + outputPath).c_str());
		args.push_back(outputPathZ);
		argsToFree.push_back(outputPathZ);
	}

	args.push_back(compositeModulePath.c_str());

	vector<string> coreDependencies = getCoreVuoDependencies();
	for (set<string>::const_iterator i = libraries.begin(); i != libraries.end(); ++i)
	{
		string library = *i;

		for (vector<string>::iterator j = coreDependencies.begin(); j != coreDependencies.end(); ++j)
		{
			string coreDependency = *j;
			if (VuoStringUtilities::endsWith(library, "lib" + coreDependency + ".a"))
				args.push_back("-force_load");  // Load all symbols of static core dependencies, not just those used in the objects.
		}

		// Use the pre-built native object file if it exists (faster than converting .bc to .o every build).
		if (VuoStringUtilities::endsWith(library, ".bc"))
		{
			string libraryObject = VuoStringUtilities::substrBefore(library, ".bc") + ".o";
			if (VuoFileUtilities::fileExists(libraryObject))
				library = libraryObject;
		}

		char *libraryZ = strdup(library.c_str());
		args.push_back(libraryZ);
		argsToFree.push_back(libraryZ);
	}

	// Add framework search paths
	vector<string> frameworkArguments;

	__block vector<string> frameworkSearchPaths;
	void (^envGetFrameworkSearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getFrameworkSearchPaths();
		frameworkSearchPaths.insert(frameworkSearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetFrameworkSearchPaths);

	for (vector<string>::const_iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
	{
		string a = "-F"+*i;
		// Keep these std::strings around until after args is done being used, since the std::string::c_str() is freed when the std::string is deleted.
		frameworkArguments.push_back(a);
		char *frameworkArgument = strdup(a.c_str());
		args.push_back(frameworkArgument);
		argsToFree.push_back(frameworkArgument);
	}

	for (set<string>::const_iterator i = frameworks.begin(); i != frameworks.end(); ++i)
	{
		args.push_back("-framework");

		string frameworkName = *i;
		frameworkName = frameworkName.substr(0, frameworkName.length() - string(".framework").length());
		char *frameworkNameZ = strdup(frameworkName.c_str());
		args.push_back(frameworkNameZ);
		argsToFree.push_back(frameworkNameZ);
	}

	// When linking on a development workstation or Jenkins or an end-user system,
	// use the partial macOS SDK bundled in Vuo.framework, since it includes all the TBDs we need.
	string vuoFrameworkPath = getVuoFrameworkPath();
	string frameworkMacOSSDKFolder = vuoFrameworkPath + "/SDKs/MacOSX.sdk";
	if (!VuoFileUtilities::fileExists(frameworkMacOSSDKFolder))
		throw VuoException("Couldn't find the macOS SDK.");

	args.push_back("-Xlinker");
	args.push_back("-syslibroot");
	args.push_back("-Xlinker");
	char *frameworkMacOSSDKFolderZ = strdup(frameworkMacOSSDKFolder.c_str());
	args.push_back(frameworkMacOSSDKFolderZ);
	argsToFree.push_back(frameworkMacOSSDKFolderZ);

	args.push_back("-Xlinker");
	args.push_back("-platform_version");
	args.push_back("-Xlinker");
	args.push_back("macos");
	args.push_back("-Xlinker");
	char *deploymentTargetZ = strdup(MACOS_DEPLOYMENT_TARGET);
	args.push_back(deploymentTargetZ);
	argsToFree.push_back(deploymentTargetZ);
	args.push_back("-Xlinker");
	char *sdkVersionZ = strdup(MACOS_SDK_VERSION);
	args.push_back(sdkVersionZ);
	argsToFree.push_back(sdkVersionZ);

	// Linker option necessary for compatibility with our bundled version of ld64:
	args.push_back("-Xlinker");
	args.push_back("--no-demangle");

	if (isVerbose)
		args.push_back("-v");

	if (isDylib)
		args.push_back("-dynamiclib");

	args.push_back("-Xlinker");
	args.push_back("-headerpad_max_install_names");

	// Tell the built dylib/executable where to find dylibs that it depends on
	for (const string &rPath : rPaths)
	{
		args.push_back("-rpath");
		args.push_back(rPath.c_str());
	}

#ifdef COVERAGE
	args.push_back("-rpath");
	args.push_back(LLVM_ROOT "/lib");
#endif

	args.push_back("-target");
	args.push_back(target.c_str());

	args.push_back("-std=c++14");
	args.push_back("-stdlib=libc++");
	args.push_back("-mmacosx-version-min=10.10");

	// Allow clang to print meaningful error messages.
	auto diagnosticConsumer = new VuoCompilerDiagnosticConsumer(issues);
	clang::DiagnosticOptions *diagOptions = new clang::DiagnosticOptions();
	IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
	clang::DiagnosticsEngine Diags(DiagID, diagOptions, diagnosticConsumer);

	if (isVerbose)
	{
		ostringstream s;
		for (vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
			s << *i << " ";
		VUserLog("\t%s", s.str().c_str());
	}

	// Redirect linker output to a file, so we can feed it through VuoLog.
	string stdoutFile = VuoFileUtilities::makeTmpFile("vuo-linker-output", "txt");
	const StringRef stdoutPath(stdoutFile);
	const StringRef *redirects[] = {
		nullptr,      // stdin
		&stdoutPath,  // stdout
		&stdoutPath,  // stderr
	};

	// ExecuteAndWait's args needs to be null-terminated.
	const char **argsz = (const char **)malloc(sizeof(char *) * args.size() + 1);
	for (int i = 0; i < args.size(); ++i)
		argsz[i] = args[i];
	argsz[args.size()] = nullptr;

	string errMsg;
	bool executionFailed;
	double t0 = VuoLogGetTime();
	int ret = llvm::sys::ExecuteAndWait(args[0], argsz, nullptr, redirects, 0, 0, &errMsg, &executionFailed);

	for (auto i : argsToFree)
		free(i);

	// Clean up composite module file.
	remove(compositeModulePath.c_str());

	if (!isDylib)
		// Ensure the linked binary has the execute permission set
		// (ld64-242 doesn't reliably set it, whereas ld64-133.3 did).
		// https://b33p.net/kosada/node/14152
		chmod(outputPath.c_str(), 0755);

	if (ret != 0)
	{
		string details;
		if (!errMsg.empty())
		{
			VUserLog("%s", errMsg.c_str());
			details += "\n" + errMsg + "\n";
		}
		string stdoutFileContents = VuoFileUtilities::readFileToString(stdoutFile);
		if (!stdoutFileContents.empty())
		{
			VUserLog("%s", stdoutFileContents.c_str());
			details += "\n" + stdoutFileContents + "\n";
		}
		VuoFileUtilities::deleteFile(stdoutFile);

		VuoCompilerIssue issue(VuoCompilerIssue::Error, "linking composition", outputPath, "", details);
		issues->append(issue);
		throw VuoCompilerException(issues, ownsIssues);
	}

	VuoFileUtilities::deleteFile(stdoutFile);
	VUserLog("\tLinking     took %5.2fs", VuoLogGetTime() - t0);

	if (shouldAdHocCodeSign)
		adHocCodeSign(outputPath);
}

/**
 * Ad-hoc code-signs the specified binary.
 */
void VuoCompiler::adHocCodeSign(string path)
{
	VuoFileUtilities::adHocCodeSign(path, {
#if VUO_PRO
		"CODESIGN_ALLOCATE=" + getCodesignAllocatePath(),
#endif
	});
}

/**
 * Returns the LLVM module read from the node class, type, or library implementation at @c inputPath (a .c file).
 *
 * @threadQueue{llvmQueue}
 */
Module *VuoCompiler::readModuleFromC(string inputPath, const vector<string> &headerSearchPaths, const vector<string> &extraArgs, VuoCompilerIssues *issues)
{
	// llvm-3.1/llvm/tools/clang/examples/clang-interpreter/main.cpp

	vector<const char *> args;
	vector<char *> argsToFree;

	args.push_back(inputPath.c_str());
	args.push_back("-DVUO_COMPILER");
	args.push_back("-fblocks");

	// Provide full backtraces, for easier debugging using Instruments and `VuoLog_backtrace()`.
	// The Clang driver translates `-fno-omit-frame-pointer` to `clang -cc1`'s `-mdisable-fp-elim`.
	// In Clang 10, this was renamed to `-mframe-pointer=all`.
	// https://b33p.net/kosada/vuo/vuo/-/issues/19064
	args.push_back("-mdisable-fp-elim");

	// Sync with /CMakeLists.txt's `commonFlags`.
	args.push_back("-Wall");
	args.push_back("-Wextra");
	args.push_back("-Wimplicit-fallthrough");
	args.push_back("-Wno-unused-parameter");
	args.push_back("-Wno-sign-compare");
	args.push_back("-Werror=implicit");

	if (VuoStringUtilities::endsWith(inputPath, ".cc"))
	{
		args.push_back("-std=c++14");
		args.push_back("-stdlib=libc++");
		args.push_back("-fexceptions");
		args.push_back("-fcxx-exceptions");
	}

	for (vector<string>::const_iterator i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
	{
		args.push_back("-I");
		args.push_back(i->c_str());
	}

#if VUO_PRO
	string dependencyOutputPath = _dependencyOutput();
	if (!dependencyOutputPath.empty())
	{
		char *outputObjectPath = strdup(dependencyOutputPath.substr(0, dependencyOutputPath.length() - 2).c_str());
		argsToFree.push_back(outputObjectPath);

		// https://bugzilla.mozilla.org/show_bug.cgi?id=1340588#c4
		// https://lists.llvm.org/pipermail/cfe-users/2018-March/001268.html
		args.push_back("-MT");
		args.push_back(outputObjectPath);
		args.push_back("-dependency-file");
		args.push_back(dependencyOutputPath.c_str());
	}
#endif

	if (isVerbose)
		args.push_back("-v");

	for (vector<string>::const_iterator i = extraArgs.begin(); i != extraArgs.end(); ++i)
		args.push_back(i->c_str());

	auto diagnosticConsumer = new VuoCompilerDiagnosticConsumer(issues);
	clang::DiagnosticOptions * diagOptions = new clang::DiagnosticOptions();
	IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
	clang::DiagnosticsEngine *diags = new clang::DiagnosticsEngine(DiagID, diagOptions, diagnosticConsumer);

	// This invokes `clang -cc1`.  See `clang -cc1 --help` for available options.
	shared_ptr<clang::CompilerInvocation> compilerInvocation(new clang::CompilerInvocation);
	clang::CompilerInvocation::CreateFromArgs(*compilerInvocation, &args[0], &args[0] + args.size(), *diags);
	compilerInvocation->TargetOpts->Triple = target;

	clang::CompilerInstance Clang;
	Clang.setInvocation(compilerInvocation);

	Clang.setDiagnostics(diags);
	if (!Clang.hasDiagnostics())
		return NULL;

	// See CompilerInvocation::GetResourcesPath -- though we're not calling it because we don't have MainAddr.
	string builtinHeaderSearchPath;
	string vuoFrameworkPath = getVuoFrameworkPath();
	if (vuoFrameworkPath.empty())
	{
		builtinHeaderSearchPath = getClangPath();
		if (VuoStringUtilities::endsWith(builtinHeaderSearchPath, "Helpers/clang"))
			builtinHeaderSearchPath = VuoStringUtilities::substrBefore(builtinHeaderSearchPath, "Helpers/clang");
		else if (VuoStringUtilities::endsWith(builtinHeaderSearchPath, "bin/clang"))
			builtinHeaderSearchPath = VuoStringUtilities::substrBefore(builtinHeaderSearchPath, "bin/clang");
		builtinHeaderSearchPath += "lib/clang/" CLANG_VERSION_STRING;
	}
	else
		builtinHeaderSearchPath = vuoFrameworkPath + "/Frameworks/llvm.framework/Versions/A/lib/clang/" CLANG_VERSION_STRING;
	Clang.getHeaderSearchOpts().ResourceDir = builtinHeaderSearchPath;

//	OwningPtr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());  // @@@ return value of takeModule() is destroyed at the end of this function
	clang::CodeGenAction *Act = new clang::EmitLLVMOnlyAction();
	if (!Clang.ExecuteAction(*Act))
		return NULL;

	for (auto i : argsToFree)
		free(i);

	unique_ptr<Module> module = Act->takeModule();
	if (!module)
		VUserLog("Error compiling %s: module is null.", inputPath.c_str());
	return module.release();
}

/**
 * Returns the LLVM module in the `arch` slice of `inputFile`.
 *
 * @threadNoQueue{llvmQueue}
 */
Module *VuoCompiler::readModuleFromBitcode(string inputPath, string arch)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(inputPath, dir, file, ext);
	VuoFileUtilities::File inputFile(dir, file + "." + ext);
	return readModuleFromBitcode(&inputFile, arch);
}

/**
 * Returns the LLVM module in the `arch` slice of `inputFile`.
 *
 * @threadNoQueue{llvmQueue}
 *
 * @throw VuoException The file couldn't be read.
 */
Module *VuoCompiler::readModuleFromBitcode(VuoFileUtilities::File *inputFile, string arch)
{
	size_t inputDataBytes;
	char *inputData = inputFile->getContentsAsRawData(inputDataBytes);

	string error;
	set<string> availableArchs;
	VuoLog_status("Loading module \"%s\" (%s)", inputFile->getRelativePath().c_str(), arch.c_str());
	Module *module = readModuleFromBitcodeData(inputData, inputDataBytes, arch, availableArchs, error);
	VuoLog_status(NULL);
	if (! module)
		VUserLog("Error: Couldn't parse module '%s' (%s): %s.", inputFile->getRelativePath().c_str(), arch.c_str(), error.c_str());

	free(inputData);

	return module;
}

/**
 * Returns the LLVM module in the `arch` slice of `inputData` (a data buffer of size `inputDataBytes`).
 *
 * @threadNoQueue{llvmQueue}
 */
Module *VuoCompiler::readModuleFromBitcodeData(char *inputData, size_t inputDataBytes, string arch,
											   set<string> &availableArchs, string &error)
{
	if (inputDataBytes < sizeof(unsigned int))
		return nullptr;

	__block Module *module = nullptr;
	dispatch_sync(llvmQueue, ^{
		StringRef inputDataAsStringRef(inputData, inputDataBytes);
		auto mb = MemoryBuffer::getMemBuffer(inputDataAsStringRef, "", false);
		if (!mb)
		{
			error = "Couldn't create MemoryBuffer";
			return;
		}

		MemoryBufferRef bitcodeBuffer;
		string moduleArch;
		unsigned int fileID = *(unsigned int *)inputData;
		if (fileID == 0x0b17c0de)
			// This is a single-architecture LLVM bitcode `.bc` file, so read the entire file.
			bitcodeBuffer = mb.get()->getMemBufferRef();

		else if (fileID == 0xdec04342)
		{
			// This is a single-architecture raw bitcode file, presumably generated by Vuo 2.2.1 or earlier.
			bitcodeBuffer = mb.get()->getMemBufferRef();
			moduleArch = "x86_64";
			availableArchs.insert(moduleArch);
		}

		else if (fileID == 0xbebafeca)
		{
			if (arch.empty())
			{
				error = "It's a Mach-O universal binary, but this compiler instance's LLVM target isn't set";
				return;
			}

			// This is a Mach-O wrapper around multiple LLVM bitcode files;
			// parse the Mach-O header to extract just a single architecture.
			auto binary = llvm::object::MachOUniversalBinary::create(mb.get()->getMemBufferRef());
			if (!binary)
			{
				error = "Couldn't read Mach-O universal binary:";
				handleAllErrors(binary.takeError(), [&error](const ErrorInfoBase &ei) {
					error += " " + ei.message();
				});
				return;
			}

			for (auto &o : binary.get()->objects())
			{
				if (o.getArchFlagName() == arch)
					bitcodeBuffer = MemoryBufferRef(mb.get()->getMemBufferRef().getBuffer().slice(o.getOffset(), o.getOffset() + o.getSize()), "");

				availableArchs.insert(o.getArchFlagName());
			}

			if (!bitcodeBuffer.getBufferSize())
			{
				error = "The Mach-O universal binary doesn't have an \"" + arch + "\" slice";
				return;
			}
		}

		auto wrappedModule = llvm::parseBitcodeFile(bitcodeBuffer, *globalLLVMContext);
		if (!wrappedModule)
		{
			error = "Couldn't parse bitcode file:";
			handleAllErrors(wrappedModule.takeError(), [&error](const ErrorInfoBase &ei) {
				error += " " + ei.message();
			});
			return;
		}

		module = wrappedModule.get().release();

		if (moduleArch.empty())
			moduleArch = getTargetArch(module->getTargetTriple());

		if (availableArchs.empty())
			availableArchs.insert(moduleArch);

		if (moduleArch != arch)
		{
			error = "The module's CPU architecture \"" + moduleArch + "\" doesn't match the compiler's CPU architecture \"" + arch + "\"";
			delete module;
			module = nullptr;
			return;
		}
	});
	return module;
}

/**
 * Verifies the LLVM module and writes it to @a outputPath (an LLVM bitcode file).
 *
 * Returns true if there was a problem verifying or writing the module.
 *
 * @threadQueue{llvmQueue}
 */
bool VuoCompiler::writeModuleToBitcode(Module *module, string outputPath)
{
	string str;
	raw_string_ostream verifyOut(str);
	if (llvm::verifyModule(*module, &verifyOut))
	{
		VUserLog("Error: Module verification failed:\n%s", verifyOut.str().c_str());
		return true;
	}

	// Ensure the module gets output in the bitcode wrapper format instead of raw bitcode.
	if (module->getTargetTriple().empty())
		setTargetForModule(module, getProcessTarget());

	std::error_code err;
	raw_fd_ostream out(outputPath.c_str(), err, sys::fs::F_None);
	if (err)
	{
		VUserLog("Error: Couldn't open file '%s' for writing: %s", outputPath.c_str(), err.message().c_str());
		return true;
	}
	llvm::WriteBitcodeToFile(module, out);

	return false;
}

/**
 * Sets the target triple and data layout for @a module, using the architecture and OS
 * from @a targetTriple and a default OS version.
 *
 * @threadQueue{llvmQueue}
 */
void VuoCompiler::setTargetForModule(Module *module, string targetTriple)
{
	// Replace the OS version in targetTriple to avoid warnings about linking modules of different target triples.
	llvm::Triple triple(targetTriple);
	if (triple.isMacOSX())
		triple.setOSName("macosx10.10.0");

	module->setTargetTriple(triple.str());

	string error;
	auto target = TargetRegistry::lookupTarget(module->getTargetTriple(), error);
	if (!target)
	{
		VUserLog("Error: Couldn't look up target: %s", error.c_str());
		return;
	}

	auto targetMachine = target->createTargetMachine(module->getTargetTriple(), "", "", TargetOptions(), Optional<Reloc::Model>());
	if (!targetMachine)
	{
		VUserLog("Error: Couldn't create targetMachine.");
		return;
	}

	module->setDataLayout(targetMachine->createDataLayout());

	delete targetMachine;
}

/**
 * Returns the CPU architecture part of the target triple.
 * E.g., for `x86_64-apple-macosx10.10.0`, returns `x86_64`.
 */
string VuoCompiler::getTargetArch(string target)
{
	auto hyphen = target.find('-');
	if (hyphen == string::npos)
		return "";

	return target.substr(0, hyphen);
}

/**
 * Returns an LLVM target triple compatible with the current process.
 */
string VuoCompiler::getProcessTarget(void)
{
	// llvm::sys::getProcessTriple() returns `LLVM_HOST_TRIPLE`,
	// which is always `x86_64-*` since we built LLVM on x86_64.
	// Instead, use llvm::sys::getDefaultTargetTriple()
	// which _actually_ returns the current process's target.
	llvm::Triple triple(llvm::sys::getDefaultTargetTriple());

	// Replace darwin with macosx, e.g. x86_64-apple-darwin18.7.0 -> x86_64-apple-macosx10.14.6
	if (triple.isMacOSX())
	{
		unsigned int major, minor, micro;
		if (triple.getMacOSXVersion(major, minor, micro))
		{
			ostringstream osVersion;
			osVersion << "macosx" << major << "." << minor << "." << micro;
			triple.setOSName(osVersion.str());
		}
	}

	return triple.str();
}

/**
 * Deallocates @a module and associated data.
 *
 * @threadNoQueue{llvmQueue}
 * @version200New
 */
void VuoCompiler::destroyModule(VuoCompilerModule *module)
{
	Module *llvmModule = module->getModule();

	// In C++ the return value of a cast may not be the same pointer as the cast arg.
	// Because of this, VuoModule::getPseudoBase() returns a different value than VuoNodeClass::getBase().
	// Calling delete on VuoModule::getPseudoBase() gives a malloc error: "pointer being freed was not allocated".
	// So call it on VuoNodeClass::getBase(). Same idea for VuoType.

	VuoNodeClass *baseNodeClass = NULL;
	VuoType *baseType = NULL;
	VuoModule *baseModule = NULL;
	VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
	if (nodeClass)
	{
		baseNodeClass = dynamic_cast<VuoNodeClass *>(nodeClass->getBase());

		/// @todo https://b33p.net/kosada/node/14359 Workaround for crash when destroying node class with unspecialized generic types.
		if (dynamic_cast<VuoCompilerSpecializedNodeClass *>(module))
			module = NULL;
	}
	else
	{
		VuoCompilerType *type = dynamic_cast<VuoCompilerType *>(module);
		if (type)
			baseType = dynamic_cast<VuoType *>(type->getBase());
		else
			baseModule = module->getPseudoBase();
	}

	delete module;
	delete baseNodeClass;
	delete baseType;
	delete baseModule;
	destroyLlvmModule(llvmModule);
}

/**
 * Calls the destructor for @a module, using the LLVM global context thread-safely.
 *
 * @threadNoQueue{llvmQueue}
 * @version200New
 */
void VuoCompiler::destroyLlvmModule(Module *module)
{
	dispatch_sync(llvmQueue, ^{
					  delete module;
				  });
}

/**
 * Instantiates a node for the given node class.
 *
 * If the node class is a generic template (e.g. vuo.data.hold), then the node is instantiated with the specialized
 * version of the node class (e.g. vuo.data.hold.VuoGenericType1).
 *
 * @param nodeClass The node class from which to create the node.
 * @param title The node's title.
 * @param x The node's x-coordinate within the composition.
 * @param y The node's y-coordinate within the composition.
 */
VuoNode * VuoCompiler::createNode(VuoCompilerNodeClass *nodeClass, string title, double x, double y)
{
	VuoCompilerNodeClass *nodeClassForNode = VuoCompilerSpecializedNodeClass::getNodeClassForNode(nodeClass, this);
	if (nodeClassForNode)
		return nodeClassForNode->newNode(title, x, y);

	return nullptr;
}

/**
 * Instantiates a node for the given node class.
 *
 * If the node class is a generic template (e.g. vuo.data.hold), then the node is instantiated with the specialized
 * version of the node class (e.g. vuo.data.hold.VuoGenericType1).
 */
VuoNode * VuoCompiler::createNode(VuoCompilerNodeClass *nodeClass, VuoNode *nodeToCopyMetadataFrom)
{
	VuoCompilerNodeClass *nodeClassForNode = VuoCompilerSpecializedNodeClass::getNodeClassForNode(nodeClass, this);
	if (nodeClassForNode)
		return nodeClassForNode->newNode(nodeToCopyMetadataFrom);

	return nullptr;
}

/**
 * Instantiates a node used during code generation to represent published input ports.
 */
VuoNode * VuoCompiler::createPublishedInputNode(vector<VuoPublishedPort *> publishedInputPorts)
{
	string nodeClassName = VuoCompilerPublishedInputNodeClass::buildNodeClassName(publishedInputPorts);
	return createPublishedNode(nodeClassName, publishedInputPorts);
}

/**
 * Instantiates a node used during code generation to represent published output ports.
 */
VuoNode * VuoCompiler::createPublishedOutputNode(vector<VuoPublishedPort *> publishedOutputPorts)
{
	string nodeClassName = VuoCompilerPublishedOutputNodeClass::buildNodeClassName(publishedOutputPorts);
	return createPublishedNode(nodeClassName, publishedOutputPorts);
}

/**
 * Helper for @ref createPublishedInputNode and @ref createPublishedOutputNode.
 */
VuoNode * VuoCompiler::createPublishedNode(const string &nodeClassName, const vector<VuoPublishedPort *> &publishedPorts)
{
	VuoCompilerNodeClass *nodeClass = getNodeClass(nodeClassName);
	VuoNode *node = createNode(nodeClass);

	// Set the generic port types on the node to match those on the published ports set by VuoCompilerComposition::updateGenericPortTypes().
	VuoCompilerPublishedInputNodeClass *inputNodeClass = dynamic_cast<VuoCompilerPublishedInputNodeClass *>(nodeClass);
	VuoCompilerPublishedOutputNodeClass *outputNodeClass = dynamic_cast<VuoCompilerPublishedOutputNodeClass *>(nodeClass);
	for (size_t i = 0; i < publishedPorts.size(); ++i)
	{
		VuoType *publishedPortType = static_cast<VuoCompilerPort *>(publishedPorts[i]->getCompiler())->getDataVuoType();
		if (! dynamic_cast<VuoGenericType *>(publishedPortType))
			continue;

		set<VuoPort *> nodePorts;
		if (inputNodeClass)
		{
			VuoPort *inputPort = node->getInputPorts().at( inputNodeClass->getInputPortIndexForPublishedInputPort(i) );
			nodePorts.insert(inputPort);

			VuoPort *outputPort = node->getOutputPorts().at( inputNodeClass->getOutputPortIndexForPublishedInputPort(i) );
			nodePorts.insert(outputPort);
		}
		else
		{
			VuoPort *inputPort = node->getInputPorts().at( outputNodeClass->getInputPortIndexForPublishedOutputPort(i) );
			nodePorts.insert(inputPort);
		}

		for (VuoPort *port : nodePorts)
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
			compilerPort->setDataVuoType(publishedPortType);
		}
	}

	reifyGenericPortTypes(node);

	return node;
}

/**
 * Causes the node class source code located at @a sourcePath to be loaded at composition-family scope —
 * unless this compiler's composition path is within the user, system, or built-in scope, in which case does nothing.
 */
void VuoCompiler::installNodeClassAtCompositionLocalScope(const string &sourcePath)
{
	dispatch_sync(environmentQueue, ^{
		if (environments.size() >= 5)
			environments.at(3).at(0)->addExpatriateSourceFile(sourcePath);
	});
}

/**
 * Causes the node class that was previously loaded from @a sourcePath by @ref VuoCompiler::installNodeClassAtCompositionLocalScope,
 * if any, to be unloaded.
 */
void VuoCompiler::uninstallNodeClassAtCompositionLocalScope(const string &sourcePath)
{
	dispatch_sync(environmentQueue, ^{
		if (environments.size() >= 5)
		{
			environments.at(3).at(0)->removeExpatriateSourceFile(sourcePath);

			set<string> sourcesRemoved;
			sourcesRemoved.insert(getModuleKeyForPath(sourcePath));
			loadModulesAndSources(set<string>(), set<string>(), set<string>(), set<string>(), set<string>(), sourcesRemoved,
								  false, false, environments.at(3).at(0), nullptr, nullptr, "");
		}
	});
}

/**
 * Temporarily replaces the subcomposition installed at @a sourcePath with a subcomposition
 * compiled from @a sourceCode. The newly compiled subcomposition becomes the active version,
 * overriding the installed subcomposition.
 *
 * This function can be called multiple times in succession, replacing one override with another.
 *
 * The most recent override remains active until the installed subcomposition is saved or
 * @ref VuoCompiler::revertOverriddenNodeClass is called.
 *
 * A call to this function may or may not result in a call to @ref VuoCompilerDelegate::loadedModules.
 * The node class won't be reloaded if @a sourceCode is the same as source code from which the module
 * was most recently loaded.
 *
 * @version200New
 */
void VuoCompiler::overrideInstalledNodeClass(const string &sourcePath, const string &sourceCode)
{
	string sourcePathCopy = sourcePath;
	string sourceCodeCopy = sourceCode;

	dispatch_async(environmentQueue, ^{
		string nodeClassName = getModuleKeyForPath(sourcePathCopy);
		ModuleInfo *sourceInfo = NULL;

		for (const vector<Environment *> &envs : environments)
		{
			for (Environment *env : envs)
			{
				ModuleInfo *potentialSourceInfo = env->listSourceFile(nodeClassName);
				if (potentialSourceInfo && VuoFileUtilities::arePathsEqual(potentialSourceInfo->getFile()->path(), sourcePathCopy))
				{
					sourceInfo = potentialSourceInfo;
					break;
				}
			}
		}

		if (! sourceInfo)
			return;

		bool shouldRecompileSourcesIfUnchanged;
		if (! sourceCodeCopy.empty())
		{
			sourceInfo->setSourceCode(sourceCodeCopy);
			sourceInfo->setSourceCodeOverridden(true);

			shouldRecompileSourcesIfUnchanged = false;
		}
		else
		{
			sourceInfo->revertSourceCode();
			sourceInfo->setSourceCodeOverridden(false);

			shouldRecompileSourcesIfUnchanged = true;
		}
		sourceInfo->setAttempted(false);
		sourceInfo->setLastModifiedToNow();

		set<string> sourcesModified;
		sourcesModified.insert(nodeClassName);

		loadModulesAndSources(set<string>(), set<string>(), set<string>(), set<string>(), sourcesModified, set<string>(),
							  false, shouldRecompileSourcesIfUnchanged, nullptr, nullptr, nullptr, "");
	});
}

/**
 * Removes an override added by @ref VuoCompiler::overrideInstalledNodeClass (if any), reverting to the
 * installed version of the node class.
 *
 * A call to this function always results in a call to @ref VuoCompilerDelegate::loadedModules,
 * even if the node class's source code was not overridden.
 *
 * @version200New
 */
void VuoCompiler::revertOverriddenNodeClass(const string &sourcePath)
{
	overrideInstalledNodeClass(sourcePath, "");
}

/**
 * Returns the node class specified by @a nodeClassName, or null if it can't be found or loaded.
 *
 * The node class module is loaded or generated if it hasn't been already.
 *
 * @eg{
 *		VuoCompiler *compiler = new VuoCompiler();
 *		VuoCompilerNodeClass *nc = compiler->getNodeClass("vuo.math.add.VuoInteger");
 *		[...]
 *		delete compiler;
 * }
 */
VuoCompilerNodeClass * VuoCompiler::getNodeClass(const string &nodeClassName)
{
	// For performance, don't bother searching if it's obviously not a node class.

	if (VuoStringUtilities::endsWith(nodeClassName, ".framework"))
		return NULL;

	// Attempt to load the node class (if it hasn't already been loaded).

	set<string> nodeClassNameSet;
	nodeClassNameSet.insert(nodeClassName);
	loadModulesIfNeeded(nodeClassNameSet);

	// If the node class has been loaded, return it.

	__block VuoCompilerNodeClass *nodeClass = NULL;
	void (^envGetNodeClass)(Environment *) = ^void (Environment *env) {
		VuoCompilerNodeClass *result = env->getNodeClass(nodeClassName);
		if (result)
			nodeClass = result;
	};
	applyToAllEnvironments(envGetNodeClass);
	return nodeClass;
}

/**
 * Returns all node classes found and loaded, indexed by node class name.
 *
 * The node class modules are loaded if they haven't been already.
 */
map<string, VuoCompilerNodeClass *> VuoCompiler::getNodeClasses()
{
	loadModulesIfNeeded();

	__block map<string, VuoCompilerNodeClass *> nodeClasses;
	void (^envGetNodeClasses)(Environment *) = ^void (Environment *env) {
		map<string, VuoCompilerNodeClass *> result = env->getNodeClasses();
		nodeClasses.insert(result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetNodeClasses);
	return nodeClasses;
}

/**
 * Returns the type specified by @a typeName, or null if it can't be found or loaded.
 *
 * The type module is loaded or generated if it haven't been already.
 */
VuoCompilerType * VuoCompiler::getType(const string &typeName)
{
	set<string> typeNameSet;
	typeNameSet.insert(typeName);
	loadModulesIfNeeded(typeNameSet);

	__block VuoCompilerType *type = NULL;
	void (^envGetType)(Environment *) = ^void (Environment *env) {
		VuoCompilerType *result = env->getType(typeName);
		if (result)
			type = result;
	};
	applyToInstalledEnvironments(envGetType);

	if (! type && VuoGenericType::isGenericTypeName(typeName))
	{
		VuoCompilerType * (^compilerGetType)(string) = ^VuoCompilerType * (string moduleKey) {
			return getType(moduleKey);
		};

		VuoGenericType *genericType = new VuoGenericType(typeName, vector<string>());
		type = VuoCompilerGenericType::newGenericType(genericType, compilerGetType);
	}

	return type;
}

/**
 * Returns all types found and loaded, indexed by type name.
 *
 * The type modules are loaded if they haven't been already.
 */
map<string, VuoCompilerType *> VuoCompiler::getTypes()
{
	loadModulesIfNeeded();

	__block map<string, VuoCompilerType *> types;
	void (^envGetTypes)(Environment *) = ^void (Environment *env) {
		map<string, VuoCompilerType *> result = env->getTypes();
		types.insert(result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetTypes);
	return types;
}

/**
 * Returns the library specified by `libraryName`, or null if it can't be found or loaded.
 *
 * The library is loaded if it haven't been already.
 */
VuoCompilerModule *VuoCompiler::getLibraryModule(const string &libraryModuleName)
{
	set<string> libraryNameSet;
	libraryNameSet.insert(libraryModuleName);
	loadModulesIfNeeded(libraryNameSet);

	__block VuoCompilerModule *module = nullptr;
	void (^envGetLibraryModule)(Environment *) = ^void (Environment *env) {
		VuoCompilerModule *result = env->getLibraryModule(libraryModuleName);
		if (result)
			module = result;
	};
	applyToInstalledEnvironments(envGetLibraryModule);

	return module;
}

/**
 * Returns all libraries found and loaded, indexed by key.
 *
 * The libraries are loaded if they haven't been already.
 */
map<string, VuoCompilerModule *> VuoCompiler::getLibraryModules()
{
	loadModulesIfNeeded();

	__block map<string, VuoCompilerModule *> libraryModules;
	void (^envGetLibraryModules)(Environment *) = ^void (Environment *env) {
		map<string, VuoCompilerModule *> result = env->getLibraryModules();
		libraryModules.insert(result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetLibraryModules);
	return libraryModules;
}

/**
 * Returns all node sets found and loaded, indexed by node set name.
 *
 * The node class modules are loaded if they haven't been already.
 */
map<string, VuoNodeSet *> VuoCompiler::getNodeSets()
{
	loadModulesIfNeeded();

	__block map<string, VuoNodeSet *> nodeSets;
	void (^envGetNodeSets)(Environment *) = ^void (Environment *env) {
		map<string, VuoNodeSet *> result = env->getNodeSets();
		nodeSets.insert(result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetNodeSets);
	return nodeSets;
}

/**
 * Returns the node set with the given @a name, or null if it can't be found.
 *
 * The node class modules are loaded if they haven't been already.
 */
VuoNodeSet * VuoCompiler::getNodeSetForName(const string &name)
{
	loadModulesIfNeeded();

	__block VuoNodeSet *nodeSet = NULL;
	void (^envFindNodeSet)(Environment *) = ^void (Environment *env) {
		VuoNodeSet *result = env->findNodeSet(name);
		if (result)
			nodeSet = result;
	};
	applyToInstalledEnvironments(envFindNodeSet);
	return nodeSet;
}

/**
 * Looks up the VuoCompilerNodeClass, VuoCompilerType, or VuoCompilerModule specified by @a moduleKey.
 */
VuoCompilerModule * VuoCompiler::getModule(const string &moduleKey)
{
	__block VuoCompilerModule *module = NULL;
	void (^envFindModule)(Environment *) = ^void (Environment *env) {
		VuoCompilerModule *result = env->findModule(moduleKey);
		if (result)
			module = result;
	};
	applyToAllEnvironments(envFindModule);
	return module;
}

/**
 * Prints a list of all loaded node classes to standard output.
 *
 * The node class modules are loaded if they haven't been already.
 *
 * @param format The format for printing the node classes.
 *	- If "", prints each class name (e.g. vuo.math.count.VuoInteger), one per line.
 *	- If "dot", prints the declaration of a node as it would appear in a .vuo (DOT format) file,
 *		with a constant value set for each data+event input port
 *		and a comment listing metadata and port types for the node class.
 */
void VuoCompiler::listNodeClasses(const string &format)
{
	map<string, VuoCompilerNodeClass *> nodeClasses = getNodeClasses();
	for (map<string, VuoCompilerNodeClass *>::const_iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
	{
		VuoCompilerNodeClass *nodeClass = i->second;
		if (format == "")
		{
			printf("%s\n", nodeClass->getBase()->getClassName().c_str());
		}
		else if (format == "path")
		{
			// TODO: If "path", prints the absolute path of each node class, one per line.
		}
		else if (format == "dot")
		{
			VuoCompilerNode *node = nodeClass->newNode()->getCompiler();

			printf("%s\n", nodeClass->getDoxygenDocumentation().c_str());
			printf("%s\n\n", node->getGraphvizDeclaration().c_str());

			delete node;
		}
	}
}

/**
 * Returns the run-path search paths (a.k.a. `-rpath` arguments) that need to be passed when linking
 * a dynamic library or executable so that it can locate the dynamic libraries that it depends on
 * (the built-in dylibs in Vuo.framework and any dylibs provided with custom node classes).
 *
 * @param narrowestScope The original or generated environment at the narrowest level of scope for which
 *     run-path search paths should be returned.
 * @return The run-path search paths in order from narrowest to broadest scope (i.e., the order in which
 *     they should be passed to the linker).
 */
vector<string> VuoCompiler::getRunPathSearchPaths(Environment *narrowestScope)
{
	vector<string> rPaths;
	for (size_t i = 0; i < environments.size(); ++i)
	{
		if (i == 0)
		{
			string vuoFrameworkPath = getVuoFrameworkPath();
			if (! vuoFrameworkPath.empty())
				rPaths.push_back(vuoFrameworkPath + "/..");
		}
		else if (i == 1)
			rPaths.push_back(VuoFileUtilities::getSystemModulesPath());
		else if (i == 2)
			rPaths.push_back(VuoFileUtilities::getUserModulesPath());
		else if (i == 3)
		{
			string localModulesPath = getCompositionLocalModulesPath();
			if (! localModulesPath.empty())
				rPaths.push_back(localModulesPath);
		}

		if (std::find(environments[i].begin(), environments[i].end(), narrowestScope) != environments[i].end())
			break;
	}

	std::reverse(rPaths.begin(), rPaths.end());
	return rPaths;
}

/**
 * Returns the file names of bitcode dependencies needed by every linked Vuo composition.
 *
 * These dependencies are added to the cache (`libVuoResources.dylib`)
 * and are thus only loaded once per process.
 */
vector<string> VuoCompiler::getCoreVuoDependencies(void)
{
	vector<string> dependencies;

	dependencies.push_back("VuoHeap");
	dependencies.push_back("VuoApp");

	dependencies.push_back("zmq");
	dependencies.push_back("json-c");
	dependencies.push_back("objc");
	dependencies.push_back("c");
	dependencies.push_back("AppKit.framework");

#ifdef COVERAGE
	dependencies.push_back(LLVM_ROOT "/lib/libprofile_rt.dylib");
#endif
	return dependencies;
}

/**
 * Returns the file name of the main function can accompany the Vuo runtime.
 */
string VuoCompiler::getRuntimeMainDependency(void)
{
	return "VuoRuntimeMain.o";
}

/**
 * Returns the file name of the Vuo runtime.
 *
 * This dependency is NOT added to the cache (`libVuoResources.dylib`)
 * and is thus loaded individually for each composition within a process,
 * giving each composition its own set of global/static variables defined in this dependency.
 */
string VuoCompiler::getRuntimeDependency(void)
{
	return "VuoRuntime.o";
}

/**
 * Returns the absolute path of Vuo.framework.
 *
 * This returns the same thing @ref VuoFileUtilities::getVuoFrameworkPath except during a call
 * to @ref VuoCompiler::generateBuiltInModuleCaches.
 */
string VuoCompiler::getVuoFrameworkPath(void)
{
	if (! vuoFrameworkInProgressPath.empty())
		return vuoFrameworkInProgressPath;

	return VuoFileUtilities::getVuoFrameworkPath();
}

/**
 * Returns the path to the Clang binary.
 */
string VuoCompiler::getClangPath(void)
{
	return clangPath;
}

/**
 * Returns the name of the node class or other module that would be located at @c path.
 */
string VuoCompiler::getModuleKeyForPath(string path)
{
	// Start with the file name without extension.
	string dir, moduleKey, ext;
	VuoFileUtilities::splitPath(path, dir, moduleKey, ext);

	if (VuoFileUtilities::isIsfSourceExtension(ext))
	{
		vector<string> nodeClassNameParts = VuoStringUtilities::split(moduleKey, '.');

		// Remove repeated file extensions.
		while (nodeClassNameParts.size() > 1 && nodeClassNameParts.back() == ext)
			nodeClassNameParts.pop_back();

		// Convert each part to lowerCamelCase.
		for (string &part : nodeClassNameParts)
			part = VuoStringUtilities::convertToCamelCase(part, false, true, false);

		// If the node class name has only one part, add a prefix.
		if (nodeClassNameParts.size() == 1)
			nodeClassNameParts.insert(nodeClassNameParts.begin(), "isf");

		moduleKey = VuoStringUtilities::join(nodeClassNameParts, '.');
	}

	return moduleKey;
}

/**
 * Returns true if the module is installed in the composition-local Modules folder for
 * this compiler's composition path.
 */
bool VuoCompiler::isCompositionLocalModule(string moduleKey)
{
	__block bool isLocal = false;

	dispatch_sync(environmentQueue, ^{
		if (environments.size() >= 5)
			isLocal = environments.at(3).at(0)->findModule(moduleKey);
	});

	return isLocal;
}

/**
 * Returns the path of the composition-local Modules folder for this compiler's composition path,
 * or an empty string if the composition path hasn't been set.
 *
 * @version200New
 */
string VuoCompiler::getCompositionLocalModulesPath(void)
{
	return lastCompositionBaseDir.empty() ? "" : lastCompositionBaseDir + "/Modules";
}

/**
 * Returns the path of the top-level composition folder for this compiler's composition path,
 * or an empty string if the composition path hasn't been set.
 *
 * If the composition is a composition-local subcomposition (it's located in a folder called Modules
 * other than the Built-in, User, and System Modules folders), returns the folder containing the
 * Modules folder. Otherwise, returns the folder containing the composition.
 *
 * @version200New
 */
string VuoCompiler::getCompositionLocalPath(void)
{
	return lastCompositionBaseDir;
}

/**
 * Adds a module search path (for node classes, types, and libraries) to use when linking a composition.
 */
void VuoCompiler::addModuleSearchPath(string path)
{
	dispatch_sync(environmentQueue, ^{
		environments.back().at(0)->addModuleSearchPath(path);
		environments.back().at(0)->addLibrarySearchPath(path);
	});
}

/**
 * Adds a header search path to use when compiling a node class.
 */
void VuoCompiler::addHeaderSearchPath(const string &path)
{
	dispatch_sync(environmentQueue, ^{
		environments.back().at(0)->addHeaderSearchPath(path);
	});
}

/**
 * Adds a library search path to use when linking a composition.
 */
void VuoCompiler::addLibrarySearchPath(const string &path)
{
	dispatch_sync(environmentQueue, ^{
		environments.back().at(0)->addLibrarySearchPath(path);
	});
}

/**
 * Adds a macOS framework search path to use when linking a composition.
 */
void VuoCompiler::addFrameworkSearchPath(const string &path)
{
	dispatch_sync(environmentQueue, ^{
		environments.back().at(0)->addFrameworkSearchPath(path);
	});
}

/**
 * Sets the verbosity to use when compiling or linking. If true, prints some debug info and passes the `-v` option to Clang.
 */
void VuoCompiler::setVerbose(bool isVerbose)
{
	this->isVerbose = isVerbose;
}

/**
 * Sets whether the compiled composition should show the Vuo Community Edition splash window,
 * if generated by Vuo Community Edition.
 */
void VuoCompiler::setShouldPotentiallyShowSplashWindow(bool potentiallyShow)
{
#if VUO_PRO
	if (VuoPro::getProAccess())
	{
		_shouldShowSplashWindow = false;
		return;
	}
#endif

	_shouldShowSplashWindow = potentiallyShow;
}

/**
 * Specifies the path at which to create a GCC-style Makefile-depfile
 * that lists the source/header files this module depends on.
 * Only effective when compiling C/C++/Objective-C files.
 */
void VuoCompiler::setDependencyOutput(const string &path)
{
#if VUO_PRO
	_setDependencyOutput(path);
#endif
}

/**
 * Returns whether the compiled composition should show the Vuo Community Edition splash window.
 */
bool VuoCompiler::shouldShowSplashWindow()
{
	return _shouldShowSplashWindow;
}

/**
 * Sets the path to the clang binary.
 */
void VuoCompiler::setClangPath(const string &clangPath)
{
	this->clangPath = clangPath;
}

/**
 * Returns the path to the VuoCompositionLoader executable.
 */
string VuoCompiler::getCompositionLoaderPath(void)
{
	string vuoFrameworkPath = getVuoFrameworkPath();
	return (vuoFrameworkPath.empty() ?
				VUO_BUILD_DIR "/bin/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader" :
				vuoFrameworkPath + "/Helpers/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader");
}

/**
 * Returns the path to the VuoCompositionStub dynamic library.
 */
string VuoCompiler::getCompositionStubPath(void)
{
	string vuoFrameworkPath = getVuoFrameworkPath();
	return (vuoFrameworkPath.empty() ?
				VUO_BUILD_DIR "/lib/libVuoCompositionStub.dylib" :
				vuoFrameworkPath + "/Modules/libVuoCompositionStub.dylib");
}

/**
 * Returns the absolute path to the local cache directory for a composition.
 */
string VuoCompiler::getCachePathForComposition(const string compositionDir)
{
	string modifierLetterColon("꞉");
	string cachedModulesName = compositionDir;
	VuoStringUtilities::replaceAll(cachedModulesName, "/", modifierLetterColon);
	return VuoFileUtilities::getCachePath() + "/" + cachedModulesName;
}

/**
 * Prints info about this compiler, for debugging.
 */
void VuoCompiler::print(void)
{
	__block vector<string> moduleSearchPaths;
	void (^envGetModuleSearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getModuleSearchPaths();
		moduleSearchPaths.insert(moduleSearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetModuleSearchPaths);

	__block vector<string> headerSearchPaths;
	void (^envGetHeaderSearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getHeaderSearchPaths();
		headerSearchPaths.insert(headerSearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetHeaderSearchPaths);

	__block vector<string> librarySearchPaths;
	void (^envGetLibrarySearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetLibrarySearchPaths);

	__block vector<string> frameworkSearchPaths;
	void (^envGetFrameworkSearchPaths)(Environment *) = ^void (Environment *env) {
		vector<string> result = env->getFrameworkSearchPaths();
		frameworkSearchPaths.insert(frameworkSearchPaths.end(), result.begin(), result.end());
	};
	applyToInstalledEnvironments(envGetFrameworkSearchPaths);

	fprintf(stderr, "Module (node class, type, library) search paths:\n");
	for (vector<string>::iterator i = moduleSearchPaths.begin(); i != moduleSearchPaths.end(); ++i)
		fprintf(stderr, " %s\n", (*i).c_str());
	fprintf(stderr, "Header search paths:\n");
	for (vector<string>::iterator i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
		fprintf(stderr, " %s\n", (*i).c_str());
	fprintf(stderr, "Other library search paths:\n");
	for (vector<string>::iterator i = librarySearchPaths.begin(); i != librarySearchPaths.end(); ++i)
		fprintf(stderr, " %s\n", (*i).c_str());
	fprintf(stderr, "Other framework search paths:\n");
	for (vector<string>::iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
		fprintf(stderr, " %s\n", (*i).c_str());
	fprintf(stderr, "Framework path:\n");
	if (! getVuoFrameworkPath().empty())
		fprintf(stderr, " %s\n", getVuoFrameworkPath().c_str());
	fprintf(stderr, "Clang path:\n");
	if (! getClangPath().empty())
		fprintf(stderr, " %s\n", getClangPath().c_str());
}

/**
 * Creates a runner object that can run the composition in file @a compositionFilePath in a new process.
 *
 * If there's an error compiling or linking the composition, the error is added to @a issues and this function returns null.
 * Any warnings are also added to @a issues.
 *
 * @version200Changed{Added `issues` argument.}
 */
VuoRunner * VuoCompiler::newSeparateProcessRunnerFromCompositionFile(string compositionFilePath, VuoCompilerIssues *issues)
{
	try
	{
		VuoCompiler compiler(compositionFilePath);
		string directory, file, extension;
		VuoFileUtilities::splitPath(compositionFilePath, directory, file, extension);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file + "-linked", "");
		compiler.compileComposition(compositionFilePath, compiledCompositionPath, true, issues);
		compiler.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		return VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, directory, false, true);
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());
		return NULL;
	}
}

/**
 * Creates a runner object that can run the composition in string @a composition in a new process.
 *
 * If there's an error compiling or linking the composition, the error is added to @a issues and this function returns null.
 * Any warnings are also added to @a issues.
 *
 * @param composition A serialized composition.
 * @param processName The executable filename that the running composition should use.
 * @param workingDirectory The compiling and linking steps and the running composition behave as if the composition is in
 *     this directory. The directory is used by when compiling and linking to search for composition-local nodes, and
 *     it's used by nodes in the running composition to resolve relative paths.
 * @param issues Problems encountered while compiling and linking.
 * @version200Changed{Added `processName`, `issues` arguments.}
 */
VuoRunner * VuoCompiler::newSeparateProcessRunnerFromCompositionString(string composition, string processName, string workingDirectory, VuoCompilerIssues *issues)
{
	try
	{
		string compositionPath = (workingDirectory.empty() ? "." : workingDirectory) + "/" + processName + ".vuo";
		VuoCompiler compiler(compositionPath);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(processName, "bc");
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(processName, "");
		compiler.compileCompositionString(composition, compiledCompositionPath, true, issues);
		compiler.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		return VuoRunner::newSeparateProcessRunnerFromExecutable(linkedCompositionPath, workingDirectory, false, true);
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());
		return NULL;
	}
}

/**
 * Creates a runner object that can run the composition in file @a compositionFilePath in this process.
 *
 * If there's an error compiling or linking the composition, the error is added to @a issues and this function returns null.
 * Any warnings are also added to @a issues.
 *
 * @version200Changed{Added `issues` argument.}
 */
VuoRunner * VuoCompiler::newCurrentProcessRunnerFromCompositionFile(string compositionFilePath, VuoCompilerIssues *issues)
{
	try
	{
		VuoCompiler compiler(compositionFilePath);
		string directory, file, extension;
		VuoFileUtilities::splitPath(compositionFilePath, directory, file, extension);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		compiler.compileComposition(compositionFilePath, compiledCompositionPath, true, issues);
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		compiler.linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		return VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, directory, true);
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());
		return NULL;
	}
}

/**
 * Creates a runner object that can run the composition in string @a composition in this process.
 *
 * If there's an error compiling or linking the composition, the error is added to @a issues and this function returns null.
 * Any warnings are also added to @a issues.
 *
 * @param composition A serialized composition.
 * @param workingDirectory The compiling and linking steps and the running composition behave as if the composition is in
 *     this directory. The directory is used by when compiling and linking to search for composition-local nodes, and
 *     it's used by nodes in the running composition to resolve relative paths.
 * @param issues Problems encountered while compiling and linking.
 *
 * @version200Changed{Added `issues` argument.}
 */
VuoRunner * VuoCompiler::newCurrentProcessRunnerFromCompositionString(string composition, string workingDirectory, VuoCompilerIssues *issues)
{
	try
	{
		string compositionPath = (workingDirectory.empty() ? "." : workingDirectory) + "/UntitledComposition.vuo";
		VuoCompiler compiler(compositionPath);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "bc");
		compiler.compileCompositionString(composition, compiledCompositionPath, true, issues);
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile("VuoRunnerComposition", "dylib");
		compiler.linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());
		return VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, workingDirectory, true);
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());
		return NULL;
	}
}

VuoCompilerDelegate::VuoCompilerDelegate(void)
{
	pendingDataQueue = dispatch_queue_create("org.vuo.compiler.delegate.pending", 0);
}

void VuoCompilerDelegate::loadedModulesCompleted(void)
{
	LoadedModulesData *data = dequeueData();
	data->release();
}

/**
 * Stores data before a call to @ref loadedModules so it can be retrieved later when it's ready to be released.
 */
void VuoCompilerDelegate::enqueueData(LoadedModulesData *data)
{
	dispatch_sync(pendingDataQueue, ^{
					  pendingData.push_back(data);
				  });
}

/**
 * Dequeues the next stored data and returns it.
 */
VuoCompilerDelegate::LoadedModulesData * VuoCompilerDelegate::dequeueData(void)
{
	__block LoadedModulesData *ret;
	dispatch_sync(pendingDataQueue, ^{
					  ret = pendingData.front();
					  pendingData.pop_front();
				  });
	return ret;
}

/**
 * Constructs an instance with reference count 0, storing data so that it can be destroyed later
 * when all VuoCompilerDelegate instances have finished using it.
 */
VuoCompilerDelegate::LoadedModulesData::LoadedModulesData(const set< pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
														  const set<VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
{
	referenceCountQueue = dispatch_queue_create("org.vuo.compiler.delegate.reference", 0);
	referenceCount = 0;

	this->modulesModified = modulesModified;
	this->modulesRemoved = modulesRemoved;
	this->issues = issues;
}

/**
 * Destroys the stored data.
 */
VuoCompilerDelegate::LoadedModulesData::~LoadedModulesData(void)
{
	delete issues;

	for (set< pair<VuoCompilerModule *, VuoCompilerModule *> >::iterator i = modulesModified.begin(); i != modulesModified.end(); ++i)
		VuoCompiler::destroyModule((*i).first);

	for (set<VuoCompilerModule *>::iterator i = modulesRemoved.begin(); i != modulesRemoved.end(); ++i)
		VuoCompiler::destroyModule(*i);
}

/**
 * Increments the reference count.
 */
void VuoCompilerDelegate::LoadedModulesData::retain(void)
{
	dispatch_sync(referenceCountQueue, ^{
					  ++referenceCount;
				  });
}

/**
 * Decrements the reference count.
 */
void VuoCompilerDelegate::LoadedModulesData::release(void)
{
	dispatch_sync(referenceCountQueue, ^{
					  --referenceCount;
					  if (referenceCount == 0) {
						  delete this;
					  }
				  });
}
