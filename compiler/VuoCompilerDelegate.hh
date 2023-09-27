/**
 * @file
 * VuoCompilerDelegate interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompilerIssues;
class VuoCompilerModule;

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
	 * This function may be called as a result of a request for modules (e.g. VuoCompiler::getNodeClass()) or
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
	friend class VuoCompilerEnvironment;
};
