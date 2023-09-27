/**
 * @file
 * VuoCompiler implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompiler.hh"

#include <dlfcn.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoClangIssues.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerDelegate.hh"
#include "VuoCompilerDiagnosticConsumer.hh"
#include "VuoCompilerEnvironment.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoComposition.hh"
#include "VuoDependencyGraphVertex.hh"
#include "VuoException.hh"
#include "VuoGenericType.hh"
#include "VuoMakeDependencies.hh"
#include "VuoModuleCache.hh"
#include "VuoModuleCacheRevision.hh"
#include "VuoModuleCompiler.hh"
#include "VuoModuleCompilerSettings.hh"
#include "VuoModuleInfo.hh"
#include "VuoModuleInfoIterator.hh"
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

set<VuoCompiler *> VuoCompiler::allCompilers;
dispatch_queue_t VuoCompiler::environmentQueue = dispatch_queue_create("org.vuo.compiler.environment", NULL);
map<string, vector< vector<VuoCompilerEnvironment *> > > VuoCompiler::sharedEnvironments;
map<string, map<string, vector<VuoCompilerEnvironment *> > > VuoCompiler::environmentsForCompositionFamily;
map<VuoCompilerEnvironment *, map<string, pair<VuoCompilerModule *, dispatch_group_t>>> VuoCompiler::invalidatedModulesAwaitingRecompilation;
map<VuoCompilerEnvironment *, set<VuoCompilerModule *>> VuoCompiler::addedModulesAwaitingReification;
map<VuoCompilerEnvironment *, set<pair<VuoCompilerModule *, VuoCompilerModule *>>> VuoCompiler::modifiedModulesAwaitingReification;
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
 * Calls @a doForEnvironment for each of the installed environments, in order from broadest to narrowest scope.
 */
void VuoCompiler::applyToInstalledEnvironments(std::function<void(VuoCompilerEnvironment *)> doForEnvironment)
{
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<VuoCompilerEnvironment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  doForEnvironment((*i)[0]);
					  }
				  });
}

/**
 * Calls @a doForEnvironment for each of the installed and generated environments, in order from broadest to narrowest scope.
 */
void VuoCompiler::applyToAllEnvironments(std::function<void(VuoCompilerEnvironment *)> doForEnvironment)
{
	dispatch_sync(environmentQueue, ^{
					  for (vector< vector<VuoCompilerEnvironment *> >::iterator i = environments.begin(); i != environments.end(); ++i) {
						  for (vector<VuoCompilerEnvironment *>::iterator j = i->begin(); j != i->end(); ++j) {
							  doForEnvironment(*j);
						  }
					  }
				  });
}

/**
 * Returns whichever of @a envA or @a envB is at a broader level of scope, or @a envA if both environments are at the same level,
 * or null if neither environment is available to this VuoCompiler instance.
 */
VuoCompilerEnvironment * VuoCompiler::environmentAtBroaderScope(VuoCompilerEnvironment *envA, VuoCompilerEnvironment *envB)
{
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		if (find(environmentsAtScope.begin(), environmentsAtScope.end(), envA) != environmentsAtScope.end())
			return envA;
		if (find(environmentsAtScope.begin(), environmentsAtScope.end(), envB) != environmentsAtScope.end())
			return envB;
	}

	return nullptr;
}

/**
 * Returns the generated environment at the same level of scope as @a env (which may be @a env itself).
 */
VuoCompilerEnvironment * VuoCompiler::generatedEnvironmentAtSameScopeAs(VuoCompilerEnvironment *env)
{
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		if (find(environmentsAtScope.begin(), environmentsAtScope.end(), env) != environmentsAtScope.end())
			return environmentsAtScope.back();

	return nullptr;
}


/**
 * Creates a compiler instance that can be used for compiling and linking.
 *
 * @param compositionPath If this compiler will be compiling a composition and its path is already known,
 *     pass the path so the compiler can locate composition-local modules. If the path is not yet known,
 *     it can be set later with VuoCompiler::setCompositionPath() or VuoCompiler::compileComposition().
 *     If not compiling a composition, pass an empty string.
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
		target = getProcessTarget();

		// Match the target macOS version of the built-in modules and precompiled headers.
		llvm::Triple triple(target);
		triple.setOSName("macosx10.10.0");
		target = triple.str();

		VDebugLog("%p  target=%s (from current process)", this, target.c_str());
	}
	else
	{
		requestedTarget = target;
		VDebugLog("%p  target=%s", this, target.c_str());
	}
	this->target = target;

#if VUO_PRO
	init_Pro();
#endif

	shouldLoadAllModules = true;
	hasLoadedAllModules = false;
	modulesToLoadQueue = dispatch_queue_create("org.vuo.compiler.modules", NULL);
	modulesLoading = dispatch_group_create();
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
						  sharedEnvironments[target] = vector< vector<VuoCompilerEnvironment *> >(3, vector<VuoCompilerEnvironment *>(2, NULL));
						  for (vector< vector<VuoCompilerEnvironment *> >::iterator i = sharedEnvironments[target].begin(); i != sharedEnvironments[target].end(); ++i)
							  for (vector<VuoCompilerEnvironment *>::iterator j = i->begin(); j != i->end(); ++j)
								  *j = new VuoCompilerEnvironment(this->target, i == sharedEnvironments[target].begin(), j != i->begin());

						  vector<string> builtInModuleSearchPaths = VuoCompilerEnvironment::getBuiltInModuleSearchPaths();
						  for (vector<string>::iterator i = builtInModuleSearchPaths.begin(); i != builtInModuleSearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addModuleSearchPath(*i, false);

						  vector<string> builtInLibrarySearchPaths = VuoCompilerEnvironment::getBuiltInLibrarySearchPaths();
						  for (vector<string>::iterator i = builtInLibrarySearchPaths.begin(); i != builtInLibrarySearchPaths.end(); ++i)
							  sharedEnvironments[target][0][0]->addLibrarySearchPath(*i);

						  vector<string> builtInFrameworkSearchPaths = VuoCompilerEnvironment::getBuiltInFrameworkSearchPaths();
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
							  shared_ptr<VuoModuleCache> builtInCache = VuoModuleCache::newBuiltInCache();
							  shared_ptr<VuoModuleCache> systemCache = VuoModuleCache::newSystemCache();
							  shared_ptr<VuoModuleCache> userCache = VuoModuleCache::newUserCache();

							  sharedEnvironments[target][0][0]->setModuleCache(builtInCache);
							  sharedEnvironments[target][0][1]->setModuleCache(builtInCache);
							  sharedEnvironments[target][1][0]->setModuleCache(systemCache);
							  sharedEnvironments[target][1][1]->setModuleCache(systemCache);
							  sharedEnvironments[target][2][0]->setModuleCache(userCache);
							  sharedEnvironments[target][2][1]->setModuleCache(userCache);
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

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		for (VuoCompilerEnvironment *env : environmentsAtScope)
			env->removeCompilerToNotify(this);

	dispatch_sync(delegateQueue, ^{});

	dispatch_release(modulesToLoadQueue);
	dispatch_release(delegateQueue);
	dispatch_release(modulesLoading);

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
		for (vector< vector<VuoCompilerEnvironment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
		{
			(*i)[0]->stopWatchingModuleSearchPaths();
			dispatch_sync((*i)[0]->moduleSearchPathContentsChangedQueue, ^{});
		}

	for (auto e : environmentsForCompositionFamily)
		for (map< string, vector<VuoCompilerEnvironment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
		{
			(i->second)[0]->stopWatchingModuleSearchPaths();
			dispatch_sync((i->second)[0]->moduleSearchPathContentsChangedQueue, ^{});
		}

	for (auto e : sharedEnvironments)
		for (vector< vector<VuoCompilerEnvironment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
			for (vector<VuoCompilerEnvironment *>::iterator j = i->begin(); j != i->end(); ++j)
				delete *j;

	for (auto e : environmentsForCompositionFamily)
		for (map< string, vector<VuoCompilerEnvironment *> >::iterator i = e.second.begin(); i != e.second.end(); ++i)
			for (vector<VuoCompilerEnvironment *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
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
 * VuoCompiler::VuoCompiler()) so the compiler can locate composition-local modules.
 *
 * If you compile a composition and haven't told the compiler its path, the composition will be able
 * to use modules at the shared scopes (built-in, system, user) but not at the composition-local scope.
 *
 * @version200New
 */
void VuoCompiler::setCompositionPath(const string &compositionPathOrig)
{
	string compositionModulesDir;
	string compositionBaseDir;
	bool isSubcomposition = false;
	string compositionPath = compositionPathOrig;
	if (! compositionPath.empty())
	{
		compositionPath = VuoFileUtilities::getAbsolutePath(compositionPath);
		VuoFileUtilities::canonicalizePath(compositionPath);

		compositionModulesDir = VuoFileUtilities::getCompositionLocalModulesPath(compositionPath);

		string file, ext;
		VuoFileUtilities::splitPath(compositionModulesDir, compositionBaseDir, file, ext);
		VuoFileUtilities::canonicalizePath(compositionBaseDir);

		string compositionDir;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, ext);
		VuoFileUtilities::canonicalizePath(compositionDir);
		isSubcomposition = (compositionDir == compositionModulesDir);
	}

	// Set up `environments` to contain all environments available to this compiler, in order from broadest to narrowest.

	dispatch_sync(environmentQueue, ^{
					  if (! environments.empty() && compositionBaseDir == lastCompositionBaseDir) {
						  return;
					  }
					  lastCompositionBaseDir = compositionBaseDir;

					  // Clear out the existing environments for this compiler.

					  VuoCompilerEnvironment *oldCompositionFamilyInstalledEnvironment = nullptr;
					  vector<VuoCompilerEnvironment *> compositionEnvironments;
					  if (! environments.empty())
					  {
						  for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
							  for (VuoCompilerEnvironment *env : environmentsAtScope)
								  env->removeCompilerToNotify(this);

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
					  for (vector< vector<VuoCompilerEnvironment *> >::iterator i = sharedEnvironments[target].begin(); i != sharedEnvironments[target].end(); ++i)
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

					  VuoCompilerEnvironment *newCompositionFamilyInstalledEnvironment = nullptr;
					  if (! isCompositionInSharedEnvironment && ! compositionPath.empty())
					  {
						  vector<VuoCompilerEnvironment *> compositionFamilyEnvironments = environmentsForCompositionFamily[target][compositionBaseDir];
						  if (compositionFamilyEnvironments.empty())
						  {
							  compositionFamilyEnvironments = vector<VuoCompilerEnvironment *>(2, NULL);
							  compositionFamilyEnvironments[0] = new VuoCompilerEnvironment(this->target, false, false);
							  compositionFamilyEnvironments[1] = new VuoCompilerEnvironment(this->target, false, true);
							  environmentsForCompositionFamily[target][compositionBaseDir] = compositionFamilyEnvironments;

							  // Allow the user to place modules/subcompositions in a Modules folder inside the composition folder.

							  compositionFamilyEnvironments[0]->addModuleSearchPath(compositionModulesDir);

							  shared_ptr<VuoModuleCache> moduleCache = VuoModuleCache::newCache(compositionBaseDir);
							  compositionFamilyEnvironments[0]->setModuleCache(moduleCache);
							  compositionFamilyEnvironments[1]->setModuleCache(moduleCache);
						  }
						  environments.push_back(compositionFamilyEnvironments);

						  newCompositionFamilyInstalledEnvironment = compositionFamilyEnvironments[0];
					  }

					  // Add the composition environment to the compiler (or add it back in if it already existed).

					  if (compositionEnvironments.empty())
					  {
						  compositionEnvironments = vector<VuoCompilerEnvironment *>(2, NULL);
						  compositionEnvironments[0] = new VuoCompilerEnvironment(this->target, false, false);
						  compositionEnvironments[1] = new VuoCompilerEnvironment(this->target, false, true);

						  shared_ptr<VuoModuleCache> moduleCache = VuoModuleCache::newCache(compositionPath);
						  compositionEnvironments[0]->setModuleCache(moduleCache);
						  compositionEnvironments[1]->setModuleCache(moduleCache);
					  }
					  environments.push_back(compositionEnvironments);

					  // Select the generated environment:
					  //   - if compiling a subcomposition in a shared environment, the same shared environment
					  //   - if compiling a subcomposition in a custom modules directory, the composition-family environment
					  //   - otherwise, the composition environment

					  int generatedModulesScope = isCompositionInSharedEnvironment || isSubcomposition ? environments.size() - 2 : environments.size() - 1;
					  generatedEnvironment = environments[generatedModulesScope][1];

					  for (auto i : environments)
						  for (VuoCompilerEnvironment *env : i)
							  env->addCompilerToNotify(this);

					  delete dependencyGraph;
					  delete compositionDependencyGraph;
					  dependencyGraph = makeDependencyNetwork(environments, ^VuoDirectedAcyclicGraph * (VuoCompilerEnvironment *env) { return env->getDependencyGraph(); });
					  compositionDependencyGraph = makeDependencyNetwork(environments, ^VuoDirectedAcyclicGraph * (VuoCompilerEnvironment *env) { return env->getCompositionDependencyGraph(); });

					  // If the compiler has a different local Modules directory than before, notify the compiler's delegate
					  // of composition-family modules that are newly available/unavailable.

					  if (oldCompositionFamilyInstalledEnvironment != newCompositionFamilyInstalledEnvironment)
					  {
						  auto getModules = [] (VuoCompilerEnvironment *env)
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

							  VuoCompilerEnvironment *scopeEnvironment = newCompositionFamilyInstalledEnvironment;
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
VuoDirectedAcyclicNetwork * VuoCompiler::makeDependencyNetwork(const vector< vector<VuoCompilerEnvironment *> > &environments,
															   VuoDirectedAcyclicGraph * (^graphForEnvironment)(VuoCompilerEnvironment *))
{
	if (!graphForEnvironment)
		return NULL;

	VuoDirectedAcyclicNetwork *network = new VuoDirectedAcyclicNetwork();

	for (vector< vector<VuoCompilerEnvironment *> >::const_iterator i = environments.begin(); i != environments.end(); ++i)
	{
		// Installed environment depends on generated environment in same scope.

		network->addEdge(graphForEnvironment(i->at(0)), graphForEnvironment(i->at(1)));

		// Installed and generated environments depend on installed environments in broader scopes.

		for (vector< vector<VuoCompilerEnvironment *> >::const_iterator ii = environments.begin(); ii != i; ++ii)
		{
			network->addEdge(graphForEnvironment(i->at(0)), graphForEnvironment(ii->at(0)));
			network->addEdge(graphForEnvironment(i->at(1)), graphForEnvironment(ii->at(0)));
		}
	}

	return network;
}

/**
 * Returns the module with the given module key, or null if it can't be found or loaded.
 *
 * The module is loaded if it haven't been already.
 */
VuoCompilerModule * VuoCompiler::loadModuleIfNeeded(const string &moduleKey)
{
	// For performance, first see if the module is already loaded.

	VuoCompilerModule *module = nullptr;
	bool foundModuleInfoInNarrowerEnvironment = false;

	auto envGetModule = [&moduleKey, &module, &foundModuleInfoInNarrowerEnvironment] (VuoCompilerEnvironment *env)
	{
		VuoCompilerModule *foundModule = env->findModule(moduleKey);
		if (foundModule)
		{
			module = foundModule;
			foundModuleInfoInNarrowerEnvironment = false;
		}
		else if (module && ! foundModuleInfoInNarrowerEnvironment && (env->listModule(moduleKey) || env->listSourceFile(moduleKey)))
		{
			foundModuleInfoInNarrowerEnvironment = true;
		}
	};

	applyToAllEnvironments(envGetModule);

	// If not, try loading it and check again.

	if (! module || foundModuleInfoInNarrowerEnvironment)
	{
		loadModulesIfNeeded({moduleKey});
		applyToAllEnvironments(envGetModule);
	}

	return module;
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
	dispatch_group_enter(modulesLoading);
	VuoDefer(^{ dispatch_group_leave(modulesLoading); });

	__block bool willLoadAllModules = false;
	dispatch_sync(modulesToLoadQueue, ^{
		if (shouldLoadAllModules && ! hasLoadedAllModules) {
			willLoadAllModules = true;
			hasLoadedAllModules = true;
		}
	});

	if (! willLoadAllModules && moduleKeys.empty())
		return;

	// Load modules, and start sources and specialized modules compiling.

	__block set<dispatch_group_t> sourcesLoading;
	dispatch_sync(environmentQueue, ^{
					  sourcesLoading = loadModulesAndSources(moduleKeys, set<string>(), set<string>(),
															 moduleKeys, set<string>(), set<string>(),
															 willLoadAllModules, false, nullptr, nullptr, nullptr, "");
				  });

	// Wait for sources and specialized modules to finish compiling and the compiled modules to be loaded
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
														 VuoCompilerEnvironment *currentEnvironment, VuoCompilerIssues *issuesForCurrentEnvironment,
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

	map<VuoCompilerEnvironment *, set<string> > modulesAdded;
	map<VuoCompilerEnvironment *, set<string> > modulesModified;
	map<VuoCompilerEnvironment *, set<string> > modulesRemoved;
	map<VuoCompilerEnvironment *, set<string> > sourcesAdded;
	map<VuoCompilerEnvironment *, set<string> > sourcesModified;
	map<VuoCompilerEnvironment *, set<string> > sourcesRemoved;

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
		for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		{
			for (VuoCompilerEnvironment *env : environmentsAtScope)
			{
				bool willLoadAllModulesInEnv = willLoadAllModules && ! env->isGenerated();

				VuoModuleInfoIterator modulesAddedIter = (willLoadAllModulesInEnv ? env->listAllModules() : env->listModules(modulesAddedKeys));
				VuoModuleInfoIterator sourcesAddedIter = (willLoadAllModulesInEnv ? env->listAllSourceFiles() : env->listSourceFiles(sourcesAddedKeys));
				VuoModuleInfoIterator sourcesModifiedIter = (willLoadAllModulesInEnv ? env->listAllSourceFiles() : env->listSourceFiles(sourcesModifiedKeys));

				VuoModuleInfo *moduleInfo;
				while ((moduleInfo = modulesAddedIter.next()))
				{
					string moduleKey = moduleInfo->getModuleKey();

					modulesAdded[env].insert(moduleKey);
				}

				// If a source file and a compiled file for the same module are in the same folder,
				// the compiled file takes precedence.
				auto isCompiledModuleAtSameSearchPath = [&env] (VuoModuleInfo *sourceInfo)
				{
					VuoModuleInfo *compiledModuleInfo = env->listModule(sourceInfo->getModuleKey());
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
	}

	map<VuoCompilerEnvironment *, VuoCompilerIssues *> issues;
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		for (VuoCompilerEnvironment *env : environmentsAtScope)
			issues[env] = (env == currentEnvironment && issuesForCurrentEnvironment ? issuesForCurrentEnvironment : new VuoCompilerIssues());

	// Check for circular dependencies in added/modified sources.

	for (vector< vector<VuoCompilerEnvironment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		VuoCompilerEnvironment *env = (*i).at(0);

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
				VuoDependencyGraphVertex *vertex = static_cast<VuoDependencyGraphVertex *>(*k);
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

				VuoModuleInfo *sourceInfo = env->listSourceFile(moduleKey);
				if (sourceInfo)
					issue.setFilePath(sourceInfo->getFile()->path());

				issues[env]->append(issue);
			}
		}
	}

	// Update the longest downstream paths for added/modified sources.

	for (const vector<VuoCompilerEnvironment *> &envs : environments)
	{
		VuoCompilerEnvironment *env = envs.at(0);

		set<string> sourcesAddedModified;
		sourcesAddedModified.insert(sourcesAdded[env].begin(), sourcesAdded[env].end());
		sourcesAddedModified.insert(sourcesModified[env].begin(), sourcesModified[env].end());

		for (const string &moduleKey : sourcesAddedModified)
		{
			VuoDirectedAcyclicGraph::Vertex *vertex = env->getCompositionDependencyGraph()->findVertex(moduleKey);
			int pathLength = env->getCompositionDependencyGraph()->getLongestDownstreamPath(vertex);

			VuoModuleInfo *sourceInfo = env->listSourceFile(moduleKey);
			sourceInfo->setLongestDownstreamPath(pathLength);
		}
	}

	// Find all modules and sources that depend on the modules and sources being modified or removed.
	// Those that belong to one of this compiler's environments are used in subsequent stages.
	// Those that belong to some other environment are scheduled to be handled by a separate call to this function.

	map<VuoCompilerEnvironment *, set<string> > modulesDepOnModulesModified;
	map<VuoCompilerEnvironment *, set<string> > modulesDepOnModulesRemoved;
	map<VuoCompilerEnvironment *, set<string> > sourcesDepOnModulesRemoved;

	map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesModified;

	{
		map<VuoCompilerEnvironment *, set<string> > sourcesDepOnModulesModified;

		__block map<VuoCompilerEnvironment *, set<string> > modulesDepOnModulesModified_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > sourcesDepOnModulesModified_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > modulesDepOnModulesRemoved_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > sourcesDepOnModulesRemoved_otherCompiler;

		vector<VuoDirectedAcyclicNetwork *> searchDependencyGraphs;
		for (VuoCompiler *compiler : allCompilers)
			searchDependencyGraphs.push_back(compiler->dependencyGraph);

		VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph = (currentEnvironment ? currentEnvironment->getDependencyGraph() : nullptr);

		findDependentModulesAndSources(modulesModified, searchDependencyGraphs, currentEnvironmentDependencyGraph, true,
									   modulesDepOnModulesModified, modulesDepOnModulesModified_otherCompiler,
									   sourcesDepOnModulesModified, sourcesDepOnModulesModified_otherCompiler);

		findDependentModulesAndSources(modulesRemoved, searchDependencyGraphs, currentEnvironmentDependencyGraph, true,
									   modulesDepOnModulesRemoved, modulesDepOnModulesRemoved_otherCompiler,
									   sourcesDepOnModulesRemoved, sourcesDepOnModulesRemoved_otherCompiler);

		map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesModified;
		map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesRemoved;
		map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesRemoved;

		__block map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesModified_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesModified_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesRemoved_otherCompiler;
		__block map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesRemoved_otherCompiler;

		findDependentModulesAndSources(modulesModified, searchDependencyGraphs, currentEnvironmentDependencyGraph, false,
									   modulesDirectDepOnModulesModified, modulesDirectDepOnModulesModified_otherCompiler,
									   sourcesDirectDepOnModulesModified, sourcesDirectDepOnModulesModified_otherCompiler);

		findDependentModulesAndSources(modulesRemoved, searchDependencyGraphs, currentEnvironmentDependencyGraph, false,
									   modulesDirectDepOnModulesRemoved, modulesDirectDepOnModulesRemoved_otherCompiler,
									   sourcesDirectDepOnModulesRemoved, sourcesDirectDepOnModulesRemoved_otherCompiler);

		auto insertInvalidatedModulesAwaitingRecompilation = [](const map<VuoCompilerEnvironment *, set<string> > &modulesToInsert)
		{
			for (auto i : modulesToInsert)
			{
				VuoCompilerEnvironment *env = i.first;
				for (const string &moduleKey : i.second)
				{
					VuoCompilerModule *module = env->findModule(moduleKey);
					invalidatedModulesAwaitingRecompilation[env][moduleKey] = { module, nullptr };
				}
			}
		};

		insertInvalidatedModulesAwaitingRecompilation(sourcesDepOnModulesModified);
		insertInvalidatedModulesAwaitingRecompilation(sourcesDepOnModulesModified_otherCompiler);

		set<VuoCompilerEnvironment *> otherEnvironments;
		for (auto i : modulesDepOnModulesModified_otherCompiler)
			otherEnvironments.insert(i.first);
		for (auto i : sourcesDepOnModulesModified_otherCompiler)
			otherEnvironments.insert(i.first);
		for (auto i : modulesDepOnModulesRemoved_otherCompiler)
			otherEnvironments.insert(i.first);
		for (auto i : sourcesDepOnModulesRemoved_otherCompiler)
			otherEnvironments.insert(i.first);

		for (VuoCompilerEnvironment *env : otherEnvironments)
		{
			VuoCompiler *otherCompiler = nullptr;
			for (VuoCompiler *c : allCompilers)
				for (const vector<VuoCompilerEnvironment *> &ee : c->environments)
					for (VuoCompilerEnvironment *e : ee)
						if (e == env)
						{
							otherCompiler = c;
							goto foundOtherCompiler;
						}
			foundOtherCompiler:

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_sync(environmentQueue, ^{
					otherCompiler->loadModulesAndSources(set<string>(), modulesDepOnModulesModified_otherCompiler[env], modulesDepOnModulesRemoved_otherCompiler[env],
														 set<string>(), sourcesDirectDepOnModulesModified_otherCompiler[env], sourcesDirectDepOnModulesRemoved_otherCompiler[env],
														 false, true, env, nullptr, nullptr, "");
				});
			});
		}
	}

	// Unload:
	//   - modules that have been removed or modified
	//   - modules that depend on them

	map<VuoCompilerEnvironment *, set<VuoCompilerModule *> > actualModulesRemoved;
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			set<string> modulesToUnload;
			modulesToUnload.insert(modulesRemoved[env].begin(), modulesRemoved[env].end());
			modulesToUnload.insert(modulesModified[env].begin(), modulesModified[env].end());
			modulesToUnload.insert(modulesDepOnModulesRemoved[env].begin(), modulesDepOnModulesRemoved[env].end());
			modulesToUnload.insert(modulesDepOnModulesModified[env].begin(), modulesDepOnModulesModified[env].end());

			actualModulesRemoved[env] = env->unloadCompiledModules(modulesToUnload);
		}
	}

	// Load:
	//   - modules that have been added or modified
	//   - modules that they depend on
	//   - modules that depend on them that were just unloaded and are not awaiting recompilation
	// Delete:
	//   - cached module files in `modulesToLoad` whose source files have been removed

	map<VuoCompilerEnvironment *, set<string> > modulesToLoad;
	map<VuoCompilerEnvironment *, map<string, string> > modulesToLoadSourceCode;
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			auto moduleNotInvalidated = [env] (const string &moduleKey)
			{
				return invalidatedModulesAwaitingRecompilation[env].find(moduleKey) == invalidatedModulesAwaitingRecompilation[env].end();
			};

			if (env == currentEnvironment && moduleLoadedCallback)
			{
				modulesToLoad[env].insert(modulesModified[env].begin(), modulesModified[env].end());
				modulesToLoad[env].insert(modulesAdded[env].begin(), modulesAdded[env].end());
			}
			else
			{
				std::copy_if(modulesAdded[env].begin(), modulesAdded[env].end(),
							 std::inserter(modulesToLoad[env], modulesToLoad[env].end()),
							 moduleNotInvalidated);
				std::copy_if(modulesModified[env].begin(), modulesModified[env].end(),
							 std::inserter(modulesToLoad[env], modulesToLoad[env].end()),
							 moduleNotInvalidated);
			}

			std::copy_if(modulesDepOnModulesModified[env].begin(), modulesDepOnModulesModified[env].end(),
						 std::inserter(modulesToLoad[env], modulesToLoad[env].end()),
						 moduleNotInvalidated);

			if (env == currentEnvironment && moduleLoadedCallback)
			{
				if (modulesAdded[env].size() == 1)
					modulesToLoadSourceCode[env][*modulesAdded[env].begin()] = moduleAddedOrModifiedSourceCode;
				else if (modulesModified[env].size() == 1)
					modulesToLoadSourceCode[env][*modulesModified[env].begin()] = moduleAddedOrModifiedSourceCode;
			}
		}
	}

	set<string> modulesAttempted;
	modulesAttempted.insert(modulesAddedKeys.begin(), modulesAddedKeys.end());
	modulesAttempted.insert(modulesModifiedKeys.begin(), modulesModifiedKeys.end());

	map<VuoCompilerEnvironment *, set<VuoCompilerModule *> > actualModulesAdded;
	map<string, VuoCompilerEnvironment *> broadestEnvironmentThatDependsOnModule;
	while (! modulesToLoad.empty())
	{
		map<VuoCompilerEnvironment *, set<string>> dependenciesToLoad;

		for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		{
			for (VuoCompilerEnvironment *env : environmentsAtScope)
			{
				set<VuoCompilerModule *> actualModulesLoaded = env->loadCompiledModules(modulesToLoad[env], modulesToLoadSourceCode[env], llvmQueue);

				actualModulesAdded[env].insert(actualModulesLoaded.begin(), actualModulesLoaded.end());
				modulesToLoad.erase(env);

				// List dependencies of the modules just loaded.
				for (set<VuoCompilerModule *>::iterator j = actualModulesLoaded.begin(); j != actualModulesLoaded.end(); ++j)
				{
					set<string> dependencies = (*j)->getDependencies();
					dependenciesToLoad[env].insert(dependencies.begin(), dependencies.end());

					for (const string &dependency : dependencies)
					{
						VuoCompilerEnvironment *currEnv = broadestEnvironmentThatDependsOnModule[dependency];
						broadestEnvironmentThatDependsOnModule[dependency] = currEnv ? environmentAtBroaderScope(currEnv, env) : env;
					}

					modulesAttempted.insert(dependencies.begin(), dependencies.end());
				}
			}
		}

		// Locate the module info for each dependency. Restrict the search to scopes that the dependent module can access.
		for (auto i : dependenciesToLoad)
		{
			VuoCompilerEnvironment *dependentEnv = i.first;

			for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
			{
				for (VuoCompilerEnvironment *env : environmentsAtScope)
				{
					VuoModuleInfoIterator dependenciesInEnv = env->listModules(i.second);
					VuoModuleInfo *moduleInfo;
					while ((moduleInfo = dependenciesInEnv.next()))
					{
						string moduleKey = moduleInfo->getModuleKey();
						modulesToLoad[env].insert(moduleKey);
					}
				}

				if (find(environmentsAtScope.begin(), environmentsAtScope.end(), dependentEnv) != environmentsAtScope.end())
					break;
			}
		}
	}

	// Notify those waiting on a source file to be compiled that its module has now been loaded.

	if (moduleLoadedCallback)
		moduleLoadedCallback();

	// Load asynchronously:
	//   - specializations of generic modules
	//
	// The set of module keys that have been attempted but not yet loaded may include some specializations of
	// generic modules (along with things that can't be loaded as modules, e.g. dynamic libraries and frameworks).
	// Organize these potential specialized modules by the environment that should load them, then let the
	// environment attempt to load them and sort out which are actually specialized modules.

	vector<string> actualModulesAddedKeys;
	for (auto i : actualModulesAdded)
		for (VuoCompilerModule *module : i.second)
			actualModulesAddedKeys.push_back(module->getPseudoBase()->getModuleKey());

	vector<string> modulesNotAdded;
	std::set_difference(modulesAttempted.begin(), modulesAttempted.end(),
						actualModulesAddedKeys.begin(), actualModulesAddedKeys.end(),
						std::back_inserter(modulesNotAdded));

	auto moduleNotAlreadyLoaded = [this] (const string &moduleKey)
	{
		for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
			for (VuoCompilerEnvironment *env : environmentsAtScope)
				if (env->findModule(moduleKey))
					return false;

		return true;
	};

	set<string> potentialSpecializedModules;
	std::copy_if(modulesNotAdded.begin(), modulesNotAdded.end(),
				 std::inserter(potentialSpecializedModules, potentialSpecializedModules.begin()),
				 moduleNotAlreadyLoaded);

	// If the potential specialized module was enqueued to be loaded because it's a dependency of one or more
	// other modules, we'll load the specialized module at a scope that all of those modules can access.
	// Otherwise, at the default scope for generated modules.
	map<VuoCompilerEnvironment *, set<string>> potentialSpecializedModulesByEnvironment;
	for (const string &moduleKey : potentialSpecializedModules)
	{
		VuoCompilerEnvironment *envForLoadingModule = generatedEnvironment;

		auto envIter = broadestEnvironmentThatDependsOnModule.find(moduleKey);
		if (envIter != broadestEnvironmentThatDependsOnModule.end())
			envForLoadingModule = generatedEnvironmentAtSameScopeAs(envIter->second);

		potentialSpecializedModulesByEnvironment[envForLoadingModule].insert(moduleKey);
	}

	set<dispatch_group_t> specializedModulesLoading;
	for (auto i : potentialSpecializedModulesByEnvironment)
	{
		VuoCompilerEnvironment *env = i.first;
		set<dispatch_group_t> s = env->generateSpecializedModules(i.second, this, moduleSourceCompilersExist, llvmQueue);
		specializedModulesLoading.insert(s.begin(), s.end());
	}

	// Delete cached compiled module files and call this function recursively for:
	//   - modules whose source files have been removed
	//   - modules whose source files depend on them

	for (vector< vector<VuoCompilerEnvironment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		VuoCompilerEnvironment *env = (*i).at(0);

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
			VUserLog("Deleting compiled module from %s cache: %s", env->getName().c_str(), VuoStringUtilities::join(sourcesToUnload, ", ").c_str());

		env->deleteFromCompiledModuleCache(sourcesToUnload);
	}

	// Compile asynchronously:
	//   - source files that have been added or modified and are not already awaiting recompilation
	//   - source files that directly depend on them
	//   - source files that directly depend on modules that have been modified

	map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesAdded;
	{
		map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesAdded;                // unused
		map<VuoCompilerEnvironment *, set<string> > modulesDirectDepOnModulesAdded_otherCompiler;  //
		__block map<VuoCompilerEnvironment *, set<string> > sourcesDirectDepOnModulesAdded_otherCompiler;

		map<VuoCompilerEnvironment *, set<string> > actualModuleKeysAdded;
		for (const vector<VuoCompilerEnvironment *> &envs : environments)
		{
			VuoCompilerEnvironment *env = envs.at(0);
			for (VuoCompilerModule *module : actualModulesAdded[env])
				actualModuleKeysAdded[env].insert( module->getPseudoBase()->getModuleKey() );
		}

		vector<VuoDirectedAcyclicNetwork *> searchDependencyGraphs;
		searchDependencyGraphs.push_back(compositionDependencyGraph);
		for (map<string, vector<VuoCompilerEnvironment *> >::iterator ii = environmentsForCompositionFamily[target].begin(); ii != environmentsForCompositionFamily[target].end(); ++ii)
		{
			vector< vector<VuoCompilerEnvironment *> > otherEnvs = sharedEnvironments[target];
			otherEnvs.push_back(ii->second);
			VuoDirectedAcyclicNetwork *other = makeDependencyNetwork(otherEnvs, ^VuoDirectedAcyclicGraph * (VuoCompilerEnvironment *env) { return env->getCompositionDependencyGraph(); });
			searchDependencyGraphs.push_back(other);
		}

		VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph = (currentEnvironment ? currentEnvironment->getCompositionDependencyGraph() : nullptr);

		findDependentModulesAndSources(actualModuleKeysAdded, searchDependencyGraphs, currentEnvironmentDependencyGraph, false,
									   modulesDirectDepOnModulesAdded, modulesDirectDepOnModulesAdded_otherCompiler,
									   sourcesDirectDepOnModulesAdded, sourcesDirectDepOnModulesAdded_otherCompiler);

		set<VuoCompilerEnvironment *> otherEnvironments;
		for (map<VuoCompilerEnvironment *, set<string> >::iterator i = sourcesDirectDepOnModulesAdded_otherCompiler.begin(); i != sourcesDirectDepOnModulesAdded_otherCompiler.end(); ++i)
			otherEnvironments.insert(i->first);

		for (VuoCompilerEnvironment *env : otherEnvironments)
		{
			dispatch_group_enter(moduleSourceCompilersExist);
			dispatch_group_enter(moduleSourceCompilersExistGlobally);

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				string moduleSearchPath = env->getModuleSearchPaths().front();
				VuoCompiler *otherCompiler = new VuoCompiler(moduleSearchPath + "/unused", target);

				dispatch_sync(environmentQueue, ^{
					otherCompiler->loadModulesAndSources(set<string>(), set<string>(), set<string>(),
														 sourcesDirectDepOnModulesAdded_otherCompiler[env], set<string>(), set<string>(),
														 false, true, env, nullptr, nullptr, "");
				});

				delete otherCompiler;

				dispatch_group_leave(moduleSourceCompilersExist);
				dispatch_group_leave(moduleSourceCompilersExistGlobally);
			});
		}
	}

	set<dispatch_group_t> sourcesLoading;
	for (const vector<VuoCompilerEnvironment *> &envs : environments)
	{
		VuoCompilerEnvironment *env = envs.at(0);

		auto moduleNotInvalidated = [env] (const string &moduleKey)
		{
			return invalidatedModulesAwaitingRecompilation[env].find(moduleKey) == invalidatedModulesAwaitingRecompilation[env].end();
		};

		set<string> sourcesToCompile;
		set<string> otherSourcesForCallerToWaitOn;
		if (env == currentEnvironment)
		{
			sourcesToCompile.insert(sourcesAdded[env].begin(), sourcesAdded[env].end());
			sourcesToCompile.insert(sourcesModified[env].begin(), sourcesModified[env].end());
		}
		else
		{
			set<string> addedSourcesToCompile;
			set<string> modifiedSourcesToCompile;

			std::copy_if(sourcesAdded[env].begin(), sourcesAdded[env].end(),
						 std::inserter(addedSourcesToCompile, addedSourcesToCompile.end()),
						 moduleNotInvalidated);
			std::copy_if(sourcesModified[env].begin(), sourcesModified[env].end(),
						 std::inserter(modifiedSourcesToCompile, modifiedSourcesToCompile.end()),
						 moduleNotInvalidated);

			sourcesToCompile.insert(addedSourcesToCompile.begin(), addedSourcesToCompile.end());
			sourcesToCompile.insert(modifiedSourcesToCompile.begin(), modifiedSourcesToCompile.end());

			std::set_difference(sourcesAdded[env].begin(), sourcesAdded[env].end(),
								addedSourcesToCompile.begin(), addedSourcesToCompile.end(),
								std::inserter(otherSourcesForCallerToWaitOn, otherSourcesForCallerToWaitOn.end()));
			std::set_difference(sourcesModified[env].begin(), sourcesModified[env].end(),
								modifiedSourcesToCompile.begin(), modifiedSourcesToCompile.end(),
								std::inserter(otherSourcesForCallerToWaitOn, otherSourcesForCallerToWaitOn.end()));
		}

		if (! sourcesToCompile.empty())
		{
			set<dispatch_group_t> s = env->compileModulesFromSourceCode(sourcesToCompile, shouldRecompileSourcesIfUnchanged, moduleSourceCompilersExist, llvmQueue);
			sourcesLoading.insert(s.begin(), s.end());
		}

		for (const string &moduleKey : otherSourcesForCallerToWaitOn)
		{
			dispatch_group_t preloadingGroup = invalidatedModulesAwaitingRecompilation[env][moduleKey].second;
			if (! preloadingGroup)
			{
				preloadingGroup = dispatch_group_create();
				dispatch_group_enter(preloadingGroup);
				invalidatedModulesAwaitingRecompilation[env][moduleKey].second = preloadingGroup;
			}
			sourcesLoading.insert(preloadingGroup);
		}
	}

	for (vector< vector<VuoCompilerEnvironment *> >::iterator i = environments.begin(); i != environments.end(); ++i)
	{
		VuoCompilerEnvironment *env = (*i).at(0);

		set<string> sourcesToCompile;
		sourcesToCompile.insert(sourcesDirectDepOnModulesAdded[env].begin(), sourcesDirectDepOnModulesAdded[env].end());
		sourcesToCompile.insert(sourcesDirectDepOnModulesModified[env].begin(), sourcesDirectDepOnModulesModified[env].end());

		if (! sourcesToCompile.empty())
			env->compileModulesFromSourceCode(sourcesToCompile, true, moduleSourceCompilersExist, llvmQueue);
	}

	// Move modified modules from `actualModulesAdded` and `actualModulesRemoved`/`invalidatedModulesAwaitingRecompilation`
	// to `actualModulesModified`.

	map<VuoCompilerEnvironment *, set< pair<VuoCompilerModule *, VuoCompilerModule *> > > actualModulesModified;
	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (set<VuoCompilerModule *>::iterator add = actualModulesAdded[env].begin(); add != actualModulesAdded[env].end(); )
			{
				VuoCompilerModule *addedModule = *add;
				string moduleKey = addedModule->getPseudoBase()->getModuleKey();

				VuoCompilerModule *removedModule = nullptr;
				auto rem = std::find_if(actualModulesRemoved[env].begin(), actualModulesRemoved[env].end(),
										[moduleKey](VuoCompilerModule *m) { return m->getPseudoBase()->getModuleKey() == moduleKey; });
				if (rem != actualModulesRemoved[env].end())
				{
					removedModule = *rem;
					actualModulesRemoved[env].erase(rem);
				}
				else
				{
					auto invalidatedIter = invalidatedModulesAwaitingRecompilation[env].find(moduleKey);
					if (invalidatedIter != invalidatedModulesAwaitingRecompilation[env].end())
					{
						removedModule = invalidatedIter->second.first;
						dispatch_group_t preloadingGroup = invalidatedIter->second.second;

						invalidatedModulesAwaitingRecompilation[env].erase(invalidatedIter);

						if (preloadingGroup)
							dispatch_group_leave(preloadingGroup);
					}
				}

				if (removedModule)
				{
					actualModulesModified[env].insert({removedModule, addedModule});
					actualModulesAdded[env].erase(add++);
				}
				else
					++add;
			}
		}
	}

	// Move previously invalidated modules whose recompilation has now failed
	// from `invalidatedModulesAwaitingRecompilation` to `actualModulesRemoved`.

	if (currentEnvironment && moduleLoadedCallback)
	{
		for (const string &moduleKey : modulesRemoved[currentEnvironment])
		{
			auto foundIter = invalidatedModulesAwaitingRecompilation[currentEnvironment].find(moduleKey);
			if (foundIter != invalidatedModulesAwaitingRecompilation[currentEnvironment].end())
			{
				VuoCompilerModule *module = foundIter->second.first;
				dispatch_group_t preloadingGroup = foundIter->second.second;

				actualModulesRemoved[currentEnvironment].insert(module);
				invalidatedModulesAwaitingRecompilation[currentEnvironment].erase(foundIter);

				if (preloadingGroup)
					dispatch_group_leave(preloadingGroup);
			}
		}
	}

	// Postpone notifying compiler delegates about invalidated modules that are awaiting recompilation.
	// Once they're recompiled, in a later call to this function, they'll be reported as modified.

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (auto i : invalidatedModulesAwaitingRecompilation[env])
			{
				VuoCompilerModule *module = i.second.first;

				auto foundIter = actualModulesRemoved[env].find(module);
				if (foundIter != actualModulesRemoved[env].end())
					actualModulesRemoved[env].erase(foundIter);
			}
		}
	}

	// Postpone notifying compiler delegates that a node class has been added/modified until its port types and
	// generic node class have been loaded — which may not happen until a later call to this function.

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			addedModulesAwaitingReification[env].insert(actualModulesAdded[env].begin(), actualModulesAdded[env].end());
			modifiedModulesAwaitingReification[env].insert(actualModulesModified[env].begin(), actualModulesModified[env].end());
			actualModulesAdded[env].clear();
			actualModulesModified[env].clear();
		}
	}

	map<VuoCompilerEnvironment *, set<VuoCompilerModule *>> postponedModulesAdded;
	map<VuoCompilerEnvironment *, set<pair<VuoCompilerModule *, VuoCompilerModule *>>> postponedModulesModified;

	// For each pending added/modified node class, reify its port types (if they've been loaded).

	auto lookUpType = [this] (const string &moduleKey) -> VuoCompilerType *
	{
		for (auto i = environments.rbegin(); i != environments.rend(); ++i)
		{
			for (VuoCompilerEnvironment *env : *i)
			{
				VuoCompilerType *type = env->getType(moduleKey);
				if (type)
					return type;
			}
		}

		return nullptr;
	};

	auto reifyPortTypesForModule = [this, lookUpType] (VuoCompilerModule *module)
	{
		bool allReified = true;

		VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
		if (nodeClass)
			allReified = reifyPortTypes(nodeClass, lookUpType);

		return allReified;
	};

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (VuoCompilerModule *module : addedModulesAwaitingReification[env])
				if (! reifyPortTypesForModule(module))
					postponedModulesAdded[env].insert(module);

			for (pair<VuoCompilerModule *, VuoCompilerModule *> i : modifiedModulesAwaitingReification[env])
				if (! reifyPortTypesForModule(i.second))
					postponedModulesModified[env].insert(i);
		}
	}

	// For each pending added/modified specialized node class, fill in its references to node classes and types
	// (if they've been loaded).

	auto lookUpGenericNodeClass = [this] (const string &nodeClassName) -> VuoCompilerNodeClass *
	{
		for (auto i = environments.rbegin(); i != environments.rend(); ++i)
		{
			VuoCompilerEnvironment *env = (*i).at(0);
			VuoCompilerNodeClass *nodeClass = env->getNodeClass(nodeClassName);
			if (nodeClass)
				return nodeClass;
		}

		return nullptr;
	};

	auto fillInModuleReferences = [lookUpGenericNodeClass, lookUpType] (VuoCompilerModule *module)
	{
		bool allFilledIn = true;

		VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(module);
		if (specializedNodeClass)
		{
			allFilledIn = allFilledIn && specializedNodeClass->updateGenericNodeClass(lookUpGenericNodeClass);

			VuoCompilerMakeListNodeClass *makeListNodeClass = dynamic_cast<VuoCompilerMakeListNodeClass *>(module);
			if (makeListNodeClass)
				allFilledIn = allFilledIn && makeListNodeClass->updateListType(lookUpType);
		}

		return allFilledIn;
	};

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		VuoCompilerEnvironment *env = environmentsAtScope.at(1);

		for (VuoCompilerModule *module : addedModulesAwaitingReification[env])
			if (! fillInModuleReferences(module))
				postponedModulesAdded[env].insert(module);

		for (pair<VuoCompilerModule *, VuoCompilerModule *> i : modifiedModulesAwaitingReification[env])
			if (! fillInModuleReferences(i.second))
				postponedModulesModified[env].insert(i);
	}

	// Separate modules that are ready to be sent to compiler delegates from those that are being postponed.

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			std::set_difference(addedModulesAwaitingReification[env].begin(), addedModulesAwaitingReification[env].end(),
								postponedModulesAdded[env].begin(), postponedModulesAdded[env].end(),
								std::inserter(actualModulesAdded[env], actualModulesAdded[env].end()));
			std::set_difference(modifiedModulesAwaitingReification[env].begin(), modifiedModulesAwaitingReification[env].end(),
								postponedModulesModified[env].begin(), postponedModulesModified[env].end(),
								std::inserter(actualModulesModified[env], actualModulesModified[env].end()));
			addedModulesAwaitingReification[env] = postponedModulesAdded[env];
			modifiedModulesAwaitingReification[env] = postponedModulesModified[env];
		}
	}

	// Notify compiler delegates of modules added, modified, and removed.

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
		for (VuoCompilerEnvironment *env : environmentsAtScope)
			env->notifyCompilers(actualModulesAdded[env], actualModulesModified[env], actualModulesRemoved[env], issues[env]);

	// Log modules added and removed.

	auto logModuleUnloaded = [](VuoCompilerEnvironment *env, VuoCompilerModule *module)
	{
		if (! env->isBuiltInOriginal())
			VUserLog("Removed from %s environment: %s", env->getName().c_str(), module->getPseudoBase()->getModuleKey().c_str());
	};

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (pair<VuoCompilerModule *, VuoCompilerModule *> i : actualModulesModified[env])
				logModuleUnloaded(env, i.second);

			for (VuoCompilerModule *module : actualModulesRemoved[env])
				logModuleUnloaded(env, module);
		}
	}

	auto logModuleLoaded = [](VuoCompilerEnvironment *env, VuoCompilerModule *module)
	{
		if (! env->isBuiltIn())
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

			if (hash.empty() && ! path.empty())
			{
				try
				{
					hash = VuoFileUtilities::calculateFileSHA256(path);
				}
				catch (VuoException &e) {}
			}

			if (hash.empty() && module->getModule())
			{
				string bitcode;
				raw_string_ostream out(bitcode);
				llvm::WriteBitcodeToFile(module->getModule(), out);
				hash = VuoStringUtilities::calculateSHA256(bitcode);
			}

			if (path.empty())
				path = module->getPseudoBase()->getModuleKey();

			if (env->isGenerated())
				VDebugLog("Loaded into %s environment:  %s (%s)", env->getName().c_str(), module->getPseudoBase()->getModuleKey().c_str(), path.c_str());
			else
				VUserLog("Loaded into %s environment:  [%8.8s] %s (%s)", env->getName().c_str(), hash.c_str(), module->getPseudoBase()->getModuleKey().c_str(), path.c_str());
		}
	};

	for (const vector<VuoCompilerEnvironment *> environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (VuoCompilerModule *module : actualModulesAdded[env])
				logModuleLoaded(env, module);

			for (pair<VuoCompilerModule *, VuoCompilerModule *> i : actualModulesModified[env])
				logModuleLoaded(env, i.second);
		}
	}

	// Restore the dependency graph to its acyclic status by removing any edges where a module in a generated environment
	// depends on a module in the installed environment at the same level of scope.

	if (currentEnvironment == generatedEnvironment)
	{
		auto removeCyclesForDependencies = [] (VuoDirectedAcyclicGraph *installedDependencyGraph, VuoDirectedAcyclicGraph *generatedDependencyGraph,
											   VuoCompilerModule *module)
		{
			for (const string &dependency : module->getDependencies())
			{
				VuoDirectedAcyclicGraph::Vertex *generatedVertex = generatedDependencyGraph->findVertex(dependency);
				for (VuoDirectedAcyclicGraph::Vertex *toVertex : generatedDependencyGraph->getImmediatelyDownstreamVertices(generatedVertex))
				{
					VuoDirectedAcyclicGraph::Vertex *installedVertex = installedDependencyGraph->findVertex(toVertex->key());
					if (installedVertex)
						generatedDependencyGraph->removeEdge(generatedVertex, toVertex);
				}
			}
		};

		VuoDirectedAcyclicGraph *installedDependencyGraph = nullptr;
		for (const vector<VuoCompilerEnvironment *> &environmentsAtScope : environments)
		{
			if (environmentsAtScope.at(1) == currentEnvironment)
			{
				installedDependencyGraph = environmentsAtScope.at(0)->getDependencyGraph();
				break;
			}
		}

		VuoDirectedAcyclicGraph *generatedDependencyGraph = currentEnvironment->getDependencyGraph();

		for (VuoCompilerModule *m : actualModulesAdded[currentEnvironment])
			removeCyclesForDependencies(installedDependencyGraph, generatedDependencyGraph, m);
		for (pair<VuoCompilerModule *, VuoCompilerModule *> i : actualModulesModified[currentEnvironment])
			removeCyclesForDependencies(installedDependencyGraph, generatedDependencyGraph, i.first);
	}

	// Since the dispatch groups for specialized modules are temporary (caller is responsible for releasing them)
	// but the dispatch groups for module sources should stay alive as long as the VuoModuleInfo that contains them,
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
 * @param includeIndirectDependents True if all dependents should be returned, false if only direct dependents.
 * @param[out] modulesDepOnChangedModules_this Dependent modules that belong to this VuoCompiler.
 * @param[out] modulesDepOnChangedModules_other Dependent modules that belong to another VuoCompiler.
 * @param[out] sourcesDepOnChangedModules_this Dependent sources that belong to this VuoCompiler.
 * @param[out] sourcesDepOnChangedModules_other Dependent sources that belong to another VuoCompiler.
 */
void VuoCompiler::findDependentModulesAndSources(map<VuoCompilerEnvironment *, set<string> > &changedModules,
												 const vector<VuoDirectedAcyclicNetwork *> &searchDependencyGraphs,
												 VuoDirectedAcyclicGraph *currentEnvironmentDependencyGraph, bool includeIndirectDependents,
												 map<VuoCompilerEnvironment *, set<string> > &modulesDepOnChangedModules_this,
												 map<VuoCompilerEnvironment *, set<string> > &modulesDepOnChangedModules_other,
												 map<VuoCompilerEnvironment *, set<string> > &sourcesDepOnChangedModules_this,
												 map<VuoCompilerEnvironment *, set<string> > &sourcesDepOnChangedModules_other)
{
	for (const vector<VuoCompilerEnvironment *> &environmentsAtScope : environments)
	{
		for (VuoCompilerEnvironment *env : environmentsAtScope)
		{
			for (const string &module : changedModules[env])
			{
				// Find dependents in the dependency graph.

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
						VuoDependencyGraphVertex *moduleVertex = static_cast<VuoDependencyGraphVertex *>(moduleVertexRaw);
						if (moduleVertex->getEnvironment())
						{
							vector<VuoDirectedAcyclicGraph::Vertex *> upstream = includeIndirectDependents ?
																					 searchDependencyGraph->getUpstreamVertices(moduleVertex) :
																					 searchDependencyGraph->getImmediatelyUpstreamVertices(moduleVertex);
							dependents.insert(upstream.begin(), upstream.end());
						}
					}
				}

				set< pair<VuoCompilerEnvironment *, string> > dependentsMap;
				for (VuoDirectedAcyclicGraph::Vertex *dependentVertexRaw : dependents)
				{
					VuoDependencyGraphVertex *v = static_cast<VuoDependencyGraphVertex *>(dependentVertexRaw);
					VuoCompilerEnvironment *dependentEnv = v->getEnvironment();
					if (! dependentEnv)
						continue;

					string dependent = v->getDependency();

					dependentsMap.insert({dependentEnv, dependent});
				}

				// Add in dependents that were omitted from the dependency graph because they would make it cyclic —
				// a generated module depends on an installed module at the same scope.

				if (! env->isGenerated())
				{
					VuoCompilerEnvironment *generatedEnvironmentAtScope = environmentsAtScope.at(1);

					auto addDependentIfNeeded = [&module, &dependentsMap] (VuoCompilerEnvironment *env, VuoCompilerModule *potentialDependentModule)
					{
						set<string> dependencies = potentialDependentModule->getDependencies();
						if (dependencies.find(module) != dependencies.end())
							dependentsMap.insert({env, potentialDependentModule->getPseudoBase()->getModuleKey()});
					};

					for (auto i : generatedEnvironmentAtScope->getNodeClasses())
						addDependentIfNeeded(generatedEnvironmentAtScope, i.second);
					for (auto i : generatedEnvironmentAtScope->getTypes())
						addDependentIfNeeded(generatedEnvironmentAtScope, i.second);
					for (auto i : generatedEnvironmentAtScope->getLibraryModules())
						addDependentIfNeeded(generatedEnvironmentAtScope, i.second);
				}

				// Distribute the dependent modules and sources to the appropriate lists (output parameters).

				for (auto i : dependentsMap)
				{
					VuoCompilerEnvironment *dependentEnv = i.first;
					string dependent = i.second;

					// Skip if the dependent module is already being modified/removed in its own right
					// (e.g. if the module depends on another in the same node set and the node set is being removed).
					if (changedModules[dependentEnv].find(dependent) != changedModules[dependentEnv].end())
						continue;

					VuoModuleInfo *foundSourceInfo = dependentEnv->listSourceFile(dependent);
					VuoModuleInfo *foundModuleInfo = dependentEnv->listModule(dependent);

					bool belongsToCurrentCompiler = false;
					for (const vector<VuoCompilerEnvironment *> &envs2 : environments)
					{
						if (find(envs2.begin(), envs2.end(), dependentEnv) != envs2.end())
						{
							belongsToCurrentCompiler = true;
							break;
						}
					}

					auto distributeToList = [dependentEnv, dependent] (map<VuoCompilerEnvironment *, set<string>> *dependentsList, VuoModuleInfo *moduleInfo)
					{
						(*dependentsList)[dependentEnv].insert(dependent);
						if (moduleInfo)
							moduleInfo->setAttempted(false);
					};

					if (foundSourceInfo)
						distributeToList(belongsToCurrentCompiler ? &sourcesDepOnChangedModules_this : &sourcesDepOnChangedModules_other, foundSourceInfo);

					if (foundModuleInfo)
						distributeToList(belongsToCurrentCompiler ? &modulesDepOnChangedModules_this : &modulesDepOnChangedModules_other, foundModuleInfo);

					if (! foundSourceInfo && ! foundModuleInfo)  // module in generated environment
						distributeToList(belongsToCurrentCompiler ? &modulesDepOnChangedModules_this : &modulesDepOnChangedModules_other, nullptr);
				}
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
								VuoCompilerIssues *issues, void *delegateDataV, VuoCompilerEnvironment *currentEnvironment)
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
		vector< pair<VuoCompilerEnvironment *, VuoCompilerModule *> > moduleVersions;
		for (const vector<VuoCompilerEnvironment *> &envs : environments)
		{
			VuoCompilerEnvironment *env = envs.at(0);
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

		vector< pair<VuoCompilerEnvironment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

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

		vector< pair<VuoCompilerEnvironment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

		if (moduleVersions.size() > 1 && moduleVersions.back().second != moduleModified)
			modulesModified.erase(i++);
		else
			++i;
	}

	for (map<string, VuoCompilerModule *>::iterator i = modulesRemoved.begin(); i != modulesRemoved.end(); )
	{
		string moduleKey = i->first;
		VuoCompilerModule *moduleRemoved = i->second;

		vector< pair<VuoCompilerEnvironment *, VuoCompilerModule *> > moduleVersions = findVersionsOfModule(moduleKey);

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

	// Synchronously log any issues, to make sure they get recorded before any resulting crashes.

	if (! issues->isEmpty())
		VUserLog("%s", issues->getLongDescription(false).c_str());

	// Asynchronously notify the delegate (if any).

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
 * Updates the data-carrying ports of @a nodeClass to match them up with known data types.
 *
 * When a node class is first loaded, its ports are assigned placeholder `VuoType`s without `VuoCompilerType`s.
 * This function fills as many of the `VuoCompilerType`s as have been loaded at this point. If not all have
 * been loaded, then this function will need to be called again.
 */
bool VuoCompiler::reifyPortTypes(VuoCompilerNodeClass *nodeClass, std::function<VuoCompilerType *(const string &)> lookUpType)
{
	bool missingTypes = false;

	auto setPortTypesForNodeClass = [lookUpType, &missingTypes] (const vector<VuoPortClass *> portClasses)
	{
		for (VuoPortClass *portClass : portClasses)
		{
			VuoCompilerPortClass *compilerPortClass = static_cast<VuoCompilerPortClass *>(portClass->getCompiler());
			VuoType *type = compilerPortClass->getDataVuoType();

			if (type && ! type->hasCompiler())
			{
				string typeName = type->getModuleKey();

				VuoCompilerType *reifiedType = lookUpType(typeName);
				if (! reifiedType)
				{
					VuoGenericType *genericType = dynamic_cast<VuoGenericType *>(type);
					if (genericType)
						reifiedType = VuoCompilerGenericType::newGenericType(genericType, lookUpType);
				}

				if (reifiedType)
					compilerPortClass->setDataVuoType(reifiedType->getBase());
			}

			if (compilerPortClass->getDataVuoType() && ! compilerPortClass->getDataVuoType()->hasCompiler())
				missingTypes = true;
		}
	};
	setPortTypesForNodeClass(nodeClass->getBase()->getInputPortClasses());
	setPortTypesForNodeClass(nodeClass->getBase()->getOutputPortClasses());

	for (VuoCompilerTriggerDescription *trigger : nodeClass->getTriggerDescriptions())
	{
		VuoType *type = trigger->getDataType();

		if (type && ! type->hasCompiler())
		{
			string typeName = type->getModuleKey();
			VuoCompilerType *reifiedType = lookUpType(typeName);
			if (reifiedType)
				trigger->setDataType(reifiedType->getBase());
		}

		if (trigger->getDataType() && ! trigger->getDataType()->hasCompiler())
			missingTypes = true;
	}

	return ! missingTypes;
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
 * Returns a VuoModuleCompiler suitable for compiling the given module,
 * or null if no such VuoModuleCompiler exists.
 */
VuoModuleCompiler * VuoCompiler::createModuleCompiler(const string &moduleKey, const string &inputPath,
													  const map<string, string> &typeNameReplacements)
{
	VuoModuleCompilerSettings settings;

	settings.target = target;
	settings.isVerbose = isVerbose;

	settings.vuoFrameworkPath = getVuoFrameworkPath();
	if (settings.vuoFrameworkPath.empty())
	{
		settings.vuoFrameworkPath = VUO_BUILD_DIR "/lib/Vuo.framework";

		// When loading compound types that are dependencies of the module being compiled, find their generic source files here.
		dispatch_sync(environmentQueue, ^{
			environments[0][0]->addModuleSearchPath(VUO_BUILD_DIR "/lib/Vuo.framework/Modules", false);
		});
	}

	if (VuoFileUtilities::getVuoFrameworkPath().empty())
		settings.macOSSDKPath = MACOS_SDK_ROOT;

	vector<string> headerSearchPaths;
	applyToInstalledEnvironments([&headerSearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envHeaderSearchPaths = env->getHeaderSearchPaths();
		headerSearchPaths.insert(headerSearchPaths.end(), envHeaderSearchPaths.begin(), envHeaderSearchPaths.end());
	});
	settings.headerSearchPaths = headerSearchPaths;

	settings.typeNameReplacements = typeNameReplacements;

	auto getVuoType = [this] (const string &moduleKey) { return this->getType(moduleKey); };

	VuoModuleCompiler *moduleCompiler = nullptr;
	for (string type : {"c", "isf"})
	{
		moduleCompiler = VuoModuleCompiler::newModuleCompiler(type, moduleKey, inputPath, settings, getVuoType);
		if (moduleCompiler)
			break;
	}

	return moduleCompiler;
}

/**
 * Compiles a node class, port type, or library module to LLVM bitcode.
 *
 * @param inputPath The file to compile, containing a text-code implementation of the node class, port type, or library module.
 * @param overriddenSourceCode An implementation overriding the contents of @a inputPath.
 * @param typeNameReplacements For each generic type name, the specialized type name that should be substituted in its place.
 * @throw VuoCompilerException The module source code is invalid.
 */
VuoModuleCompilerResults VuoCompiler::compileModuleInMemory(const string &inputPath, const string &overriddenSourceCode,
															const map<string, string> &typeNameReplacements)
{
	if (isVerbose)
		print();

	string moduleKey = getModuleKeyForPath(inputPath);

	VuoModuleCompiler *moduleCompiler = createModuleCompiler(moduleKey, inputPath, typeNameReplacements);
	if (! moduleCompiler)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", inputPath, "", "Unrecognized source file type.");
		throw VuoCompilerException(issue);
	}

	if (! overriddenSourceCode.empty())
		moduleCompiler->overrideSourceCode(overriddenSourceCode, inputPath);

	VuoCompilerIssues *issues = new VuoCompilerIssues;
	VuoModuleCompilerResults results = moduleCompiler->compile(llvmQueue, issues);

	if (! results.module)
		throw VuoCompilerException(issues, true);

	if (! issues->isEmpty())
		VUserLog("%s", issues->getLongDescription(false).c_str());

	delete issues;
	delete moduleCompiler;

	return results;
}

/**
 * Compiles a node class, port type, or library module to an LLVM bitcode file.
 *
 * @param inputPath The file to compile, containing a text-code implementation of the node class, port type, or library module.
 * @param outputPath The file in which to save the compiled LLVM bitcode.
 * @throw VuoCompilerException The module source code is invalid.
 */
void VuoCompiler::compileModule(const string &inputPath, const string &outputPath)
{
	VuoModuleCompilerResults results = compileModuleInMemory(inputPath);
	Module *module = results.module;

	string moduleKey = getModuleKeyForPath(inputPath);

	__block VuoCompilerModule *vuoModule = nullptr;
	__block VuoCompilerIssues *issues = new VuoCompilerIssues();
	dispatch_sync(llvmQueue, ^{
		vuoModule = VuoCompilerModule::newModule(moduleKey, module, "", VuoCompilerCompatibility::compatibilityWithAnySystem());

		if (vuoModule)
		{
			try
			{
				verifyModule(module, issues);
				writeModuleToBitcode(module, target, outputPath, issues);

				if (! dependencyOutputPath.empty())
				{
					shared_ptr<VuoMakeDependencies> makeDependencies = results.makeDependencies;
					if (makeDependencies)
						makeDependencies->setCompiledFilePath(outputPath);
					else
						makeDependencies = VuoMakeDependencies::createFromComponents(outputPath, {inputPath});

					makeDependencies->writeToFile(dependencyOutputPath);
				}
			}
			catch (VuoCompilerException &e) {}
			catch (VuoException &e)
			{
				VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", inputPath, "", e.what());
				issues->append(issue);
			}
		}

		delete module;
	});

	if (issues->hasErrors())
		throw VuoCompilerException(issues, true);

	delete issues;

	if (! vuoModule)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(inputPath, dir, file, ext);
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", inputPath, "", "Not a node class, type, or library.");
		throw VuoCompilerException(issue);
	}
}

/**
 * Generates a header file for a specialization of a generic module.
 *
 * @param inputPath The path of the generic module's implementation file with the specialized type(s) appended
 *    to the file name, e.g. `vuo/type/compound/VuoList_VuoInteger.cc`.
 * @param outputPath The path at which to save the header file.
 */
void VuoCompiler::generateHeaderForModule(const string &inputPath, const string &outputPath)
{
	string moduleKey = getModuleKeyForPath(inputPath);

	VuoModuleCompiler *moduleCompiler = createModuleCompiler(moduleKey, inputPath);
	if (! moduleCompiler)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "generating header", inputPath, "", "Unrecognized source file type.");
		throw VuoCompilerException(issue);
	}

	VuoCompilerIssues *issues = new VuoCompilerIssues();
	string headerContents = moduleCompiler->generateHeader(issues);

	if (issues->hasErrors())
		throw VuoCompilerException(issues, true);
	else if (! issues->isEmpty())
		VUserLog("%s", issues->getLongDescription(false).c_str());

	delete issues;
	delete moduleCompiler;

	VuoFileUtilities::writeStringToFile(headerContents, outputPath);
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
	bool ownsIssues = false;
	if (! issues)
	{
		issues = new VuoCompilerIssues;
		ownsIssues = true;
	}

	string moduleKey = getModuleKeyForPath(outputPath);
	Module *module = compileCompositionToModule(composition, moduleKey, isTopLevelComposition, issues);
	if (!module)
		throw VuoCompilerException(issues, ownsIssues);

	dispatch_sync(llvmQueue, ^{
		try
		{
			verifyModule(module, issues);
			writeModuleToBitcode(module, target, outputPath, issues);
		}
		catch (VuoCompilerException &e) {}
	});

	if (issues->hasErrors())
		throw VuoCompilerException(issues, ownsIssues);

	if (ownsIssues)
		delete issues;
}

/**
 * Compiles a composition, read from file, to LLVM bitcode.
 *
 * @param inputPath The .vuo file containing the composition. If you haven't already specified the
 *     composition path in the constructor or VuoCompiler::setCompositionPath(), then @a inputPath will be used
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
 * (VuoRunner::newCurrentProcessRunnerFromDynamicLibrary), @a optimization should be VuoCompiler::Optimization_ModuleCaches
 * or VuoCompiler::Optimization_ExistingModuleCaches. This prevents conflicts between Objective-C classes (from nodes)
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

	if (optimization == Optimization_ExistingModuleCaches)
		shouldLoadAllModules = false;

	vector<shared_ptr<VuoModuleCacheRevision>> currentModuleCaches = useCurrentModuleCaches(optimization);

	set<string> dependencies = getDependenciesForComposition(compiledCompositionPath);

	VuoLinkerInputs linkerInputs;
	linkerInputs.addDependencies(dependencies, currentModuleCaches, this);
	linkerInputs.addExternalLibrary(compiledCompositionPath);
	linkerInputs.addVuoRuntime(this);
	if (! isDylib)
		linkerInputs.addVuoRuntimeMain(this);

	link(linkedCompositionPath, linkerInputs, isDylib, rPaths, shouldAdHocCodeSign);

	for (shared_ptr<VuoModuleCacheRevision> revision : currentModuleCaches)
		revision->disuse();
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
 *     This object should not be shared between different VuoRunner instances (or else the VuoModuleCacheRevision reference
 *     counts won't be incremented/decremented appropriately).
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

	// Check if any of the module cache revisions used previously are no longer available.
	// If so, we'll need to replace the existing unloadable resource dylibs (since they may depend on those module cache revisions)
	// with a new resource dylib that provides all of the dependencies from the previous dylibs plus any new dependencies.

	vector<shared_ptr<VuoModuleCacheRevision>> currentModuleCaches = useCurrentModuleCaches(Optimization_ModuleCaches);

	vector<string> carriedOverModuleCaches = runningCompositionLibraries->getUnloadableCacheLibrariesLoaded();
	vector<string> defunctModuleCaches;
	set<string> dependenciesInDefunctModuleCaches;
	set<string> dependenciesInDefunctResources;

	// Identify module cache revisions that `runningCompositionLibraries` has already been told are outdated.
	map<string, set<string>> defunctModuleCachesAndDependencies = runningCompositionLibraries->getCacheLibrariesEnqueuedToUnload();
	for (auto i : defunctModuleCachesAndDependencies)
	{
		defunctModuleCaches.push_back(i.first);
		dependenciesInDefunctModuleCaches.insert(i.second.begin(), i.second.end());
	}

	// Identify any other module cache revisions that are no longer available because the module cache has been rebuilt.
	for (string carriedOverCachePath : carriedOverModuleCaches)
	{
		auto sameAsCarriedOverCachePath = [&carriedOverCachePath] (shared_ptr<VuoModuleCacheRevision> revision)
		{
			return revision->getDylibPath() == carriedOverCachePath;
		};

		auto foundIter = std::find_if(currentModuleCaches.begin(), currentModuleCaches.end(), sameAsCarriedOverCachePath);
		if (foundIter == currentModuleCaches.end())
		{
			defunctModuleCaches.push_back(carriedOverCachePath);

			set<string> dependenciesInCache = runningCompositionLibraries->enqueueCacheLibraryToUnload(carriedOverCachePath);
			dependenciesInDefunctModuleCaches.insert(dependenciesInCache.begin(), dependenciesInCache.end());
		}
	}

	if (! defunctModuleCaches.empty())
	{
		dependenciesInDefunctResources = runningCompositionLibraries->enqueueAllUnloadableResourceLibrariesToUnload();

		std::sort(carriedOverModuleCaches.begin(), carriedOverModuleCaches.end());
		std::sort(defunctModuleCaches.begin(), defunctModuleCaches.end());

		carriedOverUnloadableLibraries.clear();
		std::set_difference(carriedOverModuleCaches.begin(), carriedOverModuleCaches.end(),
							defunctModuleCaches.begin(), defunctModuleCaches.end(),
							std::back_inserter(carriedOverUnloadableLibraries));
	}

	// Link the new resource dylibs, if needed.

	VuoLinkerInputs linkerInputsForAddedDependencies;
	VuoLinkerInputs linkerInputsForNonUnloadableResource;
	VuoLinkerInputs linkerInputsForUnloadableResource;

	string nonUnloadableResourcePath;
	string unloadableResourcePath;

	if (! addedDependencies.empty() || ! dependenciesInDefunctModuleCaches.empty() || ! dependenciesInDefunctResources.empty())
	{
		// Look up the linker inputs for dependencies used by the new resources that are not provided by the carried-over resources.

		linkerInputsForAddedDependencies.addDependencies(addedDependencies, currentModuleCaches, this);
		linkerInputsForAddedDependencies.addDependencies(dependenciesInDefunctModuleCaches, currentModuleCaches, this);
		linkerInputsForAddedDependencies.addDependencies(dependenciesInDefunctResources, currentModuleCaches, this);

		string dir, linkedCompositionFile, ext;
		VuoFileUtilities::splitPath(linkedCompositionPath, dir, linkedCompositionFile, ext);

		// If built-in dependencies were added, create an additional resource dylib.

		bool wereBuiltInModulesAdded = ! linkerInputsForAddedDependencies.getModulesInBuiltInEnvironments().empty();
		bool wereBuiltInLibrariesAdded = ! linkerInputsForAddedDependencies.getLibrariesInBuiltInEnvironments().empty();
		if (wereBuiltInModulesAdded || wereBuiltInLibrariesAdded)
		{
			nonUnloadableResourcePath = VuoFileUtilities::makeTmpFile(linkedCompositionFile + "-resource-nonunloadable", "dylib");

			linkerInputsForNonUnloadableResource.addModulesInBuiltInEnvironments( linkerInputsForAddedDependencies.getModulesInBuiltInEnvironments() );
			linkerInputsForNonUnloadableResource.addLibrariesInBuiltInEnvironments( linkerInputsForAddedDependencies.getLibrariesInBuiltInEnvironments() );
			linkerInputsForNonUnloadableResource.addExternalLibraries( linkerInputsForAddedDependencies.getExternalLibraries() );
			linkerInputsForNonUnloadableResource.addFrameworks( linkerInputsForAddedDependencies.getFrameworks() );

			linkerInputsForNonUnloadableResource.addExternalLibraries( carriedOverNonUnloadableLibraries );
			linkerInputsForNonUnloadableResource.addExternalLibraries( carriedOverExternalLibraries );
			linkerInputsForNonUnloadableResource.addFrameworks( carriedOverFrameworks );

			vector<string> rPaths = getRunPathSearchPaths(environments.front().front());

			link(nonUnloadableResourcePath, linkerInputsForNonUnloadableResource, true, rPaths, shouldAdHocCodeSign);
		}

		// If non-built-in dependencies were added (or any module caches went away), create an additional (or replacement) resource dylib.

		bool wereNonBuiltInModulesAdded = ! linkerInputsForAddedDependencies.getModulesInNonBuiltInEnvironments().empty();
		bool wereNonBuiltInLibrariesAdded = ! linkerInputsForAddedDependencies.getLibrariesInNonBuiltInEnvironments().empty();
		if (wereNonBuiltInModulesAdded || wereNonBuiltInLibrariesAdded)
		{
			unloadableResourcePath = VuoFileUtilities::makeTmpFile(linkedCompositionFile + "-resource-unloadable", "dylib");

			linkerInputsForUnloadableResource.addModulesInNonBuiltInEnvironments( linkerInputsForAddedDependencies.getModulesInNonBuiltInEnvironments() );
			linkerInputsForUnloadableResource.addLibrariesInNonBuiltInEnvironments( linkerInputsForAddedDependencies.getLibrariesInNonBuiltInEnvironments() );
			linkerInputsForUnloadableResource.addExternalLibraries( linkerInputsForAddedDependencies.getExternalLibraries() );
			linkerInputsForUnloadableResource.addFrameworks( linkerInputsForAddedDependencies.getFrameworks() );

			linkerInputsForUnloadableResource.addExternalLibraries( carriedOverNonUnloadableLibraries );
			linkerInputsForUnloadableResource.addExternalLibraries( carriedOverUnloadableLibraries );
			linkerInputsForUnloadableResource.addExternalLibraries( carriedOverExternalLibraries );
			linkerInputsForUnloadableResource.addFrameworks( carriedOverFrameworks );

			if (! nonUnloadableResourcePath.empty())
			{
				linkerInputsForUnloadableResource.addExternalLibrary( nonUnloadableResourcePath );
				linkerInputsForUnloadableResource.addLibrariesInBuiltInEnvironments( linkerInputsForNonUnloadableResource.getDylibsInBuiltInEnvironments() );
				linkerInputsForUnloadableResource.addExternalLibraries( linkerInputsForNonUnloadableResource.getExternalDylibs() );
			}

			// This is usually correct, but may fail in the case where there are two identically-named dylibs at different
			// levels of scope. However, it's the best we can do as long as modules at the system, user, and composition
			// levels of scope are all combined into one resource dylib. (https://b33p.net/kosada/vuo/vuo/-/merge_requests/196#note_2148884)
			vector<string> rPaths = getRunPathSearchPaths(environments.back().front());

			link(unloadableResourcePath, linkerInputsForUnloadableResource, true, rPaths, shouldAdHocCodeSign);
		}
	}

	// Link the composition.

	{
		VuoLinkerInputs linkerInputsForComposition;

		linkerInputsForComposition.addExternalLibrary(compiledCompositionPath);
		linkerInputsForComposition.addVuoRuntime(this);

		linkerInputsForComposition.addExternalLibraries( linkerInputsForAddedDependencies.getDylibsInBuiltInEnvironments() );
		linkerInputsForComposition.addExternalLibraries( linkerInputsForAddedDependencies.getDylibsInNonBuiltInEnvironments() );
		linkerInputsForComposition.addExternalLibraries( linkerInputsForAddedDependencies.getExternalDylibs() );
		linkerInputsForComposition.addFrameworks( linkerInputsForAddedDependencies.getFrameworks() );

		linkerInputsForComposition.addExternalLibraries( carriedOverNonUnloadableLibraries );
		linkerInputsForComposition.addExternalLibraries( carriedOverUnloadableLibraries );
		linkerInputsForComposition.addExternalLibraries( carriedOverExternalLibraries );
		linkerInputsForComposition.addFrameworks( carriedOverFrameworks );

		if (! nonUnloadableResourcePath.empty())
		{
			linkerInputsForComposition.addExternalLibrary( nonUnloadableResourcePath );
			linkerInputsForComposition.addLibrariesInBuiltInEnvironments( linkerInputsForNonUnloadableResource.getDylibsInBuiltInEnvironments() );
			linkerInputsForComposition.addExternalLibraries( linkerInputsForNonUnloadableResource.getExternalDylibs() );
		}

		if (! unloadableResourcePath.empty())
		{
			linkerInputsForComposition.addExternalLibrary( unloadableResourcePath );
			linkerInputsForComposition.addLibrariesInBuiltInEnvironments( linkerInputsForUnloadableResource.getDylibsInBuiltInEnvironments() );
			linkerInputsForComposition.addLibrariesInNonBuiltInEnvironments( linkerInputsForUnloadableResource.getDylibsInNonBuiltInEnvironments() );
			linkerInputsForComposition.addExternalLibraries( linkerInputsForUnloadableResource.getExternalDylibs() );
		}

		vector<string> rPaths = getRunPathSearchPaths(environments.front().front());
		link(linkedCompositionPath, linkerInputsForComposition, true, rPaths, shouldAdHocCodeSign);
	}

	// Now that we're past the point where an exception can be thrown, update the RunningCompositionLibraries.

	if (! nonUnloadableResourcePath.empty())
	{
		set<string> nonUnloadableDependencies = linkerInputsForNonUnloadableResource.getNonCachedModuleKeysInBuiltInEnvironments();
		runningCompositionLibraries->enqueueResourceLibraryToLoad(nonUnloadableResourcePath, nonUnloadableDependencies, false);
	}

	if (! unloadableResourcePath.empty())
	{
		set<string> unloadableDependencies = linkerInputsForUnloadableResource.getNonCachedModuleKeysInNonBuiltInEnvironments();
		runningCompositionLibraries->enqueueResourceLibraryToLoad(unloadableResourcePath, unloadableDependencies, true);
	}

	for (auto i : linkerInputsForAddedDependencies.getCachedDependenciesInBuiltInEnvironments())
	{
		shared_ptr<VuoModuleCacheRevision> revision = i.first;
		revision->use();
		runningCompositionLibraries->enqueueCacheLibraryToLoad(revision->getDylibPath(), i.second, false, [revision](void){ revision->disuse(); });
	}

	for (auto i : linkerInputsForAddedDependencies.getCachedDependenciesInNonBuiltInEnvironments())
	{
		shared_ptr<VuoModuleCacheRevision> revision = i.first;
		revision->use();
		runningCompositionLibraries->enqueueCacheLibraryToLoad(revision->getDylibPath(), i.second, true, [revision](void){ revision->disuse(); });
	}

	runningCompositionLibraries->addExternalLibraries( linkerInputsForAddedDependencies.getExternalDylibs() );
	runningCompositionLibraries->addExternalFrameworks( linkerInputsForAddedDependencies.getFrameworks() );

	for (shared_ptr<VuoModuleCacheRevision> revision : currentModuleCaches)
		revision->disuse();
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
	vector<string> librarySearchPaths;
	applyToInstalledEnvironments([&librarySearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envLibrarySearchPaths = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), envLibrarySearchPaths.begin(), envLibrarySearchPaths.end());
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
	vector<string> dependenciesToVisit(directDependencies.begin(), directDependencies.end());

	while (! dependenciesToVisit.empty())
	{
		string moduleKey = dependenciesToVisit.back();
		dependenciesToVisit.pop_back();

		set<pair<string, VuoCompilerEnvironment *>> currentDependencies;

		// First pass: Find all dependencies of the direct dependency that are in the dependency graph and have been loaded so far.

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
			VuoDependencyGraphVertex *v = static_cast<VuoDependencyGraphVertex *>(*j);
			getNodeClass(v->getDependency());
		}

		// Second pass: Find all dependencies of the direct dependency that are in the dependency graph, this time including
		// dependencies of the node classes just generated.

		vector<VuoDirectedAcyclicGraph::Vertex *> moduleVertices = dependencyGraph->findVertex(moduleKey);
		for (VuoDirectedAcyclicGraph::Vertex *mv : moduleVertices)
		{
			VuoDependencyGraphVertex *moduleVertex = static_cast<VuoDependencyGraphVertex *>(mv);
			currentDependencies.insert({moduleKey, moduleVertex->getEnvironment()});

			vector<VuoDirectedAcyclicGraph::Vertex *> downstream = dependencyGraph->getDownstreamVertices(mv);
			for (VuoDirectedAcyclicGraph::Vertex *dv : downstream)
			{
				VuoDependencyGraphVertex *downstreamVertex = static_cast<VuoDependencyGraphVertex *>(dv);
				currentDependencies.insert({downstreamVertex->getDependency(), downstreamVertex->getEnvironment()});
			}
		}

		// Third pass: Add in dependencies that were omitted from the dependency graph because they would make it cyclic —
		// a generated module depends on an installed module at the same scope.

		set<string> extraDependencies;
		for (auto i : currentDependencies)
		{
			VuoCompilerEnvironment *env = i.second;
			if (env && env->isGenerated())
			{
				VuoCompilerModule *dependency = env->findModule(i.first);
				set<string> dependenciesOfDependency = dependency->getDependencies();
				extraDependencies.insert(dependenciesOfDependency.begin(), dependenciesOfDependency.end());
			}
		}

		vector<string> notYetAdded;
		std::set_difference(extraDependencies.begin(), extraDependencies.end(),
							dependencies.begin(), dependencies.end(),
							std::back_inserter(notYetAdded));

		vector<string> notYetAddedOrEnqueued;
		std::set_difference(notYetAdded.begin(), notYetAdded.end(),
							dependenciesToVisit.begin(), dependenciesToVisit.end(),
							std::back_inserter(notYetAddedOrEnqueued));

		dependenciesToVisit.insert(dependenciesToVisit.end(), notYetAddedOrEnqueued.begin(), notYetAddedOrEnqueued.end());

		for (auto i : currentDependencies)
			dependencies.insert(i.first);
	}

	// Check that the dependencies are compatible with the compiler's target.

	if (checkCompatibility)
	{
		string targetForCompatibility = ! requestedTarget.empty() ? requestedTarget : getProcessTarget();
		VuoCompilerCompatibility neededCompatibility = VuoCompilerCompatibility::compatibilityWithTargetTriple(targetForCompatibility);
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
										   [](VuoDirectedAcyclicGraph::Vertex *u){ return static_cast<VuoDependencyGraphVertex *>(u)->getDependency(); });

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
 * @param shouldUseExistingBuiltInCaches If true, the existing cache of built-in modules (if any) is used,
 *     without checking if it needs to be rebuilt.
 * @param shouldUseExistingOtherCaches If true, the existing caches at narrower scopes (if any) are used,
 *     without checking if they need to be rebuilt.
 * @param target If non-empty, caches are rebuilt and made available only for the given target.
 */
void VuoCompiler::makeModuleCachesAvailable(bool shouldUseExistingBuiltInCaches, bool shouldUseExistingOtherCaches, const string &target)
{
	loadModulesIfNeeded();
	dispatch_group_wait(modulesLoading, DISPATCH_TIME_FOREVER);  // Wait for any previous loadModulesIfNeeded() calls to complete.

	dispatch_sync(environmentQueue, ^{

		set<string> accumulatedDylibs;
		set<string> accumulatedFrameworks;
		vector<shared_ptr<VuoModuleCache>> accumulatedModuleCaches;
		double lastPrerequisiteModuleCacheRebuild = 0;

		for (const vector<VuoCompilerEnvironment *> &environmentsAtScope : environments)
		{
			shared_ptr<VuoModuleCache> moduleCache = environmentsAtScope.at(0)->getModuleCache();
			if (! moduleCache)
				continue;

			VuoModuleCacheManifest expectedManifest;
			vector<VuoModuleInfoIterator> expectedModules;

			for (VuoCompilerEnvironment *env : environmentsAtScope)
			{
				set<string> dylibsNeededToLinkToCache;
				set<string> frameworksNeededToLinkToCache;
				VuoModuleCacheManifest envManifest = env->getCacheableModulesAndDependencies(dylibsNeededToLinkToCache, frameworksNeededToLinkToCache);

				expectedManifest.addContentsOf(envManifest);
				accumulatedDylibs.insert(dylibsNeededToLinkToCache.begin(), dylibsNeededToLinkToCache.end());
				accumulatedFrameworks.insert(frameworksNeededToLinkToCache.begin(), frameworksNeededToLinkToCache.end());

				expectedModules.push_back(env->listAllModules());
			}

			vector<string> runPathSearchPaths = getRunPathSearchPaths(environmentsAtScope.at(0));

			bool shouldUseExistingCache = (environmentsAtScope.at(0)->isBuiltIn() ? shouldUseExistingBuiltInCaches : shouldUseExistingOtherCaches);

			moduleCache->makeAvailable(shouldUseExistingCache, accumulatedModuleCaches, lastPrerequisiteModuleCacheRebuild,
									   expectedManifest, expectedModules, accumulatedDylibs, accumulatedFrameworks, runPathSearchPaths,
									   this, getTargetArch(target));

			accumulatedModuleCaches.push_back(moduleCache);
		}
	});

	VuoModuleCache::waitForCachesToBuild();
}

/**
 * Returns the current revisions of module caches that are available to be used for linking a composition,
 * given the type of optimization requested (which may or may not allow module caches).
 */
vector<shared_ptr<VuoModuleCacheRevision>> VuoCompiler::useCurrentModuleCaches(Optimization optimization)
{
	vector<shared_ptr<VuoModuleCacheRevision>> revisions;

	if (optimization == VuoCompiler::Optimization_ModuleCaches || optimization == VuoCompiler::Optimization_ExistingModuleCaches)
	{
		makeModuleCachesAvailable(true, optimization == Optimization_ExistingModuleCaches);

		applyToInstalledEnvironments([&revisions](VuoCompilerEnvironment *env)
		{
			shared_ptr<VuoModuleCache> moduleCache = env->getModuleCache();
			if (moduleCache)
			{
				shared_ptr<VuoModuleCacheRevision> revision = moduleCache->useCurrentRevision();
				if (revision)
					revisions.push_back(revision);
			}
		});
	}

	return revisions;
}

/**
 * Asynchronously makes the module caches available, rebuilding any non-built-in module caches that are out of date,
 * to avoid a delay the first time a composition is linked with VuoCompiler::Optimization_ModuleCaches or
 * VuoCompiler::Optimization_ExistingModuleCaches.
 */
void VuoCompiler::prepareModuleCaches(void)
{
	dispatch_group_async(moduleCacheBuilding, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							 makeModuleCachesAvailable(true, false);
						 });
}

/**
 * Generates the cache of built-in modules and places it inside of Vuo.framework.
 *
 * This function is intended for running Vuo in a special mode just to generate the caches and then exit.
 * It's called while building Vuo itself, toward the end of the process of creating Vuo.framework.
 *
 * Assumes that no VuoCompiler instances have been constructed before this call.
 *
 * @param vuoFrameworkPath The absolute path of Vuo.framework, as would be returned by
 *     @ref VuoFileUtilities::getVuoFrameworkPath if the Vuo.framework dynamic library were loaded.
 * @param target The LLVM target-triple to build.
 * @param onlyGenerateModules If true, this function only generates the modules for the module cache's
 *    compiled modules directory; it doesn't create the module cache dylib.
 * @version200New
 */
void VuoCompiler::generateBuiltInModuleCache(string vuoFrameworkPath, string target, bool onlyGenerateModules)
{
	vuoFrameworkInProgressPath = vuoFrameworkPath;

	// Delete the non-built-in caches to ensure that all specialized modules that are dependencies of built-in modules
	// will be generated and saved to the built-in cache rather than being loaded from a non-built-in cache.
	string cachePath = VuoFileUtilities::getCachePath();
	if (! cachePath.empty())
		VuoFileUtilities::deleteDir(cachePath);

	VuoCompiler compiler("", target);
	compiler.generatedEnvironment = compiler.environments.at(0).at(1);

	if (onlyGenerateModules)
	{
		compiler.loadModulesIfNeeded();
		dispatch_group_wait(compiler.moduleSourceCompilersExist, DISPATCH_TIME_FOREVER);
	}
	else
	{
		compiler.makeModuleCachesAvailable(false, true, target);
	}
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
 * @param linkerInputs The items to link in. If building an executable, one of them should contain a main function.
 * @param isDylib If true, the output file will be a dynamic library. Otherwise, it will be an executable.
 * @param rPaths The `-rpath` (run-path search path) arguments to be passed to `ld`.
 * @param shouldAdHocCodeSign  Whether to ad-hoc code-sign the generated binary.  Disable to improve performance (e.g., for live-editing on systems that don't require code-signing).
 * @throw VuoCompilerException clang or ld failed to link the given dependencies.
 */
void VuoCompiler::link(string outputPath, const VuoLinkerInputs &linkerInputs,
					   bool isDylib, const vector<string> &rPaths, bool shouldAdHocCodeSign, VuoCompilerIssues *issues)
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

					  for (auto i : linkerInputs.getModules())
					  {
						  unique_ptr<Module> upi = llvm::CloneModule(i);
						  if (linker.linkInModule(std::move(upi)))
						  {
							  VuoCompilerIssue issue(VuoCompilerIssue::IssueType::Error, "linking composite module", "",
													 "", "Failed to link in the module with ID '" + i->getModuleIdentifier() + "'");
							  issues->append(issue);
						  }
					  }

					  try
					  {
						  verifyModule(compositeModule.get(), issues);
						  writeModuleToBitcode(compositeModule.get(), target, compositeModulePath, issues);
					  }
					  catch (VuoCompilerException &e) {}

					  double linkModulesTime = VuoLogGetTime() - t0;
					  if (linkModulesTime > 0.1)
						  VUserLog("\tLinkModules took %5.2fs", linkModulesTime);
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
	for (string library : linkerInputs.getLibraries())
	{
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

	vector<string> frameworkSearchPaths;
	applyToInstalledEnvironments([&frameworkSearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envFrameworkSearchPaths = env->getFrameworkSearchPaths();
		frameworkSearchPaths.insert(frameworkSearchPaths.end(), envFrameworkSearchPaths.begin(), envFrameworkSearchPaths.end());
	});

	for (vector<string>::const_iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
	{
		string a = "-F"+*i;
		// Keep these std::strings around until after args is done being used, since the std::string::c_str() is freed when the std::string is deleted.
		frameworkArguments.push_back(a);
		char *frameworkArgument = strdup(a.c_str());
		args.push_back(frameworkArgument);
		argsToFree.push_back(frameworkArgument);
	}

	for (const string &framework : linkerInputs.getFrameworks())
	{
		args.push_back("-framework");

		string frameworkName = framework.substr(0, framework.length() - string(".framework").length());
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
	shared_ptr<VuoClangIssues> clangIssues = std::make_shared<VuoClangIssues>();
	auto diagnosticConsumer = new VuoCompilerDiagnosticConsumer(clangIssues);
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

	if (! clangIssues->isEmpty())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "linking composition", outputPath, "", "Failed to link file");
		issue.setClangIssues(clangIssues);
		issues->append(issue);
	}

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

	if (! issues->isEmpty())
		VUserLog("%s", issues->getLongDescription(false).c_str());

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
 * Checks the module for errors.
 *
 * @throw VuoCompilerException Errors were found.
 *
 * @threadQueue{llvmQueue}
 */
void VuoCompiler::verifyModule(Module *module, VuoCompilerIssues *issues)
{
	string str;
	raw_string_ostream verifyOut(str);
	if (llvm::verifyModule(*module, &verifyOut))
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", "", "Verification failed for module '" + module->getModuleIdentifier() + "'",
							   verifyOut.str());
		issue.setModuleKey(module->getModuleIdentifier());
		issues->append(issue);
		throw VuoCompilerException(issues, false);
	}
}

/**
 * Writes the module to @a outputPath (an LLVM bitcode file).
 *
 * @throw VuoCompilerException Writing failed.
 *
 * @threadQueue{llvmQueue}
 */
void VuoCompiler::writeModuleToBitcode(Module *module, string target, string outputPath, VuoCompilerIssues *issues)
{
	// Ensure the module gets output in the bitcode wrapper format instead of raw bitcode.
	setTargetForModule(module, target);

	std::error_code err;
	raw_fd_ostream out(outputPath.c_str(), err, sys::fs::F_None);
	if (err)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", "", "Write failed for module '" + module->getModuleIdentifier() + "'",
							   "Couldn't write to '" + outputPath + "': " + err.message());
		issue.setModuleKey(module->getModuleIdentifier());
		issues->append(issue);
		throw VuoCompilerException(issues, false);
	}
	llvm::WriteBitcodeToFile(module, out);
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
 * Returns an LLVM target triple that matches the current process.
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
 * Causes the node class that was previously loaded from @a sourcePath by VuoCompiler::installNodeClassAtCompositionLocalScope(),
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
 * VuoCompiler::revertOverriddenNodeClass() is called.
 *
 * A call to this function may or may not result in a call to VuoCompilerDelegate::loadedModules().
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
		VuoModuleInfo *sourceInfo = NULL;

		for (const vector<VuoCompilerEnvironment *> &envs : environments)
		{
			for (VuoCompilerEnvironment *env : envs)
			{
				VuoModuleInfo *potentialSourceInfo = env->listSourceFile(nodeClassName);
				if (potentialSourceInfo && VuoFileUtilities::arePathsEqual(potentialSourceInfo->getFile()->path(), sourcePathCopy))
				{
					sourceInfo = potentialSourceInfo;
					break;
				}
			}
		}

		if (! sourceInfo)
			return;

		if (! sourceCodeCopy.empty())
		{
			sourceInfo->setSourceCode(sourceCodeCopy);
			sourceInfo->setSourceCodeOverridden(true);
			sourceInfo->setAttempted(false);
			sourceInfo->setLastModifiedToNow();

			loadModulesAndSources({}, {}, {}, {}, { nodeClassName }, {}, false, false, sourceInfo->getEnvironment(), nullptr, nullptr, "");
		}
		else
		{
			sourceInfo->revertSourceCode();
			sourceInfo->setSourceCodeOverridden(false);

			sourceInfo->getEnvironment()->deleteOverriddenModuleFile(nodeClassName);

			VuoModuleInfo *moduleInfo = sourceInfo->getEnvironment()->listModule(nodeClassName);
			moduleInfo->setAttempted(false);
			moduleInfo->setLastModifiedToNow();

			loadModulesAndSources({}, { nodeClassName }, {}, {}, {}, {}, false, false, sourceInfo->getEnvironment(), nullptr, nullptr, "");
		}
	});
}

/**
 * Removes an override added by VuoCompiler::overrideInstalledNodeClass() (if any), reverting to the
 * installed version of the node class.
 *
 * A call to this function always results in a call to VuoCompilerDelegate::loadedModules(),
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
 * If the node class has any ports with compound types, those types may still be loading in the
 * background after this function returns. If you need to wait for them to load, either call getType()
 * on each compound type, which will block until the type loads, or wait for a notification from
 * the VuoCompilerDelegate that the node class has been added.
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

	if (! VuoNodeClass::isNodeClassName(nodeClassName) ||
			nodeClassName == VuoNodeClass::publishedInputNodeClassName || nodeClassName == VuoNodeClass::publishedInputNodeClassName)
		return nullptr;

	VuoCompilerModule *module = loadModuleIfNeeded(nodeClassName);
	return dynamic_cast<VuoCompilerNodeClass *>(module);
}

/**
 * Returns all node classes found and loaded, indexed by node class name.
 *
 * The node class modules are loaded if they haven't been already.
 */
map<string, VuoCompilerNodeClass *> VuoCompiler::getNodeClasses()
{
	loadModulesIfNeeded();

	map<string, VuoCompilerNodeClass *> nodeClasses;
	applyToInstalledEnvironments([&nodeClasses](VuoCompilerEnvironment *env)
	{
		map<string, VuoCompilerNodeClass *> envNodeClasses = env->getNodeClasses();
		nodeClasses.insert(envNodeClasses.begin(), envNodeClasses.end());
	});

	return nodeClasses;
}

/**
 * Returns the type specified by @a typeName, or null if it can't be found or loaded.
 *
 * The type module is loaded or generated if it haven't been already.
 */
VuoCompilerType * VuoCompiler::getType(const string &typeName)
{
	if (! VuoGenericType::isGenericTypeName(typeName))
	{
		VuoCompilerModule *module = loadModuleIfNeeded(typeName);
		return dynamic_cast<VuoCompilerType *>(module);
	}
	else
	{
		VuoCompilerType * (^compilerGetType)(string) = ^VuoCompilerType * (string moduleKey) {
			return getType(moduleKey);
		};

		VuoGenericType *genericType = new VuoGenericType(typeName, vector<string>());
		return VuoCompilerGenericType::newGenericType(genericType, compilerGetType);
	}
}

/**
 * Returns all types found and loaded, indexed by type name.
 *
 * The type modules are loaded if they haven't been already.
 */
map<string, VuoCompilerType *> VuoCompiler::getTypes()
{
	loadModulesIfNeeded();

	map<string, VuoCompilerType *> types;
	applyToInstalledEnvironments([&types](VuoCompilerEnvironment *env)
	{
		map<string, VuoCompilerType *> envTypes = env->getTypes();
		types.insert(envTypes.begin(), envTypes.end());
	});

	return types;
}

/**
 * Returns the library specified by `libraryName`, or null if it can't be found or loaded.
 *
 * The library is loaded if it haven't been already.
 */
VuoCompilerModule * VuoCompiler::getLibraryModule(const string &libraryModuleName)
{
	return loadModuleIfNeeded(libraryModuleName);
}

/**
 * Returns all libraries found and loaded, indexed by key.
 *
 * The libraries are loaded if they haven't been already.
 */
map<string, VuoCompilerModule *> VuoCompiler::getLibraryModules()
{
	loadModulesIfNeeded();

	map<string, VuoCompilerModule *> libraryModules;
	applyToInstalledEnvironments([&libraryModules](VuoCompilerEnvironment *env)
	{
		map<string, VuoCompilerModule *> envLibraryModules = env->getLibraryModules();
		libraryModules.insert(envLibraryModules.begin(), envLibraryModules.end());
	});

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

	map<string, VuoNodeSet *> nodeSets;
	applyToInstalledEnvironments([&nodeSets](VuoCompilerEnvironment *env)
	{
		map<string, VuoNodeSet *> envNodeSets = env->getNodeSets();
		nodeSets.insert(envNodeSets.begin(), envNodeSets.end());
	});

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

	VuoNodeSet *nodeSet = nullptr;
	applyToInstalledEnvironments([&nodeSet, &name](VuoCompilerEnvironment *env)
	{
		VuoNodeSet *foundNodeSet = env->findNodeSet(name);
		if (foundNodeSet)
			nodeSet = foundNodeSet;
	});

	return nodeSet;
}

/**
 * Looks up the VuoCompilerNodeClass, VuoCompilerType, or VuoCompilerModule specified by @a moduleKey.
 */
VuoCompilerModule * VuoCompiler::getModule(const string &moduleKey)
{
	VuoCompilerModule *module = nullptr;
	applyToAllEnvironments([&module, &moduleKey](VuoCompilerEnvironment *env)
	{
		VuoCompilerModule *foundModule = env->findModule(moduleKey);
		if (foundModule)
			module = foundModule;
	});

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
vector<string> VuoCompiler::getRunPathSearchPaths(VuoCompilerEnvironment *narrowestScope)
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
 * Returns the file name of the main function that can accompany the Vuo runtime.
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
 * This returns the same thing as VuoFileUtilities::getVuoFrameworkPath() except during a call
 * to VuoCompiler::generateBuiltInModuleCaches().
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
 * Specifies the path at which to create a GCC-style Makefile-depfile (used by CMake's DEPFILE)
 * that lists the source/header files this module depends on.
 */
void VuoCompiler::setDependencyOutput(const string &path)
{
	dependencyOutputPath = path;
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
 * Prints info about this compiler, for debugging.
 */
void VuoCompiler::print(void)
{
	vector<string> moduleSearchPaths;
	applyToInstalledEnvironments([&moduleSearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envModuleSearchPaths = env->getModuleSearchPaths();
		moduleSearchPaths.insert(moduleSearchPaths.end(), envModuleSearchPaths.begin(), envModuleSearchPaths.end());
	});

	vector<string> headerSearchPaths;
	applyToInstalledEnvironments([&headerSearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envHeaderSearchPaths = env->getHeaderSearchPaths();
		headerSearchPaths.insert(headerSearchPaths.end(), envHeaderSearchPaths.begin(), envHeaderSearchPaths.end());
	});

	vector<string> librarySearchPaths;
	applyToInstalledEnvironments([&librarySearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envLibrarySearchPaths = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), envLibrarySearchPaths.begin(), envLibrarySearchPaths.end());
	});

	vector<string> frameworkSearchPaths;
	applyToInstalledEnvironments([&frameworkSearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envFrameworkSearchPaths = env->getFrameworkSearchPaths();
		frameworkSearchPaths.insert(frameworkSearchPaths.end(), envFrameworkSearchPaths.begin(), envFrameworkSearchPaths.end());
	});

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
		compiler.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, Optimization_ModuleCaches);
		remove(compiledCompositionPath.c_str());

		typedef VuoRunner * (*runnerFunctionType)(const char *, const char *, bool, bool);
		runnerFunctionType runnerFunction = (runnerFunctionType)dlsym(RTLD_DEFAULT, "VuoRunner_newSeparateProcessRunnerFromExecutable");
		return runnerFunction(linkedCompositionPath.c_str(), directory.c_str(), false, true);
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
		compiler.linkCompositionToCreateExecutable(compiledCompositionPath, linkedCompositionPath, Optimization_ModuleCaches);
		remove(compiledCompositionPath.c_str());

		typedef VuoRunner * (*runnerFunctionType)(const char *, const char *, bool, bool);
		runnerFunctionType runnerFunction = (runnerFunctionType)dlsym(RTLD_DEFAULT, "VuoRunner_newSeparateProcessRunnerFromExecutable");
		return runnerFunction(linkedCompositionPath.c_str(), workingDirectory.c_str(), false, true);
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
		compiler.linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, Optimization_ModuleCaches);
		remove(compiledCompositionPath.c_str());

		typedef VuoRunner * (*runnerFunctionType)(const char *, const char *, bool);
		runnerFunctionType runnerFunction = (runnerFunctionType)dlsym(RTLD_DEFAULT, "VuoRunner_newCurrentProcessRunnerFromDynamicLibrary");
		return runnerFunction(linkedCompositionPath.c_str(), directory.c_str(), true);
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
		compiler.linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, Optimization_ModuleCaches);
		remove(compiledCompositionPath.c_str());

		typedef VuoRunner * (*runnerFunctionType)(const char *, const char *, bool);
		runnerFunctionType runnerFunction = (runnerFunctionType)dlsym(RTLD_DEFAULT, "VuoRunner_newCurrentProcessRunnerFromDynamicLibrary");
		return runnerFunction(linkedCompositionPath.c_str(), workingDirectory.c_str(), true);
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());
		return NULL;
	}
}
