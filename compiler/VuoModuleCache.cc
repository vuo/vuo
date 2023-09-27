/**
 * @file
 * VuoModuleCache implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCache.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerIssue.hh"
#include "VuoException.hh"
#include "VuoModuleCacheRevision.hh"
#include "VuoModuleInfo.hh"
#include "VuoStringUtilities.hh"
#include "VuoTimeUtilities.hh"
#include <climits>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>

std::mutex VuoModuleCache::buildMutex;
int VuoModuleCache::buildsInProgress = 0;
std::condition_variable VuoModuleCache::buildsInProgressCondition;
std::mutex VuoModuleCache::buildsInProgressMutex;
const string VuoModuleCache::builtInCacheDirName = "Builtin";
const string VuoModuleCache::systemCacheDirName = "System";
const string VuoModuleCache::userCacheDirName = "User";
const string VuoModuleCache::pidCacheDirPrefix = "pid-";
map<string, VuoModuleCache::LockInfo *> VuoModuleCache::interprocessLockInfo;

/**
 * Constructs an instance that doesn't yet refer to any files or locks.
 */
VuoModuleCache::LockInfo::LockInfo(void)
{
	needsLock = true;
	dylibLockFile = nullptr;
	compiledModulesLockFile = nullptr;
	hasLockForWriting = false;
}

/**
 * Releases any file locks held and deallocates memory.
 */
VuoModuleCache::LockInfo::~LockInfo(void)
{
	if (dylibLockFile)
		dylibLockFile->unlock();

	if (compiledModulesLockFile)
		compiledModulesLockFile->unlock();

	delete dylibLockFile;
	delete compiledModulesLockFile;
}

/**
 * Creates a VuoModuleCache instance for modules at the built-in scope.
 */
shared_ptr<VuoModuleCache> VuoModuleCache::newBuiltInCache(void)
{
	try
	{
		return shared_ptr<VuoModuleCache>(new VuoModuleCache(VuoCompiler::getVuoFrameworkPath() + "/Modules/" + builtInCacheDirName, true));
	}
	catch (VuoException &e)
	{
		return nullptr;
	}
}

/**
 * Creates a VuoModuleCache instance for modules at the system scope.
 */
shared_ptr<VuoModuleCache> VuoModuleCache::newSystemCache(void)
{
	try
	{
		return shared_ptr<VuoModuleCache>(new VuoModuleCache(VuoFileUtilities::getCachePath() + "/" + systemCacheDirName, false));
	}
	catch (VuoException &e)
	{
		return nullptr;
	}
}

/**
 * Creates a VuoModuleCache instance for modules at the user scope.
 */
shared_ptr<VuoModuleCache> VuoModuleCache::newUserCache(void)
{
	try
	{
		return shared_ptr<VuoModuleCache>(new VuoModuleCache(VuoFileUtilities::getCachePath() + "/" + userCacheDirName, false));
	}
	catch (VuoException &e)
	{
		return nullptr;
	}
}

/**
 * Creates a VuoModuleCache instance for modules at a composition-family, composition, or other scope.
 *
 * @param uniquePath A path that is unique to the scope, which will be mangled to form the cache directory name.
 */
shared_ptr<VuoModuleCache> VuoModuleCache::newCache(const string &uniquePath)
{
	try
	{
		return shared_ptr<VuoModuleCache>(new VuoModuleCache(VuoModuleCache::getCacheDirectoryPath(uniquePath), false));
	}
	catch (VuoException &e)
	{
		return nullptr;
	}
}

/**
 * Constructs an instance that has not yet been made available.
 *
 * Creates @a cacheDirectoryPath if it doesn't already exist. Claims exclusive access to write to the
 * cache directory (other than pid-specific subdirectories) if no other process is already using it.
 *
 * @throw VuoException The user doesn't have permission either to create the cache directory or to write
 *    the file for locking the compiled modules directory.
 */
VuoModuleCache::VuoModuleCache(const string &cacheDirectoryPath, bool builtIn)
{
	this->cacheDirectoryPath = cacheDirectoryPath;
	VuoFileUtilities::canonicalizePath(this->cacheDirectoryPath);
	this->builtIn = builtIn;
	available = false;
	invalidated = true;
	lastRebuild = 0;
	hasInterprocessLock = false;

	LockInfo *lockInfo = interprocessLockInfo[cacheDirectoryPath];
	if (! lockInfo)
	{
		VuoFileUtilities::makeDir(cacheDirectoryPath);

		lockInfo = new LockInfo;
		interprocessLockInfo[cacheDirectoryPath] = lockInfo;

		if (builtIn)
		{
			lockInfo->needsLock = false;
		}
		else
		{
			string lockFileName = "modules.lock";
			VuoFileUtilities::createFile(cacheDirectoryPath + "/" + lockFileName);

			lockInfo->compiledModulesLockFile = new VuoFileUtilities::File(cacheDirectoryPath, lockFileName);
			lockInfo->hasLockForWriting = lockInfo->compiledModulesLockFile->lockForWriting(true);

			if (lockInfo->hasLockForWriting)
			{
				ostringstream pid;
				pid << getpid() << endl;
				VuoFileUtilities::writeStringToFile(pid.str(), lockInfo->compiledModulesLockFile->path());
			}
		}
	}

	hasInterprocessLock = ! lockInfo->needsLock || lockInfo->hasLockForWriting;
}

/**
 * Returns the absolute path to the local cache directory for a composition, composition family, or test.
 *
 * @param uniquePath A path that has a one-to-one correspondence to the cache's scope.
 *    The returned value is generated by mangling this path string.
 */
string VuoModuleCache::getCacheDirectoryPath(const string &uniquePath)
{
	string modifierLetterColon("꞉");  // Unicode "Modifier Letter Colon", not a regular ASCII colon. OK to include in a file name.
	string relativePath = uniquePath.empty() ? "/" : uniquePath;
	VuoStringUtilities::replaceAll(relativePath, "/", modifierLetterColon);
	return VuoFileUtilities::getCachePath() + "/" + relativePath;
}

/**
 * Helper function that returns the relative path of one of the directories of compiled modules within a cache.
 */
string VuoModuleCache::getRelativePathOfModulesDirectory(bool isGenerated, const string &targetArch)
{
	return string() + (isGenerated ? "Generated" : "Installed") + "/Modules/" + targetArch;
}

/**
 * Returns the directory containing compiled/generated bitcode for installed or generated modules.
 */
string VuoModuleCache::getCompiledModulesPath(bool isGenerated, const string &targetArch)
{
	return cacheDirectoryPath + "/" + getRelativePathOfModulesDirectory(isGenerated, targetArch);
}

/**
 * Returns the directory containing compiled/generated bitcode for unsaved edits to installed modules.
 */
string VuoModuleCache::getOverriddenCompiledModulesPath(bool isGenerated, const string &targetArch)
{
	ostringstream pid;
	pid << getpid();

	string dir, scope, unused;
	VuoFileUtilities::splitPath(cacheDirectoryPath, dir, scope, unused);
	VuoFileUtilities::canonicalizePath(scope);

	return VuoFileUtilities::getCachePath() + "/" + pidCacheDirPrefix + pid.str() + "/" + scope + "/" + getRelativePathOfModulesDirectory(isGenerated, targetArch);
}

/**
 * Attempts to modify the contents of the compiled modules directory in a way that is safe when
 * multiple processes are vying for the same directory.
 *
 * Returns true if this process holds the lock for writing to the module cache and thus was able to
 * modify the contents. Otherwise, returns false.
 */
bool VuoModuleCache::modifyCompiledModules(std::function<bool(void)> modify)
{
	if (hasInterprocessLock)
		return modify();

	return false;
}

/**
 * Returns a human-readable description of the module cache.
 */
string VuoModuleCache::getDescription(void)
{
	return string() + "the module cache at '" + cacheDirectoryPath + "'";
}

/**
 * Returns the path of the manifest file within the module cache.
 */
string VuoModuleCache::getManifestPath(const string &targetArch)
{
	string path = cacheDirectoryPath + "/manifest.txt";

	// Give the manifest for each built-in cache arch a unique name, so they can be built in parallel.
	if (builtIn && ! targetArch.empty())
		path += "-" + targetArch;

	return path;
}

/**
 * Returns the path of a dylib file within the module cache.
 * - For the built-in module caches, the returned path for a given target is always the same
 * - For other module caches, this function returns an unused path at which a new revision of the dylib can be saved.
 */
string VuoModuleCache::getDylibPath(const string &targetArch)
{
	string partialPath = cacheDirectoryPath + "/libVuoModuleCache";

	if (builtIn)
	{
		string path = partialPath + ".dylib";

		// Give the dylib for each built-in cache arch a unique name, so they can be built in parallel.
		if (! targetArch.empty())
			path += "-" + targetArch;

		return path;
	}

	auto isPathAvailable = [](const string &path)
	{
		return ! VuoFileUtilities::fileExists(path + ".dylib");
	};

	string preferredPath = partialPath + "-" + VuoTimeUtilities::getCurrentDateTimeForFileName();
	string uniquePath = VuoStringUtilities::formUniqueIdentifier(isPathAvailable, preferredPath, preferredPath + "-");
	return uniquePath + ".dylib";
}

/**
 * Returns the most recently created dylib file within the module cache, or an empty string if none exists.
 */
string VuoModuleCache::findLatestRevisionOfDylib(double &lastModified)
{
	if (builtIn)
		throw VuoException("Only call this function on a non-built-in module cache.");

	string hypotheticalPath = getDylibPath();

	string dir, file, ext;
	VuoFileUtilities::splitPath(hypotheticalPath, dir, file, ext);
	string hypotheticalFileName = file + "." + ext;

	vector< pair<string, double> > existingRevisions;
	for (VuoFileUtilities::File *existingFile : VuoFileUtilities::findFilesInDirectory(cacheDirectoryPath, {"dylib"}))
	{
		if (areDifferentRevisionsOfSameDylib(existingFile->getRelativePath(), hypotheticalFileName))
			existingRevisions.push_back({existingFile->path(), VuoFileUtilities::getFileLastModifiedInSeconds(existingFile->path())});

		delete existingFile;
	}

	if (existingRevisions.empty())
	{
		lastModified = 0;
		return "";
	}

	auto latest = std::max_element(existingRevisions.begin(), existingRevisions.end(),
								   [](pair<string, double> p1, pair<string, double> p2) { return p1.second < p2.second; });

	lastModified = latest->second;
	return latest->first;
}

/**
 * Returns true if the two paths differ only in their revision suffix.
 */
bool VuoModuleCache::areDifferentRevisionsOfSameDylib(const string &dylibPath1, const string &dylibPath2)
{
	string dir1, dir2, file1, file2, ext;
	VuoFileUtilities::splitPath(dylibPath1, dir1, file1, ext);
	VuoFileUtilities::splitPath(dylibPath2, dir2, file2, ext);

	if (VuoFileUtilities::arePathsEqual(dir1, dir2))
	{
		vector<string> parts1 = VuoStringUtilities::split(file1, '-');
		vector<string> parts2 = VuoStringUtilities::split(file2, '-');

		if (parts1.size() == 2 && parts2.size() == 2)
			return parts1.front() == parts2.front() && parts1.back() != parts2.back();
	}

	return false;
}

/**
 * Attempts to make the module cache available — either making it available initially or making it available
 * again after a change to the cacheable contents has made it out-of-date.
 *
 * If the cache is out-of-date (and @a shouldUseExistingCache is false), it is scheduled to be rebuilt
 * asynchronously.
 *
 * If the cacheable contents haven't changed since the last time this function was called, this call
 * just returns without checking or rebuilding the cache.
 *
 * The caller must call VuoModuleCache::waitForCachesToBuild() after one or more calls to this function
 * to make sure spawned threads complete before the main thread exits.
 *
 * @param shouldUseExistingCache If true, the existing cache (if any) is used, without checking if it's
 *     consistent with the expected contents.
 * @param prerequisiteModuleCaches The caches at broader scopes on which this cache depends.
 * @param lastPrerequisiteModuleCacheRebuild When making a series of caches available in order from broadest to narrowest scope,
 *     pass in a variable set to 0 on the first call and the same variable on subsequent calls. This function updates the variable
 *     to either the last-modified time of the cache if it doesn't need to be rebuilt or a very large number if the cache does
 *     need to be rebuilt.
 * @param expectedManifest A list of the modules and static libraries that the module cache should contain when up-to-date.
 * @param expectedModules Iterators to get information about the module files for all modules that the module cache should contain.
 * @param dylibsToLinkTo The paths of all dylibs that need to be linked into the dylib when rebuilding it.
 * @param frameworksToLinkTo The names of all frameworks that need to be linked into the dylib when rebuilding it.
 * @param runPathSearchPaths The rpaths that need to be passed to the linker when rebuilding the cache.
 * @param compiler The compiler to use for linking the cache (if it needs to be rebuilt).
 * @param targetArch If non-empty, the cache will only be checked and rebuilt for this single architecture.
 *
 * @threadQueue{VuoCompiler::environmentQueue}
 */
void VuoModuleCache::makeAvailable(bool shouldUseExistingCache,
								   vector<shared_ptr<VuoModuleCache>> prerequisiteModuleCaches, double &lastPrerequisiteModuleCacheRebuild,
								   const VuoModuleCacheManifest &expectedManifest, vector<VuoModuleInfoIterator> expectedModules,
								   const set<string> &dylibsToLinkTo, const set<string> &frameworksToLinkTo, const vector<string> &runPathSearchPaths,
								   VuoCompiler *compiler, const string &targetArch)
{
	string cacheDescription = getDescription();

	std::unique_lock<std::mutex> statusLock(statusMutex);

	// Don't bother rechecking the cache if neither the cacheable modules nor the caches that it depends on have changed.

	if (! invalidated && (builtIn || lastRebuild >= lastPrerequisiteModuleCacheRebuild))
	{
		VDebugLog("No need to recheck %s.", cacheDescription.c_str());
		lastPrerequisiteModuleCacheRebuild = lastRebuild;
		return;
	}

	try
	{
		VDebugLog("Checking if %s is up-to-date…", cacheDescription.c_str());

		bool isCacheUpToDate = true;
		invalidated = false;

		statusLock.unlock();

		string manifestPath = getManifestPath(targetArch);

		// Lock the cache for reading.

		LockInfo *lockInfo = interprocessLockInfo[cacheDirectoryPath];
		VuoFileUtilities::File *fileForLocking = nullptr;

		if (lockInfo->needsLock)
		{
			if (! lockInfo->dylibLockFile)
			{
				VuoFileUtilities::makeDir(cacheDirectoryPath);

				string lockFileName = "dylib.lock";
				VuoFileUtilities::createFile(cacheDirectoryPath + "/" + lockFileName);

				lockInfo->dylibLockFile = new VuoFileUtilities::File(cacheDirectoryPath, lockFileName);
			}

			fileForLocking = lockInfo->dylibLockFile;
			if (! fileForLocking->lockForReading())
				VDebugLog("\tWarning: Couldn't lock for reading.");
		}

		// Create the manifest file if it doesn't already exist. (If it does exist, don't affect the last-modified time.)

		bool manifestFileExists = VuoFileUtilities::fileExists(manifestPath);
		if (! manifestFileExists)
		{
			if (shouldUseExistingCache)
				throw VuoException("Trying to use the existing cache, but the cache manifest doesn't exist.", false);

			VuoFileUtilities::createFile(manifestPath);
			isCacheUpToDate = false;
		}

		// If this is the first time making the cache available, see if there's a dylib on disk.

		double lastRebuild_local = 0;
		string dylibPath = builtIn ? getDylibPath(targetArch) : findLatestRevisionOfDylib(lastRebuild_local);

		if (dylibPath.empty())
		{
			if (shouldUseExistingCache)
				throw VuoException("Trying to use the existing cache, but the cache dylib doesn't exist.", false);

			dylibPath = getDylibPath();
		}

		// Check if the dylib is newer than the other caches that it depends on.

		if (isCacheUpToDate)
			isCacheUpToDate = lastRebuild_local >= lastPrerequisiteModuleCacheRebuild;

		// Check if the dylib looks remotely valid.

		if (isCacheUpToDate)
		{
			bool dylibHasData = VuoFileUtilities::fileContainsReadableData(dylibPath);
			if (! dylibHasData)
			{
				if (shouldUseExistingCache)
					throw VuoException("Trying to use the existing cache, but the cache doesn't contain readable data.", false);
				else
					isCacheUpToDate = false;
			}
		}

		// List the items actually in the cache, according to its manifest.

		VuoModuleCacheManifest actualManifest;
		bool usingExistingCache = false;

		if (isCacheUpToDate || shouldUseExistingCache)
		{
			actualManifest.readFromFile(manifestPath);

			if (shouldUseExistingCache)
				usingExistingCache = true;
		}

		// Check if the list of actual items matches the list of expected items.

		if (isCacheUpToDate && ! usingExistingCache)
		{
			if (! actualManifest.hasSameContentsAs(expectedManifest))
				isCacheUpToDate = false;
		}

		// Check if the cache is newer than all of the modules in it.

		if (isCacheUpToDate && ! usingExistingCache)
		{
			for (VuoModuleInfoIterator i : expectedModules)
			{
				VuoModuleInfo *moduleInfo;
				while ((moduleInfo = i.next()))
				{
					if (! moduleInfo->isOlderThan(lastRebuild_local))
					{
						isCacheUpToDate = false;
						break;
					}
				}
			}
		}

		// If the cache is up-to-date, we're done.

		if (isCacheUpToDate || usingExistingCache)
		{
			VDebugLog("Up-to-date.");

			lastPrerequisiteModuleCacheRebuild = lastRebuild_local;

			std::lock_guard<std::mutex> contentsLock(contentsMutex);
			if (! currentRevision)
				currentRevision = VuoModuleCacheRevision::createAndUse(dylibPath, actualManifest, builtIn, ! builtIn);

			std::lock_guard<std::mutex> statusLock(statusMutex);
			lastRebuild = lastRebuild_local;
			if (! invalidated)
				available = true;

			return;
		}

		// If this process is not allowed to rebuild the cache, give up.

		if (! hasInterprocessLock)
			throw VuoException("The cache file is out-of-date but can't be rebuilt because it's being used by another process. "
							   "If any composition windows are open from previous Vuo sessions, quit them. "
							   "If any processes whose names start with \"VuoComposition\" or one of your composition file names appear in Activity Monitor, force-quit them.",
							   false);

		// Otherwise, (re)build the cache asynchronously.

		lastPrerequisiteModuleCacheRebuild = ULONG_MAX;

		{
			std::lock_guard<std::mutex> contentsLock(contentsMutex);
			if (currentRevision)
			{
				currentRevision->disuse();
				currentRevision = nullptr;
			}
			else if (VuoFileUtilities::fileExists(dylibPath))
			{
				// Clean up the dylib that existed before making this VuoModuleCache available for the first time.
				auto revision = VuoModuleCacheRevision::createAndUse(dylibPath, actualManifest, builtIn, true);
				revision->disuse();
			}

			std::lock_guard<std::mutex> statusLock(statusMutex);
			available = false;
		}

		auto rebuild = [this, prerequisiteModuleCaches, expectedManifest, dylibsToLinkTo, frameworksToLinkTo, runPathSearchPaths, compiler, targetArch,
					   cacheDescription, manifestPath, fileForLocking]()
		{
			std::lock_guard<std::mutex> buildLock(buildMutex);

			// Wait for other caches that this cache depends on to finish rebuilding so their dylibs are available to link to.
			vector<shared_ptr<VuoModuleCacheRevision>> prerequisiteModuleCacheRevisions;
			bool arePrerequisiteModuleCachesAvailable = true;
			for (shared_ptr<VuoModuleCache> other : prerequisiteModuleCaches)
			{
				shared_ptr<VuoModuleCacheRevision> revision = other->useCurrentRevision();
				if (! revision)
				{
					arePrerequisiteModuleCachesAvailable = false;
					break;
				}

				prerequisiteModuleCacheRevisions.push_back(revision);
			}

			if (arePrerequisiteModuleCachesAvailable)
			{
				VUserLog("Rebuilding %s…", cacheDescription.c_str());

				bool gotLockForWriting = false;
				try
				{
					std::lock_guard<std::mutex> contentsLock(contentsMutex);

					string dylibPath = getDylibPath(targetArch);

					VuoLinkerInputs linkerInputs;
					linkerInputs.addDependencies(expectedManifest.getContents(), {}, compiler);
					linkerInputs.addExternalLibraries(dylibsToLinkTo);
					linkerInputs.addFrameworks(frameworksToLinkTo);

					for (shared_ptr<VuoModuleCacheRevision> revision : prerequisiteModuleCacheRevisions)
						linkerInputs.addExternalLibrary(revision->getDylibPath());

					// Try to upgrade the file lock for writing.
					if (fileForLocking)
					{
						gotLockForWriting = fileForLocking->lockForWriting(true);
						if (! gotLockForWriting)
							throw VuoException("Couldn't upgrade the lock to writing.", false);

						ostringstream pid;
						pid << getpid() << endl;
						VuoFileUtilities::writeStringToFile(pid.str(), fileForLocking->path());
					}

					// Link the dependencies to create the cache dylib in a temporary file.
					string dir, file, ext;
					VuoFileUtilities::splitPath(dylibPath, dir, file, ext);
					string tmpPath = VuoFileUtilities::makeTmpFile(file, "dylib");
					VuoCompilerIssues *issues = new VuoCompilerIssues;
					compiler->link(tmpPath, linkerInputs, true, runPathSearchPaths, false, issues);

					if (! issues->isEmpty())
						VUserLog("Warning: May not have fully rebuilt %s:\n%s", cacheDescription.c_str(), issues->getLongDescription(false).c_str());

					// Move the temporary file into the cache.
					VuoFileUtilities::moveFile(tmpPath, dylibPath);

					// Change the dylib's ID from the temporary path to the path within the cache.
					VuoFileUtilities::executeProcess({
						VuoCompiler::getVuoFrameworkPath() + "/Helpers/install_name_tool",
						"-id",
						dylibPath,
						dylibPath,
					});

					// Ad-hoc code-sign the runtime-generated System and User caches,
					// but don't ad-hoc code-sign the buildtime-generated Builtin module cache
					// since `framework/CMakeLists.txt` later changes its ID/rpath/loads.
					if (VuoCompiler::vuoFrameworkInProgressPath.empty())
						VuoCompiler::adHocCodeSign(dylibPath);

					// Write the list of dependencies to the manifest file.
					expectedManifest.writeToFile(manifestPath);

					// Downgrade the file lock back to reading.
					if (fileForLocking && ! fileForLocking->lockForReading())
						VDebugLog("\tWarning: Couldn't downgrade the lock back to reading.");

					currentRevision = VuoModuleCacheRevision::createAndUse(dylibPath, expectedManifest, builtIn, ! builtIn);

					std::lock_guard<std::mutex> statusLock(statusMutex);

					if (! builtIn)
						lastRebuild = VuoFileUtilities::getFileLastModifiedInSeconds(dylibPath);

					if (! invalidated)
						available = true;
				}
				catch (VuoException &e)
				{
					// Downgrade the file lock back to reading.
					if (gotLockForWriting)
						if (! fileForLocking->lockForReading())
							VDebugLog("\tWarning: Couldn't downgrade the lock back to reading.");

					VUserLog("Warning: Couldn't rebuild %s: %s", cacheDescription.c_str(), e.what());
				}
			}
			else
			{
				VDebugLog("Not rebuilding %s since one of the module caches it depends on is unavailable.", cacheDescription.c_str());
			}

			for (shared_ptr<VuoModuleCacheRevision> revision : prerequisiteModuleCacheRevisions)
				revision->disuse();

			{
				std::unique_lock<std::mutex> lck(buildsInProgressMutex);
				--buildsInProgress;
				buildsInProgressCondition.notify_all();
			}

			VUserLog("Done.");
		};

		{
			std::lock_guard<std::mutex> lck(buildsInProgressMutex);
			++buildsInProgress;
		}

		// This function is executing on environmentQueue. The `rebuild` lambda cannot execute on environmentQueue
		// since it calls VuoCompiler::getLinkerInputs(). So, spawn a separate thread to get off of environmentQueue.
		std::thread t(rebuild);
		t.detach();
	}
	catch (VuoException &e)
	{
		if (VuoCompiler::vuoFrameworkInProgressPath.empty())
			VUserLog("Warning: Couldn't make %s available: %s", cacheDescription.c_str(), e.what());
		else if (builtIn)
			VUserLog("Warning: Couldn't create %s when generating built-in module caches: %s", cacheDescription.c_str(), e.what());

		lastPrerequisiteModuleCacheRebuild = ULONG_MAX;
	}
}

/**
 * Waits for cache (re)builds scheduled by @ref makeAvailable to complete.
 */
void VuoModuleCache::waitForCachesToBuild(void)
{
	std::unique_lock<std::mutex> lck(buildsInProgressMutex);
	while (buildsInProgress > 0)
		buildsInProgressCondition.wait(lck);
}

/**
 * Returns the most recent revision of this module cache if one is available, otherwise null.
 *
 * The caller is responsible for calling VuoModuleCacheRevision::disuse() on the returned revision
 * when finished using it.
 */
shared_ptr<VuoModuleCacheRevision> VuoModuleCache::useCurrentRevision(void)
{
	std::lock_guard<std::mutex> contentsLock(contentsMutex);
	std::lock_guard<std::mutex> statusLock(statusMutex);

	if (available)
	{
		currentRevision->use();
		return currentRevision;
	}

	return nullptr;
}

/**
 * Indicates that the cacheable data has changed, so the cache may need to be rebuilt.
 */
void VuoModuleCache::invalidate(void)
{
	std::lock_guard<std::mutex> statusLock(statusMutex);

	invalidated = true;
	available = false;
}

/**
 * Deletes:
 *    - any composition-local module cache directories that have not been accessed for more than 30 days.
 *    - any pid-specific module cache directories for pids that are not currently running.
 */
void VuoModuleCache::deleteOldCaches(void)
{
	double maxSeconds = 30 * 24 * 60 * 60;  // 30 days

	set<VuoFileUtilities::File *> cacheDirs = VuoFileUtilities::findAllFilesInDirectory(VuoFileUtilities::getCachePath());
	for (set<VuoFileUtilities::File *>::iterator i = cacheDirs.begin(); i != cacheDirs.end(); ++i)
	{
		string path = (*i)->path();

		string file = (*i)->basename();
		if (file != builtInCacheDirName && file != systemCacheDirName && file != userCacheDirName)
		{
			double fileSeconds = VuoFileUtilities::getSecondsSinceFileLastAccessed(path);
			if (fileSeconds > maxSeconds)
				VuoFileUtilities::deleteDir(path);
		}

		if (VuoStringUtilities::beginsWith(file, pidCacheDirPrefix))
		{
			string pidAsString = file.substr(pidCacheDirPrefix.length());
			int pid = atoi(pidAsString.c_str());
			if (kill(pid, 0) != 0)  // no running process has this pid
				VuoFileUtilities::deleteDir(path);
		}

		delete *i;
	}
}
