/**
 * @file
 * VuoCompilerDelegate implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerDelegate.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerIssue.hh"

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
