/**
 * @file
 * VuoModuleCache interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <mutex>
#include <thread>
#include "VuoFileUtilities.hh"
#include "VuoModuleCacheManifest.hh"
#include "VuoModuleInfoIterator.hh"
class VuoCompiler;
class VuoModuleCacheRevision;

/**
 * Provides an interface to the caches of compiled modules stored in the filesystem,
 * with the ability to build a cache, rebuild a cache if it's out of date, and search for a cached module.
 *
 * Each instance of this class is associated with one scope of modules (built-in, system, user, etc.)
 * and may contain modules from multiple environments at that scope.
 *
 * @see VuoCompilerEnvironment
 */
class VuoModuleCache
{
public:
	static shared_ptr<VuoModuleCache> newBuiltInCache(void);
	static shared_ptr<VuoModuleCache> newSystemCache(void);
	static shared_ptr<VuoModuleCache> newUserCache(void);
	static shared_ptr<VuoModuleCache> newCache(const string &uniquePath);
	string getCompiledModulesPath(bool isGenerated, const string &targetArch);
	string getOverriddenCompiledModulesPath(bool isGenerated, const string &targetArch);
	bool modifyCompiledModules(std::function<bool(void)> modify);
	void makeAvailable(bool shouldUseExistingCache, vector<shared_ptr<VuoModuleCache>> prerequisiteModuleCaches, double &lastPrerequisiteModuleCacheRebuild, const VuoModuleCacheManifest &expectedManifest, vector<VuoModuleInfoIterator> expectedModules, const set<string> &dylibsToLinkTo, const set<string> &frameworksToLinkTo, const vector<string> &runPathSearchPaths, VuoCompiler *compiler, const string &targetArch = "");
	static void waitForCachesToBuild(void);
	shared_ptr<VuoModuleCacheRevision> useCurrentRevision(void);
	void invalidate(void);
	static void deleteOldCaches(void);

private:
	VuoModuleCache(const string &cacheDirectoryPath, bool builtIn);
	static string getCacheDirectoryPath(const string &uniquePath);
	static string getRelativePathOfModulesDirectory(bool isGenerated, const string &targetArch);
	string getDescription(void);
	string getManifestPath(const string &targetArch = "");
	string getDylibPath(const string &targetArch = "");
	string findLatestRevisionOfDylib(double &lastModified);
	static bool areDifferentRevisionsOfSameDylib(const string &dylibPath1, const string &dylibPath2);

	string cacheDirectoryPath;  ///< The directory that contains the cache for this level of scope.
	bool builtIn;  ///< True if this is the cache of built-in modules.
	bool available;  ///< True if the module cache has been checked (and rebuilt if needed) and is now up-to-date.
	bool invalidated;  ///< True if the module cache has not yet been checked or may be out-of-date due to changes in the modules in its scope.
	shared_ptr<VuoModuleCacheRevision> currentRevision; ///< The most recent revision of this module cache.
	double lastRebuild;  ///< The time (in seconds since a reference date) when the module cache's dylib was last modified (if it's not the cache of built-in modules).
	std::mutex statusMutex;  ///< Synchronizes access to `available` and `invalidated` and `lastRebuild`.
	std::mutex contentsMutex;  ///< Synchronizes access to `actualContents` and `currentDylibPath`.
	static std::mutex buildMutex;  ///< Ensures that module caches are rebuilt one at a time in the order they were requested.
	static int buildsInProgress;  ///< The number of module caches that have been scheduled for rebuilding or are currently rebuilding.
	static std::condition_variable buildsInProgressCondition;  ///< Waits on and notifies about changes to #buildsInProgress.
	static std::mutex buildsInProgressMutex;  ///< Synchronizes access to #buildsInProgressCondition.

	static const string builtInCacheDirName;  ///< The name of the cache directory for the built-in scope.
	static const string systemCacheDirName;  ///< The name of the cache directory for the system scope.
	static const string userCacheDirName;  ///< The name of the cache directory for the user scope.
	static const string pidCacheDirPrefix;  ///< The prefix of the names of pid-specific cache directories.

	/**
	 * Info related to file locks for keeping different processes from interfering with each other when using
	 * the same module cache directory.
	 *
	 * One process at a time gets access to write to the module cache directory at a particular scope. It can
	 * modify the contents of the compiled modules directory and rebuild the cache dylib. Other processes can
	 * only read the cache contents and write to their (pid-specific) overridden compiled modules directory.
	 */
	class LockInfo
	{
	public:
		LockInfo(void);
		~LockInfo(void);

		bool needsLock;  ///< True unless this cache doesn't need to be locked because processes are already prevented from interfering with each other when accessing it.
		VuoFileUtilities::File *dylibLockFile;  ///< File used for locking the cache dylib for reading and writing.
		VuoFileUtilities::File *compiledModulesLockFile;  ///< File used for locking the compiled modules directory for writing.
		bool hasLockForWriting;  ///< True if `compiledModulesLockFile` holds the lock.
	};

	static map<string, LockInfo *> interprocessLockInfo;  ///< Indexed by `cacheDirectoryPath`. Static so the data can be reused if an module cache is destroyed and recreated, e.g. with @ref VuoCompiler::reset.
	bool hasInterprocessLock;  ///< True if this instance either holds the lock for writing or doesn't need a lock. Stored in a data member so it can be accessed without concern for thread-safety of `interprocessLockInfo`.

	friend class TestModuleCaches;
	friend class ModuleCachesDiff;
	friend class ModuleScope;
};
