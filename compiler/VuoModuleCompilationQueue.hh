/**
 * @file
 * VuoModuleCompilationQueue interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include "VuoFileUtilities.hh"

/**
 * Enforces rules to make sure module source files are compiled in the correct order
 * and without consuming too many resources.
 *
 * - Only a limited number of source files can be compiled concurrently.
 * - Source files that depend on other source files are compiled after the source files they depend on.
 * - Successive versions of a source file are compiled in the order they were enqueued.
 *
 * This class is not responsible for ensuring that dependencies are compiled before the
 * source files that depend on them. That's handled by @ref VuoCompiler.
 */
class VuoModuleCompilationQueue
{
public:
	/**
	 * A description of a source file to be compiled.
	 */
	class Item
	{
	public:
		Item();
		string moduleKey;  ///< The module key.
		string sourcePath;  ///< The path of the source file.
		string sourceCode;  ///< The source code to compile, which may or may not be the same as the file contents.
		VuoFileUtilities::File *sourceFile;   ///< The source file, which may be in a directory or an archive.
		string cachedModulesPath;  ///< The directory in which the compiled file will be saved.
		string compiledModulePath;  ///< The path where the compiled file will be saved.
		dispatch_group_t loadingGroup;  ///< Enables callers to wait until the source is compiled and the resulting module is loaded.
		pair<int, int> priority;  ///< Priority in the queue (based on dependencies). Lower numbers are higher priority. First item in pair takes precedence over second.

		bool hasHigherPriority(Item *other);
	};

	VuoModuleCompilationQueue();
	void enqueue(Item *item);
	Item * next();
	void completed(Item *item);

private:
	Item * ripeItem(map<string, list< pair<Item *, bool> > >::iterator iter);
	bool& ripeItemIsCompiling(map<string, list< pair<Item *, bool> > >::iterator iter);
	void pluckRipeItem(map<string, list< pair<Item *, bool> > >::iterator iter);

	/**
	 * Key: Source path.
	 * Value: The list of versions enqueued for that source path, with oldest in front.
	 * The bool is false initially after @ref enqueue(), then true after being returned by @ref next().
	 * Elements are removed from the list by @ref completed().
	 */
	map<string, list< pair<Item *, bool> > > queue;

	std::mutex queueMutex;  ///< Synchronizes access to @ref queue.
	std::condition_variable queueChanged;  ///< Notifies threads waiting for an item to become available.

	static const int maxConcurrentCompilations;  ///< The maximum number of source files that are allowed be compiled concurrently.
};
